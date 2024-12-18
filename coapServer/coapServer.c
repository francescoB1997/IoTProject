#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "contiki.h"
#include "contiki-net.h"
#include "coap-engine.h"
#include "coap-blocking-api.h"
#include "os/dev/button-hal.h"
#include "os/dev/leds.h"
#include "net/ipv6/simple-udp.h"

#define UDP_CLIENT_PORT	8765
#define UDP_SERVER_PORT	5678
#define SEARCH_INTERVAL		  (1 * CLOCK_SECOND)
#define COAP_INTERVAL         (5 * CLOCK_SECOND)

PROCESS(coapServer, "coapServer");
AUTOSTART_PROCESSES(&coapServer);

static struct etimer periodic_timer;
static bool ledState = true;
//extern static bool pumpState;
//static bool pumpState = false;

extern coap_resource_t resPumpObs;  //Risorsa osservabile--> Stato della pompa sommersa
extern coap_resource_t res_led;

void client_chunk_handler(coap_message_t *response)
{
  const uint8_t *chunk;

  if(response == NULL) {
    puts("Request timed out");
    return;
  }

  int len = coap_get_payload(response, &chunk);

  printf("COAP MSG: | %.*s | \n", len, (char *)chunk);
  if(ledState)
    leds_on(LEDS_NUM_TO_MASK(LEDS_GREEN));
  else
    leds_off(LEDS_NUM_TO_MASK(LEDS_GREEN));
  ledState = !ledState;
}

PROCESS_THREAD(coapServer, ev, data)
{
  static struct simple_udp_connection udp_conn;
  //static struct fakeMoistureSensor fMoistSens_1;
  static uip_ipaddr_t BRIp;
  static bool X = true;
  button_hal_button_t *btn;
  PROCESS_BEGIN();
  printf("CoapServer\n");
  simple_udp_register(&udp_conn, UDP_CLIENT_PORT, NULL,UDP_SERVER_PORT, NULL);
  etimer_set(&periodic_timer, SEARCH_INTERVAL);
  
  btn = button_hal_get_by_index(0);

  while(X)
  {
     PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));
     if(NETSTACK_ROUTING.node_is_reachable() && NETSTACK_ROUTING.get_root_ipaddr(&BRIp))
     {
        char str[] = {"Hello, i'm a CoapServer\0"};
        simple_udp_sendto(&udp_conn, str, strlen(str), &BRIp);
       
        X = false;
        continue;
     }
     printf("Ricerca...\n");
     if(ledState)
        leds_single_on(LEDS_YELLOW);
     else
        leds_single_off(LEDS_YELLOW);
     ledState = !ledState;
     
     etimer_set(&periodic_timer, SEARCH_INTERVAL);
  }
  leds_single_off(LEDS_YELLOW);
  printf("Border Router trovato\n");

  coap_activate_resource(&resPumpObs, "pumpState"); // Nome della risorsa da inserire nella URI

  while(true)
  {
    printf("dormo\n");
    PROCESS_WAIT_EVENT();
    if(ev == button_hal_press_event)
    {
        btn = (button_hal_button_t *)data;
        printf("%s", BUTTON_HAL_GET_DESCRIPTION(btn));
        resPumpObs.trigger();   //Da invocare ogni volta che la pompa cambia stato
    }
    //etimer_set(&periodic_timer, COAP_INTERVAL );
  }
  printf("PROCESS_END()\n");
  PROCESS_END();
}
