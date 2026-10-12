#include "ProvisioningState.h"
#include <cassert>
#include <cstdio>
using namespace dpwatch;
int main() {
  ProvisioningState s;
  assert(s.canStart() && !s.canStop());
  assert(s.start());
  assert(!s.start());
  assert(s.state() == SetupState::Starting);
  s.advertise();
  assert(s.state() == SetupState::Waiting && s.canStop());
  s.received();
  assert(s.state() == SetupState::Connecting);
  s.failed();
  assert(s.state() == SetupState::Error && s.canStop() && !s.canStart());
  s.received();
  s.succeeded();
  assert(s.state() == SetupState::Success);
  s.finished();
  assert(s.state() == SetupState::Success && s.canStart() && !s.canStop());
  assert(s.start());
  s.advertise();
  s.stop();
  assert(s.state() == SetupState::Stopping);
  s.succeeded();
  assert(s.state() == SetupState::Stopping);
  s.finished();
  assert(s.state() == SetupState::Off);
  assert(s.start());
  s.unavailable();
  assert(s.state() == SetupState::Unavailable && s.canStart());
  s.received();
  assert(s.state() == SetupState::Unavailable);
  std::puts("Provisioning state tests passed: startup, busy gating, retry, "
            "success, cancel, stale events and unavailable.");
}
