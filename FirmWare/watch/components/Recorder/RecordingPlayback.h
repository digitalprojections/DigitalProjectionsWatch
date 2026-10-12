#pragma once
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
namespace dpwatch {
inline bool canStartRecorderWork(bool stopped, bool session, bool codec) {
  return stopped && !session && !codec;
}
struct WavRecording {
  uint32_t sampleRate = 0, dataBytes = 0;
  uint16_t channels = 0;
};
inline uint32_t wav32(const unsigned char *p) {
  return uint32_t(p[0]) | (uint32_t(p[1]) << 8) | (uint32_t(p[2]) << 16) |
         (uint32_t(p[3]) << 24);
}
inline uint16_t wav16(const unsigned char *p) {
  return uint16_t(p[0]) | uint16_t(p[1] << 8);
}
// Validate the complete chunk range before configuring the codec. Leave FILE at
// PCM.
inline bool readRecording(FILE *f, WavRecording &out) {
  out = {};
  if (!f || fseek(f, 0, SEEK_END) != 0)
    return false;
  const long length = ftell(f);
  if (length < 44 || fseek(f, 0, SEEK_SET) != 0)
    return false;
  unsigned char h[16];
  if (fread(h, 1, 12, f) != 12 || memcmp(h, "RIFF", 4) ||
      memcmp(h + 8, "WAVE", 4))
    return false;
  const uint64_t end = uint64_t(wav32(h + 4)) + 8;
  if (end > uint64_t(length) || end < 44)
    return false;
  bool format = false;
  uint16_t align = 0;
  for (uint64_t pos = 12; pos + 8 <= end;) {
    if (fread(h, 1, 8, f) != 8)
      return false;
    const uint32_t size = wav32(h + 4);
    const uint64_t next = pos + 8 + size + (size & 1U);
    if (next > end)
      return false;
    if (!memcmp(h, "fmt ", 4)) {
      if (size < 16 || fread(h, 1, 16, f) != 16)
        return false;
      out.channels = wav16(h + 2);
      out.sampleRate = wav32(h + 4);
      align = wav16(h + 12);
      if (wav16(h) != 1 || (out.channels != 1 && out.channels != 2) ||
          wav16(h + 14) != 16 || out.sampleRate < 8000 ||
          out.sampleRate > 48000 || align != out.channels * 2 ||
          wav32(h + 8) != out.sampleRate * align)
        return false;
      format = true;
    } else if (!memcmp(h, "data", 4)) {
      if (!format || !size || size % align)
        return false;
      out.dataBytes = size;
      return true;
    }
    if (next > 0x7fffffffULL ||
        fseek(f, static_cast<long>(next), SEEK_SET) != 0)
      return false;
    pos = next;
  }
  return false;
}
// Only six names are retained, regardless of the size of the SD directory.
struct RecordingPage {
  static constexpr size_t visible = 5;
  std::array<std::string, visible + 1> names{};
  size_t count = 0;
  std::string before;
  explicit RecordingPage(const char *cursor = "") : before(cursor) {}
  void offer(const char *filename) {
    std::string n(filename);
    if (n.size() < 5 || n.size() > 150 || n.find('/') != std::string::npos ||
        n.find('\\') != std::string::npos || n.substr(n.size() - 4) != ".wav" ||
        (!before.empty() && n >= before))
      return;
    size_t i = 0;
    while (i < count && names[i] > n)
      ++i;
    if (i < count && names[i] == n)
      return;
    if (i >= names.size())
      return;
    const size_t last = std::min(count, names.size() - 1);
    for (size_t j = last; j > i; --j)
      names[j] = names[j - 1];
    names[i] = n;
    count = std::min(count + 1, names.size());
  }
  bool hasMore() const { return count > visible; }
};
} // namespace dpwatch
