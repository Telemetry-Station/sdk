/* SPDX-License-Identifier: MIT */
#ifndef SIM_MODEM_H
#define SIM_MODEM_H

#include "ts_bear9704.h"

/* A simulated BEAR Module 9704 for host builds. It speaks the modem's serial
 * protocol, so the real driver runs unchanged against it. Used by the unit
 * tests and by examples/host/simulated_demo.c. */

#define SIM_IMEI "300234010753370"
#define SIM_MAX_EVENTS 32
#define SIM_MAX_RULES 4

typedef struct {
    uint32_t now;          /* Simulated milliseconds; advance with sim_run(). */
    bool booted;           /* The module's BOOTED signal. */
    bool hold_booted;      /* Keeps BOOTED high after power_stop. */
    uint32_t topic;        /* Topic echoed in replies. 1234 if left at 0. */
    char cmd[512];
    size_t cmd_len;
    char reply[1024];
    size_t reply_len, reply_pos;
    char sent_log[2048];   /* Everything the driver wrote. */
    ts_event_t events[SIM_MAX_EVENTS];
    int event_count;
    int32_t last_detail;

    /* Fault injection for tests. */
    int read_error;        /* 1: serial_read fails. */
    int write_error;       /* 1: serial_write fails. 2: reports too many bytes. */
    struct {
        const char *prefix;
        const char *reply; /* Sent instead of the normal reply. "" stays silent. */
    } rules[SIM_MAX_RULES];
    int rule_count;
} sim_modem_t;

void sim_init(sim_modem_t *sim);
ts_platform_t sim_platform(sim_modem_t *sim);

/* Event callback that records events into sim->events. */
void sim_on_event(void *user, ts_event_t event, int32_t detail);

/* Replaces the reply to every command that starts with prefix. */
void sim_override(sim_modem_t *sim, const char *prefix, const char *reply);

/* Queues a line as if the modem had sent it. */
void sim_queue(sim_modem_t *sim, const char *line);

/* Advances time one millisecond per step, polling the driver each step. */
void sim_run(ts_bear9704_t *bear, sim_modem_t *sim, int steps);

bool sim_saw(const sim_modem_t *sim, ts_event_t event);

#endif
