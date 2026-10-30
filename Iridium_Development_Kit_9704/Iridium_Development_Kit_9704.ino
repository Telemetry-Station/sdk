/* SPDX-License-Identifier: MIT */
/*
 * Telemetry Station messaging example for the Iridium 9704 LaunchPad
 *
 * Open this folder in the Arduino IDE. Set your settings in src/ts_config.h,
 * then use the Serial Monitor at 115200 baud. Type ? for the commands.
 * The code that does the work is in src/.
 */
#include "src/ts_launchpad.h"

void setup() {
  tsLaunchpadBegin();
}

void loop() {
  // No delay(): the modem needs to be serviced continuously.
  tsLaunchpadLoop();
}
