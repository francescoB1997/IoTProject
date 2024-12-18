#include "contiki.h"
#include "net/routing/routing.h"
#include "mqtt.h"
#include "net/ipv6/uip.h"
#include "net/ipv6/uip-icmp6.h"
#include "net/ipv6/sicslowpan.h"
#include "sys/etimer.h"
#include "sys/ctimer.h"
#include "lib/sensors.h"
#include "dev/button-hal.h"
#include "dev/leds.h"
#include "mqtt-client.h"
#include "fakeMoistureSensor.h"
#include <string.h>
#include <strings.h>

static const char *broker_ip = MQTT_CLIENT_BROKER_IP_ADDR;

static uint8_t state;

PROCESS_NAME(mqtt_client_process);
AUTOSTART_PROCESSES(&mqtt_client_process);

static char client_id[BUFFER_SIZE];
static char pub_topic[] = { "viaNapoli/data" }; //In questo topic verranno pubblicate info circa MoistureValue e pumpState
static char sub_topic[] = { "viaNapoli/commands" }; //In questo topic il nodo riceverà i comandi --> modifica del pumpState, moficia della modalita di irrigazione, modifica delle soglie Dry e Wet.
static bool ledState = false;

static mqtt_status_t status;
static char broker_address[CONFIG_IP_ADDR_STR_LEN];

static struct etimer periodic_timer;
static struct etimer publishTimer;
static struct fakeMoistureSensor fMoistSens_1;
static bool notifiedDry = false;
static bool notifiedWet = false;
static bool irrigMode = true;    // true --> AUTO, false --> MAN

static char app_buffer[APP_BUFFER_SIZE];

static struct mqtt_message *msg_ptr = 0;

static struct mqtt_connection conn;
static uip_ipaddr_t BRIp;

PROCESS(mqtt_client_process, "MQTT Client");

void changeIrrigationMode(bool newMode)
{
    notifiedDry = false;
    notifiedWet = false;
    if(newMode)
    {
        leds_off(LEDS_RED);
        //Prima stavano qua, prova adesso se fa che si blocca quando pubblica AUTO
    }
    else
        leds_on(LEDS_RED);
    irrigMode = newMode;
    printf("NewIrrigationMode--> %s\n", (irrigMode) ? "AUTO" : "MAN");
}

void changeMinThreshold(uint8_t newThreshold)
{
    fMoistSens_1.dryTreshold = newThreshold;
    printf("NewMinThreshold--> %d\n", fMoistSens_1.dryTreshold);
}

void changeMaxThreshold(uint8_t newThreshold)
{
    fMoistSens_1.wetTreshold = newThreshold;
    printf("NewMaxThreshold--> %d\n", fMoistSens_1.wetTreshold);
}

static void pub_handler(const char *topic, uint16_t topic_len, const uint8_t *chunk, uint16_t chunk_len) //{"name":"newThresholdMin","value":"15"}
{
  printf("Pub Handler: topic='%s' (len=%u), chunk_len=%u\n", topic, topic_len, chunk_len);
  printf("Chunk--> %s\n", (char *)chunk);
  
  //if( (topic_len == strlen("viaNapoli/commands/irrigMode")) && (!strcmp(topic, "viaNapoli/commands/irrigMode")) ) 
  //{
  if( ((char)chunk[chunk_len] == '\0') ) //Controllo che il vettore sia Null-terminated, perchè potrei avere OverRead a casua della strlen e strstr
  {
    if((strlen((char *)chunk) >= strlen("irrigMode")) && (strstr((char *)chunk, "irrigMode")) ) //Security check
    {
        if( (strstr((char *)chunk, "AUTO")) ) //if((strlen((char *)chunk) == strlen("AUTO")) && (!strcmp((char *)chunk, "AUTO")) )
        {
            changeIrrigationMode(true);
            //leds_off(LEDS_RED);
        }
        else if(strstr((char *)chunk, "MAN"))
        {
            changeIrrigationMode(false);
        }
        else
            printf("strstr irrigazione fallita\n");
     }
     else if((strlen((char *)chunk) >= strlen("pumpState")) && (strstr((char *)chunk, "pumpState")) )  //(topic_len == strlen("viaNapoli/commands/pumpState")) && (!strcmp(topic, "viaNapoli/commands/pumpState")) )
     {
        if( (strstr((char *)chunk, "ON")) ) 
        {
            notifiedDry = true;//?
            notifiedWet = false; //?
            changePumpState(true);
            
        }
        else if( strstr((char *)chunk, "OFF")) 
        {
            notifiedWet = true; //?
            notifiedDry = false; //?
            changePumpState(false);
        }
        else
            printf("strstr pumpState fallita\n");
     }
     else if((strlen((char *)chunk) >= strlen("newThresholdMin")) && (strstr((char *)chunk, "newThresholdMin")))
     {
        //{"name":"newThresholdMin","value":"55"}
        char *p = strtok((char *)chunk, ":");
        p = strtok(NULL, ":");
        p = strtok(NULL, "\"");
        
        uint8_t newThreshold = atoi((char *)p);
        changeMinThreshold(newThreshold);   ////Il check della nuova soglia viene svolto dall'applicazione
     }
     else if((strlen((char *)chunk) >= strlen("newThresholdMax")) && (strstr((char *)chunk, "newThresholdMax")))
     {
        char *p = strtok((char *)chunk, ":");
        p = strtok(NULL, ":");
        p = strtok(NULL, "\"");
        
        printf("p value-> %s\n", p);
        
        uint8_t newThreshold = atoi((char *)p);
        changeMaxThreshold(newThreshold);   //Il check della nuova soglia viene svolto dall'applicazione
     }
     else
        printf("chunk failed\n");
    }
    else
        printf("chunk non null-terminated\n");
}

static void mqtt_event(struct mqtt_connection *m, mqtt_event_t event, void *data)
{
  switch(event) {
  case MQTT_EVENT_CONNECTED: {
    printf("Application has a MQTT connection\n");

    state = STATE_CONNECTED;
    break;
  }
  case MQTT_EVENT_DISCONNECTED: {
    printf("MQTT Disconnect. Reason %u\n", *((mqtt_event_t *)data));

    state = STATE_DISCONNECTED;
    process_poll(&mqtt_client_process);
    break;
  }
  case MQTT_EVENT_PUBLISH: {
    msg_ptr = data;

    pub_handler(msg_ptr->topic, strlen(msg_ptr->topic),
                msg_ptr->payload_chunk, msg_ptr->payload_length);
    break;
  }
  case MQTT_EVENT_SUBACK: {
#if MQTT_311
    mqtt_suback_event_t *suback_event = (mqtt_suback_event_t *)data;

    if(suback_event->success) {
      printf("Application is subscribed to topic successfully\n");
    } else {
      printf("Application failed to subscribe to topic (ret code %x)\n", suback_event->return_code);
    }
#else
    printf("Application is subscribed to topic successfully\n");
#endif
    break;
  }
  case MQTT_EVENT_UNSUBACK: {
    printf("Application is unsubscribed to topic successfully\n");
    break;
  }
  case MQTT_EVENT_PUBACK: {
    printf("Publishing complete.*********************\n");
    break;
  }
  default:
    printf("Application got a unhandled MQTT event: %i\n", event);
    break;
  }
}

static bool have_connectivity(void)
{
  if(uip_ds6_get_global(ADDR_PREFERRED) == NULL || uip_ds6_defrt_choose() == NULL)
    return false;
  return true;
}

void inizializeSensor()
{
    fMoistSens_1.sensingValue = DEFAULT_SENSING_VALUE;
    fMoistSens_1.wetTreshold = DEFAULT_WET_TRESHOLD;
    fMoistSens_1.dryTreshold = DEFAULT_DRY_TRESHOLD;
    fMoistSens_1.pumpState = false;
}

void checkMoisture() // Il nodo che ha il sensore di umidità, può publicare il comando per attivare o meno la pompa.
{
    if( (fMoistSens_1.pumpState) && (fMoistSens_1.sensingValue >= fMoistSens_1.wetTreshold) )
    {
       if(!notifiedWet)
       {
          memset(app_buffer, '\0', APP_BUFFER_SIZE);
          sprintf(app_buffer, "{\"name\":\"pumpState\",\"value\":\"OFF\"}");
          printf("pubblicato nel topic %s --> %s \n", pub_topic, app_buffer);
          mqtt_publish(&conn, NULL, pub_topic, (uint8_t *)app_buffer, strlen(app_buffer), MQTT_QOS_LEVEL_0, MQTT_RETAIN_OFF);
          //notifiedWet = true;
          notifiedDry = false;
         // changePumpState(false);
       }
    }
    else if( (!fMoistSens_1.pumpState) && (fMoistSens_1.sensingValue <= fMoistSens_1.dryTreshold) )
    {
        if(!notifiedDry)
        {
            memset(app_buffer, '\0', APP_BUFFER_SIZE);
            sprintf(app_buffer, "{\"name\":\"pumpState\",\"value\":\"ON\"}");
            printf("pubblicato nel topic %s --> %s \n", pub_topic, app_buffer);
            mqtt_publish(&conn, NULL, pub_topic, (uint8_t *)app_buffer, strlen(app_buffer), MQTT_QOS_LEVEL_0, MQTT_RETAIN_OFF);
            //notifiedDry = true;
            notifiedWet = false;
           //changePumpState(true);
        }
    }
    //else
        //L'impianto rimane nello stato in cui si trova
}

void changePumpState(bool newState)
{
    fMoistSens_1.pumpState = newState;
    printf("NewPumpState--> %s\n", (fMoistSens_1.pumpState) ? "ON" : "OFF");
}

void changeSensigvalue() //Simulazione asciugatura / irrigazione terreno
{
    if(fMoistSens_1.pumpState && fMoistSens_1.sensingValue < MAX_MOISTURE_VALUE)
        fMoistSens_1.sensingValue += 2;
    else if( (!fMoistSens_1.pumpState) && (fMoistSens_1.sensingValue > MIN_MOISTURE_VALUE) && (fMoistSens_1.sensingValue > 2))
        fMoistSens_1.sensingValue -= 2;
    
    if(irrigMode)
        checkMoisture();
}

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(mqtt_client_process, ev, data)
{
  static struct simple_udp_connection udp_conn;
  button_hal_button_t *btn;
  PROCESS_BEGIN();
  btn = button_hal_get_by_index(0);
  printf("MoistureNode\n");
  // Initialize the ClientID as MAC address
  snprintf(client_id, BUFFER_SIZE, "%02x%02x%02x%02x%02x%02x",
                     linkaddr_node_addr.u8[0], linkaddr_node_addr.u8[1],
                     linkaddr_node_addr.u8[2], linkaddr_node_addr.u8[5],
                     linkaddr_node_addr.u8[6], linkaddr_node_addr.u8[7]);
  simple_udp_register(&udp_conn, UDP_CLIENT_PORT, NULL, UDP_SERVER_PORT, NULL);
  
  mqtt_register(&conn, &mqtt_client_process, client_id, mqtt_event, MAX_TCP_SEGMENT_SIZE);
  state=STATE_INIT;
  etimer_set(&periodic_timer, SEARCH_INTERVAL);
  
  while(true)
  {
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));
    if(NETSTACK_ROUTING.node_is_reachable() && NETSTACK_ROUTING.get_root_ipaddr(&BRIp))
    {
      char str[] = {"Hi, i'm the node with Moisture sensor"};
      simple_udp_sendto(&udp_conn, str, strlen(str), &BRIp);
      printf("BR trovato\n");
      break;
    }
    printf("Ricerca BR...\n");
    if(ledState)
      leds_on(LEDS_NUM_TO_MASK(LEDS_YELLOW));
    else
      leds_off(LEDS_NUM_TO_MASK(LEDS_YELLOW));
    ledState = !ledState;
      etimer_set(&periodic_timer, SEARCH_INTERVAL );
  }
  leds_off(LEDS_NUM_TO_MASK(LEDS_YELLOW));
  
  inizializeSensor(&fMoistSens_1);
  
  etimer_set(&periodic_timer, STATE_MACHINE_PERIODIC);
  etimer_set(&publishTimer, PUBLISH_PERIOD);
  etimer_set(&fMoistSens_1.sensingTimer, MOISTURE_SENSING_PERIOD);
    
  while(true)
  {
    PROCESS_WAIT_EVENT();
    if( (ev == PROCESS_EVENT_TIMER && data == &periodic_timer) || ev == PROCESS_EVENT_POLL )
    {
        if(state==STATE_INIT)
        {
            if(have_connectivity()==true)  
			    state = STATE_NET_OK;
	        else
	            printf("No connessione\n");
		}		  
		if(state == STATE_NET_OK)
		{
		    // Connect to MQTT server
			printf("Connecting!\n");  
			memcpy(broker_address, broker_ip, strlen(broker_ip));
			mqtt_connect(&conn, broker_address, DEFAULT_BROKER_PORT, (DEFAULT_PUBLISH_INTERVAL * 3) / CLOCK_SECOND, MQTT_CLEAN_SESSION_ON);
			state = STATE_CONNECTING;
		}
		if(state==STATE_CONNECTED)
		{
		    status = mqtt_subscribe(&conn, NULL, sub_topic, MQTT_QOS_LEVEL_0); //Sottoscrizione a tutti i topic che riguardano questo terreno
			printf("Subscribing ...\n");
			if(status == MQTT_STATUS_OUT_QUEUE_FULL)
			{
		        printf("Tried to subscribe but command queue was full!\n");
				PROCESS_EXIT();
			}  
		    state = STATE_SUBSCRIBED;
		}
		else if ( state == STATE_DISCONNECTED )
		{
		    state = STATE_INIT;
		    printf("Disconnected form MQTT broker\n");
		}
		etimer_set(&periodic_timer, STATE_MACHINE_PERIODIC);
    }
    else if (ev == PROCESS_EVENT_TIMER && data == &fMoistSens_1.sensingTimer) //if(etimer_expired(&fMoistSens_1.sensingTimer))
    {
        changeSensigvalue();
        etimer_reset(&fMoistSens_1.sensingTimer);
    }
    else if(ev == PROCESS_EVENT_TIMER && data == &publishTimer)//(etimer_expired(&publishTimer))
    {
        memset(app_buffer, '\0', APP_BUFFER_SIZE);
        sprintf(app_buffer, "{\"name\":\"moisture\",\"value\":\"%d\"}", fMoistSens_1.sensingValue);
        printf("publicato nel topic %s --> %s \n", pub_topic, app_buffer);
        mqtt_publish(&conn, NULL, pub_topic, (uint8_t *)app_buffer, strlen(app_buffer), MQTT_QOS_LEVEL_0, MQTT_RETAIN_OFF);
        etimer_reset(&publishTimer);
    }
     else if(ev == button_hal_press_event)
    {
        memset(app_buffer, 0, APP_BUFFER_SIZE);
        btn = (button_hal_button_t *)data;
        sprintf(app_buffer, "{\"name\":\"irrigMode\",\"value\":\"%s\"}", (!irrigMode) ? "AUTO" : "MAN");
        //sprintf(app_buffer, "{CIAO}");
        //printf("testo %s\n", app_buffer);
        mqtt_publish(&conn, NULL, pub_topic, (uint8_t *)app_buffer, strlen(app_buffer), MQTT_QOS_LEVEL_0, MQTT_RETAIN_OFF);
        printf("bottonePremuto: %s\n", BUTTON_HAL_GET_DESCRIPTION(btn));
        changeIrrigationMode(!irrigMode);
    }
  }
  PROCESS_END();
}

