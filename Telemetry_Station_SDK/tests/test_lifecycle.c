/* Lifecycle and payload contracts. Host models do not establish electrical safety. */
#include <stdio.h>
#include <string.h>
#include "sim_modem.h"
#include "app_payload.h"
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"FAIL %s:%d: %s\n",__FILE__,__LINE__,#x); return 1; } } while (0)
static sim_modem_t modem;
static ts_bear9704_t bear;
static ts_platform_t base;
static bool uart, supply_ok=true;
static unsigned starts, stops, order_errors;
static void uart_enable(void *u,bool on) { if(on && !modem.booted) ++order_errors; uart=on; base.uart_enable(u,on); }
static bool power_start(void *u) { ++starts; if(!supply_ok)return false; return base.power_start(u); }
static void power_stop(void *u) { if(uart)++order_errors; ++stops; base.power_stop(u); }
static int init(void) {
 ts_platform_t p; sim_init(&modem);base=sim_platform(&modem);p=base;
 p.uart_enable=uart_enable;p.power_start=power_start;p.power_stop=power_stop;
 uart=false;starts=stops=order_errors=0;supply_ok=true;
 CHECK(ts_bear_init(&bear,&p,sim_on_event,&modem)==TS_OK);return 0;
}
static int ready(void) { CHECK(!init());CHECK(ts_bear_start(&bear)==TS_OK);modem.booted=true;sim_run(&bear,&modem,3000);CHECK(ts_bear_is_ready(&bear));return 0; }
int main(int argc,char **argv) {
 const char *name=argc>1?argv[1]:"";
 if(!strcmp(name,"delayed_boot")) {
  CHECK(!init());CHECK(ts_bear_start(&bear)==TS_OK);sim_run(&bear,&modem,29000);
  CHECK(!uart && !stops && !modem.sent_log[0]);modem.booted=true;sim_run(&bear,&modem,3000);CHECK(ts_bear_is_ready(&bear));CHECK(!order_errors);
 } else if(!strcmp(name,"incomplete_reply")) {
  CHECK(!init());sim_override(&modem,"GET apiVersion","200 apiVersion {\"active_version\":");
  CHECK(ts_bear_start(&bear)==TS_OK);modem.booted=true;sim_run(&bear,&modem,(int)TS_CMD_TIMEOUT_MS+20);
  CHECK(ts_bear_state(&bear)==TS_FAULT);CHECK(!sim_saw(&modem,TS_EVENT_READY));
 } else if(!strcmp(name,"interrupted_command")) {
  CHECK(!ready());CHECK(ts_bear_request_signal(&bear)==TS_OK);modem.write_error=1;sim_run(&bear,&modem,10);
  CHECK(ts_bear_state(&bear)==TS_FAULT);CHECK(!uart);CHECK(!order_errors);
  modem.write_error=0;modem.cmd_len=0;CHECK(ts_bear_start(&bear)==TS_OK);modem.booted=true;sim_run(&bear,&modem,3000);CHECK(ts_bear_is_ready(&bear));
 } else if(!strcmp(name,"unknown_delivery")) {
  uint8_t payload=1;char log[sizeof modem.sent_log];CHECK(!ready());modem.event_count=0;
  sim_override(&modem,"PUT messageOriginateSegment","");CHECK(ts_bear_send(&bear,1234,&payload,1)==TS_OK);
  sim_run(&bear,&modem,200);CHECK(sim_saw(&modem,TS_EVENT_MO_ACCEPTED));CHECK(!sim_saw(&modem,TS_EVENT_MO_DELIVERED));
  sim_run(&bear,&modem,(int)TS_MO_TIMEOUT_MS);CHECK(sim_saw(&modem,TS_EVENT_MO_FAILED));CHECK(!sim_saw(&modem,TS_EVENT_MO_DELIVERED));
  memcpy(log,modem.sent_log,sizeof log);sim_run(&bear,&modem,10000);CHECK(!memcmp(log,modem.sent_log,sizeof log));
 } else if(!strcmp(name,"shutdown_order")) {
  CHECK(!ready());modem.hold_booted=true;CHECK(ts_bear_shutdown(&bear)==TS_OK);sim_run(&bear,&modem,100);
  CHECK(strstr(modem.sent_log,"\"state\":\"inactive\""));CHECK(!uart && stops==1 && !order_errors);
  CHECK(!sim_saw(&modem,TS_EVENT_STOPPED));CHECK(ts_bear_start(&bear)==TS_ERR_BUSY);
  modem.booted=false;sim_run(&bear,&modem,2);CHECK(sim_saw(&modem,TS_EVENT_STOPPED));
 } else if(!strcmp(name,"power_cycles")) {
  unsigned i;CHECK(!init());for(i=0;i<100;i++) {
   modem.sent_log[0]=0;modem.event_count=0;CHECK(ts_bear_start(&bear)==TS_OK);modem.booted=true;
   sim_run(&bear,&modem,3000);CHECK(ts_bear_is_ready(&bear));CHECK(ts_bear_shutdown(&bear)==TS_OK);
   sim_run(&bear,&modem,100);CHECK(ts_bear_state(&bear)==TS_OFF && !uart && !order_errors);
  }CHECK(starts==100 && stops==100);
 } else if(!strcmp(name,"payload_topic")) {
  uint8_t data[TS_MAX_PAYLOAD+1]={0};CHECK(!ready());CHECK(ts_bear_send(&bear,0,data,1)==TS_ERR_CAPACITY);
  CHECK(ts_bear_send(&bear,63,data,1)==TS_ERR_CAPACITY);CHECK(ts_bear_send(&bear,1234,data,sizeof data)==TS_ERR_CAPACITY);
  CHECK(ts_bear_send(&bear,1234,data,TS_MAX_PAYLOAD)==TS_OK);
  printf("Host sizeof driver=%zu; target size/stack still NOT RUN\n",sizeof bear);
 } else if(!strcmp(name,"decoder_vector")) {
  app_reading_t r={0x1234,-123};uint8_t data[4];size_t n,bits;
  CHECK(app_payload_encode(&r,data,sizeof data,&n,&bits));CHECK(n==4 && bits==32);
  CHECK(data[0]==0x12 && data[1]==0x34 && data[2]==0xff && data[3]==0x85);
 } else if(!strcmp(name,"invalid_supply")) {
  CHECK(!init());supply_ok=false;CHECK(ts_bear_start(&bear)==TS_ERR_IO);CHECK(ts_bear_state(&bear)==TS_OFF && !uart);
 } else if(!strcmp(name,"unexpected_reset")) {
  CHECK(!ready());modem.booted=false;sim_run(&bear,&modem,100);
  CHECK(!ts_bear_is_ready(&bear));CHECK(!uart);
 } else if(!strcmp(name,"restart_booted_high")) {
  CHECK(!ready());modem.hold_booted=true;CHECK(ts_bear_shutdown(&bear)==TS_OK);
  sim_run(&bear,&modem,(int)TS_BOOT_TIMEOUT_MS+100);CHECK(ts_bear_state(&bear)==TS_FAULT && modem.booted);
  { unsigned old=starts;CHECK(ts_bear_start(&bear)!=TS_OK);CHECK(starts==old); }
 } else if(!strcmp(name,"boot_timeout_sequence")) {
  CHECK(!init());CHECK(ts_bear_start(&bear)==TS_OK);sim_run(&bear,&modem,(int)TS_BOOT_TIMEOUT_MS+100);
  CHECK(sim_saw(&modem,TS_EVENT_ERROR));CHECK(stops==0); /* Must not negate ENABLE before BOOTED rises. */
 } else {fprintf(stderr,"Unknown case\n");return 2;}
 printf("PASS %s (host model only)\n",name);return 0;
}
