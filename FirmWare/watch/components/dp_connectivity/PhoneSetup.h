#pragma once
#include "ProvisioningState.h"
namespace dpwatch {
struct PhoneSetupSnapshot {
  SetupState state = SetupState::Off;
  bool canStart = true, canStop = false;
  char name[24]{};
  char code[9]{};
};
bool startPhoneSetup();
void stopPhoneSetup();
PhoneSetupSnapshot phoneSetupSnapshot();
} // namespace dpwatch
