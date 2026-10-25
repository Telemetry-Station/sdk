/* SPDX-License-Identifier: MIT */
#ifndef TS_LAUNCHPAD_H
#define TS_LAUNCHPAD_H

/*
 * The only part of the kit that depends on Arduino and Iridium's official
 * 9704 LaunchPad library. It reads the board, asks ts_mo_tracker and
 * app_payload what to do, and talks to the modem.
 */
void tsLaunchpadBegin();
void tsLaunchpadLoop();

#endif
