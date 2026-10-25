/* SPDX-License-Identifier: MIT */
#include "ts_bear9704.h"

/* Names for logging only. Not linked unless you add this file to your build. */

const char *ts_state_name(ts_state_t state)
{
    switch (state) {
    case TS_OFF:            return "TS_OFF";
    case TS_BOOTING:        return "TS_BOOTING";
    case TS_API_QUERY:      return "TS_API_QUERY";
    case TS_API_SELECT:     return "TS_API_SELECT";
    case TS_SIM_QUERY:      return "TS_SIM_QUERY";
    case TS_SIM_SET:        return "TS_SIM_SET";
    case TS_MODE_QUERY:     return "TS_MODE_QUERY";
    case TS_MODE_SET:       return "TS_MODE_SET";
    case TS_READY:          return "TS_READY";
    case TS_MO_START:       return "TS_MO_START";
    case TS_MO_ACTIVE:      return "TS_MO_ACTIVE";
    case TS_SHUTDOWN_WAIT:  return "TS_SHUTDOWN_WAIT";
    case TS_POWERING_DOWN:  return "TS_POWERING_DOWN";
    case TS_FAULT:          return "TS_FAULT";
    }
    return "TS_STATE_UNKNOWN";
}

const char *ts_event_name(ts_event_t event)
{
    switch (event) {
    case TS_EVENT_READY:        return "TS_EVENT_READY";
    case TS_EVENT_MO_ACCEPTED:  return "TS_EVENT_MO_ACCEPTED";
    case TS_EVENT_MO_DELIVERED: return "TS_EVENT_MO_DELIVERED";
    case TS_EVENT_MO_FAILED:    return "TS_EVENT_MO_FAILED";
    case TS_EVENT_MT_RECEIVED:  return "TS_EVENT_MT_RECEIVED";
    case TS_EVENT_MT_DROPPED:   return "TS_EVENT_MT_DROPPED";
    case TS_EVENT_SIGNAL:       return "TS_EVENT_SIGNAL";
    case TS_EVENT_IMEI:         return "TS_EVENT_IMEI";
    case TS_EVENT_STOPPED:      return "TS_EVENT_STOPPED";
    case TS_EVENT_ERROR:        return "TS_EVENT_ERROR";
    }
    return "TS_EVENT_UNKNOWN";
}

const char *ts_result_name(ts_result_t result)
{
    switch (result) {
    case TS_OK:             return "TS_OK";
    case TS_ERR_ARGUMENT:   return "TS_ERR_ARGUMENT";
    case TS_ERR_BUSY:       return "TS_ERR_BUSY";
    case TS_ERR_CAPACITY:   return "TS_ERR_CAPACITY";
    case TS_ERR_IO:         return "TS_ERR_IO";
    case TS_ERR_PROTOCOL:   return "TS_ERR_PROTOCOL";
    case TS_ERR_TIMEOUT:    return "TS_ERR_TIMEOUT";
    case TS_ERR_REJECTED:   return "TS_ERR_REJECTED";
    case TS_ERR_NOT_READY:  return "TS_ERR_NOT_READY";
    }
    return "TS_ERR_UNKNOWN";
}
