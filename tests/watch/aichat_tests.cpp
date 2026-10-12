#include "AiChatFailure.h"
#include <cassert>
#include <cstring>
#include <cstdio>
int main() {
  using dpwatch::aiChatFailureMessage;
  assert(std::strstr(aiChatFailureMessage(true, "Start protocol"), "memory"));
  assert(std::strstr(aiChatFailureMessage(true, "Initialize audio"), "memory"));
  assert(std::strstr(aiChatFailureMessage(false, "Initialize audio processor"), "Audio could not start"));
  assert(std::strstr(aiChatFailureMessage(false, "Acquire Audio session"), "Audio could not start"));
  assert(std::strstr(aiChatFailureMessage(false, "Get service information"), "connection failed"));
  assert(std::strstr(aiChatFailureMessage(false, nullptr), "Retrying"));
  puts("AI Chat failures: local memory/audio errors and connection retry guidance passed");
}
