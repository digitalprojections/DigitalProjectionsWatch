#include "MotionService.h"
#include "DisplayPower.h"
#include "bsp/esp-bsp.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs.h"
#include "qmi8658.h"
#include <atomic>
#include <new>
namespace dpwatch {
namespace {
constexpr char TAG[] = "DP:Motion";
class NvsCalibrationStore final : public CalibrationStore {
  public:
    bool load(CalibrationProfile &profile) override {
        nvs_handle_t handle;
        if (nvs_open("dp_motion", NVS_READONLY, &handle) != ESP_OK)
            return false;
        size_t size = sizeof(profile);
        const auto error = nvs_get_blob(handle, "calibration", &profile, &size);
        nvs_close(handle);
        return error == ESP_OK && size == sizeof(profile);
    }
    bool save(const CalibrationProfile &profile) override {
        nvs_handle_t handle;
        if (nvs_open("dp_motion", NVS_READWRITE, &handle) != ESP_OK)
            return false;
        auto error = nvs_set_blob(handle, "calibration", &profile, sizeof(profile));
        if (error == ESP_OK)
            error = nvs_commit(handle);
        nvs_close(handle);
        return error == ESP_OK;
    }
};
portMUX_TYPE snapshotLock = portMUX_INITIALIZER_UNLOCKED;
MotionSnapshot published;
std::atomic<bool> calibrationRequested{false};
TaskHandle_t worker = nullptr;
unsigned milliseconds() { return static_cast<unsigned>(esp_timer_get_time() / 1000); }
void publish(const MotionSnapshot &value) {
    portENTER_CRITICAL(&snapshotLock);
    published = value;
    portEXIT_CRITICAL(&snapshotLock);
}
bool configure(qmi8658_dev_t &sensor) {
    auto bus = bsp_i2c_get_handle();
    if (!bus)
        return false;
    // Probe before constructing the device so a failed address does not leak a handle.
    const uint8_t address = i2c_master_probe(bus, 0x6B, 40) == ESP_OK ? 0x6B : 0x6A;
    if (i2c_master_probe(bus, address, 40) != ESP_OK)
        return false;
    if (qmi8658_init(&sensor, bus, address) != ESP_OK)
        return false;
    qmi8658_set_accel_unit_mg(&sensor, true);
    qmi8658_set_gyro_unit_dps(&sensor, true);
    // Set both sensors explicitly: the factory GravitySphere only configured acceleration.
    // CTRL7 off, accelerometer 4g/125Hz, gyro 256dps/125Hz, low-pass filters, both on.
    return qmi8658_write_register(&sensor, QMI8658_CTRL7, 0x00) == ESP_OK &&
           qmi8658_set_accel_range(&sensor, QMI8658_ACCEL_RANGE_4G) == ESP_OK &&
           qmi8658_set_accel_odr(&sensor, QMI8658_ACCEL_ODR_125HZ) == ESP_OK &&
           qmi8658_set_gyro_range(&sensor, QMI8658_GYRO_RANGE_256DPS) == ESP_OK &&
           qmi8658_set_gyro_odr(&sensor, QMI8658_GYRO_ODR_125HZ) == ESP_OK &&
           qmi8658_write_register(&sensor, QMI8658_CTRL5, 0x77) == ESP_OK &&
           qmi8658_write_register(&sensor, QMI8658_CTRL7, 0x03) == ESP_OK;
}
void run(void *) {
    NvsCalibrationStore store;
    CalibrationService calibration(store);
    qmi8658_dev_t sensor{};
    const bool available = configure(sensor);
    MotionSnapshot state;
    ESP_LOGI(TAG, "Sensor %s; saved calibration %s", available ? "ready" : "unavailable",
             calibration.hasProfile() ? "loaded" : "absent");
    auto lastWake = xTaskGetTickCount();
    for (;;) {
        const auto now = milliseconds();
        if (calibrationRequested.exchange(false) && available &&
            calibration.state() != CalibrationState::Loading)
            calibration.start(now);
        bool fresh = false;
        if (available) {
            bool ready = false;
            qmi8658_data_t data{};
            if (qmi8658_is_data_ready(&sensor, &ready) == ESP_OK && ready &&
                qmi8658_read_sensor_data(&sensor, &data) == ESP_OK) {
                const MotionSample raw{data.accelX / 1000.0f,
                                       data.accelY / 1000.0f,
                                       data.accelZ / 1000.0f,
                                       data.gyroX,
                                       data.gyroY,
                                       data.gyroZ};
                calibration.sample(raw, now);
                state.sample = calibration.correct(raw);
                state.timestamp = now;
                fresh = true;
            }
            calibration.tick(now);
        }
        state.state = available ? calibration.state() : CalibrationState::Unavailable;
        if (!fresh && now - state.timestamp > 500)
            state.state = CalibrationState::Unavailable;
        state.generation = calibration.hasProfile() ? calibration.profile().generation : 0;
        state.bootPressed = gpio_get_level(GPIO_NUM_0) == 0 && !displayConsumesBoot();
        publish(state);
        vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(20));
    }
}
} // namespace
bool startMotionService() {
    if (worker)
        return true;
    gpio_config_t button{};
    button.pin_bit_mask = 1ULL << GPIO_NUM_0;
    button.mode = GPIO_MODE_INPUT;
    button.pull_up_en = GPIO_PULLUP_ENABLE;
    if (gpio_config(&button) != ESP_OK)
        return false;
    return xTaskCreate(run, "dp_motion", 4096, nullptr, 4, &worker) == pdPASS;
}
MotionSnapshot motionSnapshot() {
    portENTER_CRITICAL(&snapshotLock);
    auto result = published;
    portEXIT_CRITICAL(&snapshotLock);
    result.ageMs = milliseconds() - result.timestamp;
    return result;
}
void requestMotionCalibration() { calibrationRequested.store(true); }
} // namespace dpwatch
