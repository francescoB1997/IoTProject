#include "contiki.h"
#include "net/routing/routing.h"
#include "net/netstack.h"
#include "net/ipv6/simple-udp.h"
#include "os/sys/etimer.h"
#include "uiplib.h"
/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_INFO

#define WITH_SERVER_REPLY  1
#define UDP_CLIENT_PORT	8765
#define UDP_SERVER_PORT	5678

//#define PERIOD (2 * CLOCK_SECOND)

static struct simple_udp_connection udp_conn;
//static struct etimer mioTimer;

/* Declare and auto-start this file's process */
PROCESS(contiki_ng_br, "Contiki-NG Border Router");
AUTOSTART_PROCESSES(&contiki_ng_br);

/****************                         La funziona sotto, fa parte del codice del RPL Server*/

static void udp_rx_callback(struct simple_udp_connection *c,
         const uip_ipaddr_t *sender_addr,
         uint16_t sender_port,
         const uip_ipaddr_t *receiver_addr,
         uint16_t receiver_port,
         const uint8_t *data,
         uint16_t datalen)
{
  printf("Received request '%.*s' from ", datalen, (char *) data);
  uiplib_ipaddr_print(sender_addr);
  printf("\n");
}

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(contiki_ng_br, ev, data)
{
  PROCESS_BEGIN();
  
  #if BORDER_ROUTER_CONF_WEBSERVER
  PROCESS_NAME(webserver_nogui_process);
  process_start(&webserver_nogui_process, NULL);
  #endif /* BORDER_ROUTER_CONF_WEBSERVER */

  printf("Contiki-NG Border Router started\n");

/* Initialize DAG root */
  NETSTACK_ROUTING.root_start();

  /* Initialize UDP connection */
  simple_udp_register(&udp_conn, UDP_SERVER_PORT, NULL, UDP_CLIENT_PORT, udp_rx_callback);
                      
  //etimer_set(&mioTimer, PERIOD);
  while(1)
  {
    PROCESS_YIELD();
  //  etimer_reset(&mioTimer);
  }

  PROCESS_END();
}
