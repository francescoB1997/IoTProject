#ifndef PROJECT_CONF_H_
#define PROJECT_CONF_H_

/* Enable TCP */
#define UIP_CONF_TCP 1

#define MQTT_CLIENT_CONF_WITH_IBM_WATSON 0

/* MQTT broker address. */
#define MQTT_CLIENT_BROKER_IP_ADDR "fd00::1"

#define UDP_CLIENT_PORT	8765
#define UDP_SERVER_PORT	5678

#define DEFAULT_BROKER_PORT         1883
#define DEFAULT_PUBLISH_INTERVAL    (30 * CLOCK_SECOND)

#define STATE_INIT    		  0
#define STATE_NET_OK    	  1
#define STATE_CONNECTING      2
#define STATE_CONNECTED       3
#define STATE_SUBSCRIBED      4
#define STATE_DISCONNECTED    5

/* Maximum TCP segment size for outgoing segments of our socket */
#define MAX_TCP_SEGMENT_SIZE    32
#define CONFIG_IP_ADDR_STR_LEN   64

#define BUFFER_SIZE 64

#define STATE_MACHINE_PERIODIC     (CLOCK_SECOND >> 1)
#define SEARCH_INTERVAL		  (1 * CLOCK_SECOND)
#define PUBLISH_PERIOD        (3 * CLOCK_SECOND)

#define APP_BUFFER_SIZE 128

#endif /* PROJECT_CONF_H_ */
