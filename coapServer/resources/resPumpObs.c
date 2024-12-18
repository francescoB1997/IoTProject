#include "coap-engine.h"
#include "os/dev/leds.h"
#include "stdio.h"

static void res_get_handler (coap_message_t *, coap_message_t *, uint8_t *, uint16_t , int32_t *);
static void res_put_handler (coap_message_t *, coap_message_t *, uint8_t *, uint16_t , int32_t *);
static void res_pumpObs_event_handler(void);

static bool pumpState = false;

EVENT_RESOURCE(resPumpObs, "title=\"resPumpObs\";rt=\"Text\";obs", 
                           res_get_handler, // GET Handler
                           NULL, // POST Handler
                           res_put_handler, // PUT Handler
                           NULL, // DELETE Handler
                           res_pumpObs_event_handler); //Tale handler verra' invocato ogni volta che lo stato interno della risorsa verra' modificato

static int counter = 0;
static void res_get_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset) 
{
  printf("DBG: res_get_handlerOBS\n");
  char message[40] = { 0 }; //{"name":"pumpState","value":"ON"}
  sprintf(message, "{\"name\":\"pumpState\",\"value\":\"%s\"}", (pumpState) ? "ON" : "OFF");
  int length = strlen(message);
  memcpy(buffer, message, length);
  
  coap_set_header_content_format(response, TEXT_PLAIN); /* text/plain is the default, hence this option could be omitted. */
  coap_set_header_etag(response, (uint8_t *)&length, 1);
  coap_set_payload(response, buffer, length);
}

static void res_put_handler (coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset)
{
    const char *payload = NULL;
    //coap_get_post_variable(request, "state", &value); //{"name":"state","value":"ON"}
    coap_get_post_variable(request, "payload", &payload);
    if(payload != NULL )
    {
        char *temp_payload = malloc(strlen(payload));
        memset(temp_payload, '\0', strlen(payload));
        sprintf(temp_payload, "%s",payload);
        printf("Payload--> %s\n", temp_payload);
        char *p = strtok(temp_payload, ":");
        printf("strtok1--> %s\n", p);
        p = strtok(NULL, ":");
        printf("strtok2--> %s\n", p);
        p = strtok(NULL, "\"");
        printf("strtok3--> %s\n", p);
        if(!strncmp(p,"ON",2))
        {
            leds_on(LEDS_GREEN);
            pumpState = true;
        }
        else if(!strncmp(p,"OFF", 3))
        {
            leds_off(LEDS_GREEN);
            pumpState = false;
        }    
        else
        {
            printf("Error pumpState value --> %s\n", temp_payload);
            return;
        }
        resPumpObs.trigger();
    }
    else
        printf("DBG: Handler put error\n");
}

static void res_pumpObs_event_handler(void)
{
    printf("DBG: res_ledObs_event_handler\n");
    counter++;
    coap_notify_observers(&resPumpObs);
}
