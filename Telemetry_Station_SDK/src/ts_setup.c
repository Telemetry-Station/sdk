/* SPDX-License-Identifier: MIT */
#include "ts_internal.h"
#include <string.h>

/* Start-up handshake. After the modem boots we:
 *   1. pick an API version,
 *   2. select the internal SIM,
 *   3. switch the modem from "inactive" to "active".
 * Each step is a request whose reply moves us to the next state. */

static bool send_and_wait(ts_bear9704_t *ctx, const char *command,
                          ts_state_t next)
{
    if (!ts_cmd(ctx, command)) return false;
    ctx->state = next;
    return true;
}

static const char *awaited_target(ts_state_t state)
{
    switch (state) {
    case TS_API_QUERY:
    case TS_API_SELECT: return "apiVersion";
    case TS_SIM_QUERY:
    case TS_SIM_SET:    return "simConfig";
    default:            return "operationalState";
    }
}

static bool on_api_query(ts_bear9704_t *ctx, const ts_jspr_frame_t *f)
{
    uint32_t major, minor, patch;
    ts_cmd_t cmd;
    if (ts_json_version(f->json, "active_version", &major, &minor, &patch))
        return send_and_wait(ctx, "GET simConfig {}\r", TS_SIM_QUERY);
    /* No version active yet: choose the first one the modem supports. */
    if (!ts_json_first_version(f->json, &major, &minor, &patch)) return false;
    if (!ts_cmd_begin(ctx, &cmd)) return false;
    ts_cmd_str(&cmd, "PUT apiVersion {\"active_version\":{\"major\":");
    ts_cmd_u32(&cmd, major);
    ts_cmd_str(&cmd, ",\"minor\":");
    ts_cmd_u32(&cmd, minor);
    ts_cmd_str(&cmd, ",\"patch\":");
    ts_cmd_u32(&cmd, patch);
    ts_cmd_str(&cmd, "}}\r");
    if (!ts_cmd_end(ctx, &cmd)) return false;
    ctx->state = TS_API_SELECT;
    return true;
}

static bool on_sim_query(ts_bear9704_t *ctx, const ts_jspr_frame_t *f)
{
    char sim[24];
    if (ts_json_string(f->json, "interface", sim, sizeof(sim)) &&
        !strcmp(sim, "internal"))
        return send_and_wait(ctx, "GET operationalState {}\r", TS_MODE_QUERY);
    return send_and_wait(ctx, "PUT simConfig {\"interface\":\"internal\"}\r",
                         TS_SIM_SET);
}

static bool on_mode_query(ts_bear9704_t *ctx, const ts_jspr_frame_t *f)
{
    char mode[32];
    if (!ts_json_string(f->json, "state", mode, sizeof(mode))) return false;
    if (!strcmp(mode, "active")) {
        ts_set_ready(ctx);
        return true;
    }
    /* Any state other than "inactive" (such as a test mode) is left alone. */
    if (strcmp(mode, "inactive")) return false;
    return send_and_wait(ctx, "PUT operationalState {\"state\":\"active\"}\r",
                         TS_MODE_SET);
}

bool ts_setup_on_frame(ts_bear9704_t *ctx, const ts_jspr_frame_t *f)
{
    if (strcmp(f->target, awaited_target(ctx->state))) return true;
    if (f->code != TS_JSPR_OK) return false;

    switch (ctx->state) {
    case TS_API_QUERY:  return on_api_query(ctx, f);
    case TS_API_SELECT: return send_and_wait(ctx, "GET simConfig {}\r", TS_SIM_QUERY);
    case TS_SIM_QUERY:  return on_sim_query(ctx, f);
    case TS_SIM_SET:    return send_and_wait(ctx, "GET operationalState {}\r", TS_MODE_QUERY);
    case TS_MODE_QUERY: return on_mode_query(ctx, f);
    case TS_MODE_SET:   ts_set_ready(ctx); return true;
    default:            return true;
    }
}
