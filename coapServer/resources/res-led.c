#include "coap-engine.h"
#include "os/dev/leds.h"
#include "stdio.h"

static void res_get_handler (coap_message_t *, coap_message_t *, uint8_t *, uint16_t , int32_t *);
static void res_put_handler(coap_message_t *, coap_message_t *, uint8_t *, uint16_t , int32_t *);

RESOURCE(res_led,
    "title=\"ledState\";rt=\"Text\"",
    res_get_handler, // GET Handler
    NULL, // POST Handler
    res_put_handler, // PUT Handler
    NULL); // DELETE Handler

static int ledState = 0;

static void res_get_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset) 
{
  printf("DBG: res_get_handlerLED\n");
  /* Some data that has the length up to REST_MAX_CHUNK_SIZE. For more, see the chunk resource. */
  char message[20] = {0};
  sprintf(message, "ledState: %d", ledState);
  int length = strlen(message);
  memcpy(buffer, message, length);
  
  coap_set_header_content_format(response, TEXT_PLAIN); /* text/plain is the default, hence this option could be omitted. */
  coap_set_header_etag(response, (uint8_t *)&length, 1);
  coap_set_payload(response, buffer, length);
}

static void res_put_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset)
{
  const char *value = NULL;
  char message[5] = { 0 };
  int messageLen, newState;
  coap_get_post_variable(request, "led", &value);
  if(!value)
    return;
    
  printf("DBG: res_put_handlerLED\n");
  if(!value)
    return;
    
  newState = atoi(value);
  if(!newState)
    leds_single_off(LEDS_YELLOW);
  else
    leds_single_on(LEDS_YELLOW);
  ledState = newState;
  sprintf(message, "%s", "OK");
  printf("DBG: %s\n", message);
  messageLen = strlen(message);
  memcpy(buffer, message, messageLen);
  coap_set_header_content_format(response, TEXT_PLAIN); /* text/plain is the default, hence this option could be omitted. */
  coap_set_header_etag(response, (uint8_t *)&messageLen, 1);
  coap_set_payload(response, buffer, messageLen);
}
