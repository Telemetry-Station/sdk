/* SPDX-License-Identifier: MIT */
/* Runs the whole SDK workflow against a simulated modem. No hardware needed:
 *
 *   cmake -S . -B build && cmake --build build && ./build/simulated_demo
 *
 * It shows the sequence your firmware will see on a real BEAR Module 9704:
 * start, ready, IMEI (for activation), send, delivered, shut down, and the
 * moment it is safe to remove power. Times are simulated milliseconds.
 */
#include <stdio.h>

#include "sim_modem.h"
#include "ts_activation.h"
#include "ts_bear9704.h"

#define DEMO_TOPIC 1234u /* The simulator's topic. Yours comes from activation. */

typedef struct {
    sim_modem_t *sim;
    ts_bear9704_t *bear;
    bool delivered;
    bool stopped;
} demo_t;

static void on_event(void *user, ts_event_t event, int32_t detail)
{
    demo_t *demo = user;
    char text[256];

    sim_on_event(demo->sim, event, detail);
    printf("[%6lu ms] %-22s detail=%ld\n", (unsigned long)demo->sim->now,
           ts_event_name(event), (long)detail);

    if (event == TS_EVENT_IMEI &&
        ts_activation_text(ts_bear_imei(demo->bear), 4u, text, sizeof text))
        printf("\n%s\n", text);
    if (event == TS_EVENT_MO_DELIVERED) demo->delivered = true;
    if (event == TS_EVENT_STOPPED) demo->stopped = true;
}

int main(void)
{
    static ts_bear9704_t bear;
    static sim_modem_t sim;
    demo_t demo = {&sim, &bear, false, false};
    ts_platform_t port;
    const uint8_t reading[4] = {0x01, 0x02, 0x03, 0x04}; /* Your encoded payload. */
    bool sent = false, shutting_down = false;
    int step;

    sim_init(&sim);
    sim.topic = DEMO_TOPIC;
    port = sim_platform(&sim);
    ts_bear_init(&bear, &port, on_event, &demo);

    printf("Telemetry Station SDK %s, simulated BEAR Module 9704\n", TS_SDK_VERSION_STRING);
    printf("Driver state: %lu bytes of RAM on this computer (see docs/DESIGN.md)\n\n",
           (unsigned long)sizeof bear);
    ts_bear_start(&bear);
    sim.booted = true; /* The real module raises BOOTED on its own. */

    for (step = 0; step < 20000 && !demo.stopped; ++step) {
        sim_run(&bear, &sim, 1);
        if (!sent && ts_bear_send(&bear, DEMO_TOPIC, reading, sizeof reading) == TS_OK)
            sent = true;
        if (demo.delivered && !shutting_down && ts_bear_shutdown(&bear) == TS_OK)
            shutting_down = true;
    }
    if (!demo.stopped) {
        puts("Simulation did not finish.");
        return 1;
    }
    puts("\nBOOTED is low: it is now safe to remove power from the module.");
    return 0;
}
