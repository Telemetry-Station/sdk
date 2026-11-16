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
#include "app_payload.h"
#include "telemetry_payload.h"
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

    sim_on_event(demo->sim, event, detail);
    printf("[%6lu ms] %-22s detail=%ld\n", (unsigned long)demo->sim->now,
           ts_event_name(event), (long)detail);

    if (event == TS_EVENT_IMEI)
        puts("Module ID: " SIM_IMEI " (fictional; do not activate)");
    if (event == TS_EVENT_READY) puts("Module started. Preparing the sample.");
    if (event == TS_EVENT_MO_ACCEPTED) puts("Message queued by the simulated modem.");
    if (event == TS_EVENT_MO_DELIVERED) puts("Simulated delivery confirmed.");
    if (event == TS_EVENT_MO_DELIVERED) demo->delivered = true;
    if (event == TS_EVENT_STOPPED) demo->stopped = true;
}

int main(void)
{
    static ts_bear9704_t bear;
    static sim_modem_t sim;
    demo_t demo = {&sim, &bear, false, false};
    ts_platform_t port;
    const app_reading_t sample = {0, 215}; /* Same synthetic sample as the F169. */
    telemetry_payload_values_t decoded = {0};
    uint8_t reading[TELEMETRY_PAYLOAD_MAX_BYTES];
    size_t bytes, bits, i;
    bool sent = false, shutting_down = false;
    int step;

    sim_init(&sim);
    sim.topic = DEMO_TOPIC;
    port = sim_platform(&sim);
    if (ts_bear_init(&bear, &port, on_event, &demo) != TS_OK ||
        !app_payload_encode(&sample, reading, sizeof reading, &bytes, &bits)) return 1;

    printf("Telemetry Station SDK %s, simulated BEAR Module 9704\n", TS_SDK_VERSION_STRING);
    printf("Driver state: %lu bytes of RAM on this computer (see docs/DESIGN.md)\n\n",
           (unsigned long)sizeof bear);
    puts("No hardware, account or airtime is used. All delivery events are simulated.");
    puts("Sample sensor: 21.5 C (example data, not a live sensor). Sending once.");
    printf("Sample payload (without transport CRC): ");
    for (i = 0; i < bytes; ++i) printf("%02x", (unsigned)reading[i]);
    puts("");
    if (ts_bear_start(&bear) != TS_OK) return 1;
    sim.booted = true; /* The real module raises BOOTED on its own. */

    for (step = 0; step < 20000 && !demo.stopped; ++step) {
        sim_run(&bear, &sim, 1);
        if (!sent && ts_bear_can_send(&bear) &&
            ts_bear_send(&bear, DEMO_TOPIC, reading, bytes) == TS_OK)
            sent = true;
        if (demo.delivered && !shutting_down && ts_bear_shutdown(&bear) == TS_OK)
            shutting_down = true;
    }
    if (!demo.stopped) {
        puts("Simulation did not finish.");
        return 1;
    }
    if (!demo.delivered || !telemetry_payload_decode(reading, bytes, &decoded) ||
        decoded.sequence != sample.sequence ||
        decoded.temperature_tenths_c != sample.temperature_tenths_c) return 1;
    puts("\nModule stopped. The simulated power-down is complete.");
    printf("Local decode preview: sample %lu, temperature %.1f C\n",
           (unsigned long)decoded.sequence, (double)decoded.temperature_tenths_c / 10.0);
    puts("This preview decodes the local sample; it is not a cloud receipt.");
    puts("Next: follow docs/GETTING_STARTED.md, step 3, to check hardware support.");
    return 0;
}
