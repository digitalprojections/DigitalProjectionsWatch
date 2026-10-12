#pragma once
#include <cstring>
namespace dpwatch {
// Keep local initialization failures distinct from cloud connectivity failures.
inline const char *aiChatFailureMessage(bool outOfMemory, const char *source) {
  if (outOfMemory) return "Not enough memory. Close AI Chat and retry.";
  if (source && (std::strstr(source, "audio") || std::strstr(source, "Audio")))
    return "Audio could not start. Close AI Chat and retry.";
  return "AI connection failed. Retrying automatically.";
}
}
