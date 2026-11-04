#include "ts_gnss.h"
#include <assert.h>
#include <string.h>
static unsigned on,off,pos;
static uint8_t frame[106];
static unsigned length=100;
static int error;
static bool power_ok=true;
static bool power(void *u,bool enabled) { (void)u;if(enabled)++on;else ++off;return power_ok; }
static int read_byte(void *u,uint8_t *b) { (void)u;if(error)return -1;if(pos==length)return 0;*b=frame[pos++];return 1; }
#if TS_ENABLE_GNSS
static void packet(bool fix) {
 unsigned i;uint8_t a=0,b=0;memset(frame,0,sizeof frame);pos=0;length=100;
 frame[0]=0xb5;frame[1]=0x62;frame[2]=1;frame[3]=7;frame[4]=92;
 frame[26]=3;frame[27]=fix?1:0;frame[29]=8;frame[30]=1;frame[34]=2;
 for(i=2;i<98;++i){a=(uint8_t)(a+frame[i]);b=(uint8_t)(b+a);}frame[98]=a;frame[99]=b;
}
static void test_header_recovery(ts_gnss_t *g)
{
 uint8_t valid[100];
 packet(true);memcpy(valid,frame,sizeof valid);
 for(unsigned header=0;header<6;++header) {
  for(unsigned overlap=0;overlap<2;++overlap) {
   unsigned powered_off=off;
   if(header==0 && overlap)continue; /* 0xb5 is valid at the first position. */
   memcpy(frame,valid,header);frame[header]=overlap?0xb5:0xff;
   pos=0;length=header+1;
   assert(ts_gnss_begin(g,0,10));ts_gnss_poll(g,1);
   assert(pos==length && g->status==TS_GNSS_ACQUIRING && off==powered_off);
   assert(g->used==overlap && g->fix.latitude_e7==0 && g->fix.longitude_e7==0);
   if(overlap)assert(g->frame[0]==0xb5);
   memcpy(frame+length,valid+overlap,sizeof valid-overlap);
   length+=(unsigned)sizeof valid-overlap;
   for(unsigned i=0;i<4;++i)ts_gnss_poll(g,1);
   assert(pos==length && g->used==0 && g->status==TS_GNSS_FIX);
   assert(g->fix.longitude_e7==1 && g->fix.latitude_e7==2 && g->fix.satellites==8);
   assert(off==powered_off+1);
  }
 }
}
#endif
int main(void) {
 ts_gnss_t g;ts_gnss_port_t p={0,read_byte,power};
 assert(!ts_gnss_init(0,&p));assert(!ts_gnss_init(&g,0));assert(ts_gnss_init(&g,&p));
 assert(!ts_gnss_begin(&g,0,0));assert(!on);
#if TS_ENABLE_GNSS
 packet(true);assert(ts_gnss_begin(&g,UINT32_MAX-5u,20));assert(!ts_gnss_begin(&g,0,10));
 for(unsigned i=0;i<4;++i)ts_gnss_poll(&g,1);
 assert(g.status==TS_GNSS_FIX && g.fix.longitude_e7==1 && g.fix.latitude_e7==2 && off==1);
 packet(false);assert(ts_gnss_begin(&g,0,10));for(unsigned i=0;i<4;++i)ts_gnss_poll(&g,1);
 assert(g.status==TS_GNSS_ACQUIRING);ts_gnss_poll(&g,10);assert(g.status==TS_GNSS_TIMEOUT);
 packet(true);frame[99]^=1;assert(ts_gnss_begin(&g,0,10));for(unsigned i=0;i<4;++i)ts_gnss_poll(&g,1);
 assert(g.status==TS_GNSS_ACQUIRING);ts_gnss_cancel(&g);assert(g.status==TS_GNSS_OFF);
 error=1;assert(ts_gnss_begin(&g,0,10));ts_gnss_poll(&g,1);assert(g.status==TS_GNSS_IO_ERROR);
 assert(!ts_gnss_begin(&g,0,UINT32_MAX));
 error=0;packet(true); /* A checksummed but out-of-range latitude is not a fix. */
 frame[34]=0xff;frame[35]=0xff;frame[36]=0xff;frame[37]=0x7f;
 { uint8_t a=0,b=0;for(unsigned i=2;i<98;++i){a=(uint8_t)(a+frame[i]);b=(uint8_t)(b+a);}frame[98]=a;frame[99]=b; }
 assert(ts_gnss_begin(&g,0,10));for(unsigned i=0;i<4;++i)ts_gnss_poll(&g,1);
 assert(g.status==TS_GNSS_ACQUIRING && g.fix.latitude_e7==0);ts_gnss_cancel(&g);
 test_header_recovery(&g);
 power_ok=false;assert(!ts_gnss_begin(&g,0,10));assert(g.status==TS_GNSS_IO_ERROR);
#else
 assert(!ts_gnss_begin(&g,0,100));ts_gnss_poll(&g,0);ts_gnss_cancel(&g);assert(!on && !off);
#endif
 return 0;
}
