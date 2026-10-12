#pragma once
namespace dpwatch {
enum class SetupState {
  Off,
  Starting,
  Waiting,
  Connecting,
  Success,
  Error,
  Stopping,
  Unavailable
};
class ProvisioningState {
  SetupState value = SetupState::Off;
  bool active = false;

public:
  SetupState state() const { return value; }
  bool canStart() const { return !active; }
  bool canStop() const { return active && value != SetupState::Stopping; }
  bool start() {
    if (active)
      return false;
    active = true;
    value = SetupState::Starting;
    return true;
  }
  void advertise() {
    if (value == SetupState::Starting)
      value = SetupState::Waiting;
  }
  void received() {
    if (active && (value == SetupState::Waiting || value == SetupState::Error))
      value = SetupState::Connecting;
  }
  void failed() {
    if (value == SetupState::Connecting)
      value = SetupState::Error;
  }
  void succeeded() {
    if (value == SetupState::Connecting)
      value = SetupState::Success;
  }
  void stop() {
    if (canStop())
      value = SetupState::Stopping;
  }
  void finished() {
    active = false;
    if (value != SetupState::Success && value != SetupState::Error)
      value = SetupState::Off;
  }
  void unavailable() {
    active = false;
    value = SetupState::Unavailable;
  }
};
} // namespace dpwatch
