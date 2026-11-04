/* SPDX-License-Identifier: MIT */
#include "ts_gnss.h"
#include <string.h>
bool ts_gnss_init(ts_gnss_t *g, const ts_gnss_port_t *port)
{
    if (!g || !port || !port->read_byte || !port->power) return false;
    memset(g, 0, sizeof(*g)); g->port = *port; return true;
}
#if TS_ENABLE_GNSS
static void finish(ts_gnss_t *g, ts_gnss_status_t result)
{
    g->status = g->port.power(g->port.user, false) ? result : TS_GNSS_IO_ERROR;
}
static uint32_t le32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}
static int32_t signed32(uint32_t n)
{
    return n <= INT32_MAX ? (int32_t)n : -1 - (int32_t)(UINT32_MAX - n);
}
static bool valid_frame(ts_gnss_t *g)
{
    uint8_t a=0,b=0,i; const uint8_t *p=g->frame+6;
    for(i=2;i<98;++i) { a=(uint8_t)(a+g->frame[i]); b=(uint8_t)(b+a); }
    if (a!=g->frame[98] || b!=g->frame[99] || p[20]!=3u || !(p[21]&1u)) return false;
    g->fix.longitude_e7=signed32(le32(p+24));
    g->fix.latitude_e7=signed32(le32(p+28));
    if(g->fix.latitude_e7 < -900000000L || g->fix.latitude_e7 > 900000000L ||
       g->fix.longitude_e7 < -1800000000L || g->fix.longitude_e7 > 1800000000L) {
        memset(&g->fix,0,sizeof(g->fix)); return false;
    }
    g->fix.horizontal_accuracy_mm=le32(p+40); g->fix.satellites=p[23];
    return true;
}
#endif
bool ts_gnss_begin(ts_gnss_t *g, uint32_t now, uint32_t budget_ms)
{
#if TS_ENABLE_GNSS
    if(!g || !g->port.power || !g->port.read_byte || g->status==TS_GNSS_ACQUIRING ||
       !budget_ms || budget_ms>INT32_MAX) return false;
    memset(&g->fix,0,sizeof(g->fix)); g->used=0;
    if(!g->port.power(g->port.user,true)) {
        finish(g,TS_GNSS_IO_ERROR); return false;
    }
    g->deadline=now+budget_ms; g->status=TS_GNSS_ACQUIRING; return true;
#else
    (void)g; (void)now; (void)budget_ms; return false;
#endif
}
void ts_gnss_cancel(ts_gnss_t *g)
{
#if TS_ENABLE_GNSS
    if(g && g->status==TS_GNSS_ACQUIRING) finish(g,TS_GNSS_OFF);
#else
    (void)g;
#endif
}
void ts_gnss_poll(ts_gnss_t *g,uint32_t now)
{
#if TS_ENABLE_GNSS
    unsigned int i;
    if(!g || g->status!=TS_GNSS_ACQUIRING) return;
    if((int32_t)(now-g->deadline)>=0) { finish(g,TS_GNSS_TIMEOUT); return; }
    for(i=0;i<32u;++i) { /* Cooperative byte budget; no wait for a whole frame. */
        uint8_t c; int n=g->port.read_byte(g->port.user,&c);
        if(n==0) return;
        if(n!=1) { finish(g,TS_GNSS_IO_ERROR); return; }
        g->frame[g->used++]=c;
        if((g->used==1 && c!=0xb5) || (g->used==2 && c!=0x62) ||
           (g->used==3 && c!=1) || (g->used==4 && c!=7) ||
           (g->used==5 && c!=92) || (g->used==6 && c!=0)) {
            g->used=(c==0xb5)?1u:0u; g->frame[0]=c;
        } else if(g->used==100) {
            g->used=0;
            if(valid_frame(g)) { finish(g,TS_GNSS_FIX); return; }
        }
    }
#else
    (void)g; (void)now;
#endif
}
