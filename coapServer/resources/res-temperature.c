#include "coap-engine.h"
#include "stdio.h"
static void res_get_handler (coap_message_t *, coap_message_t *, uint8_t *, uint16_t , int32_t *);

static void res_put_handler(coap_message_t *, coap_message_t *, uint8_t *, uint16_t , int32_t *);

static int i = 0;

RESOURCE(res_temperature,
    "title=\"Temperature\";rt=\"Text\"",
    res_get_handler, // GET Handler
    NULL, // POST Handler
    res_put_handler, // PUT Handler
    NULL); // DELETE Handler

static void res_get_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset) 
{
  printf("DBG: res_get_handlerTEMP\n");
  /* Some data that has the length up to REST_MAX_CHUNK_SIZE. For more, see the chunk resource. */
  char message[20] = {0};
  sprintf(message, "Temp: %d °", i);
  i++;
  int length = strlen(message);
  memcpy(buffer, message, length);
  
  coap_set_header_content_format(response, TEXT_PLAIN); /* text/plain is the default, hence this option could be omitted. */
  coap_set_header_etag(response, (uint8_t *)&length, 1);
  coap_set_payload(response, buffer, length);
}

static void res_put_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset) 
{
  const char *value = NULL;
  char message[10] = { 0 };
  int messageLen, newTemp;
  coap_get_post_variable(request, "newTemp", &value);
  if(!value)
    return;
    
  printf("DBG: res_put_handlerTEMP\n");
  newTemp = atoi(value);
  if(newTemp < 0)
    sprintf(message, "%s", "Errore");
  else
  {
    i = newTemp;
    sprintf(message, "%s", "OK");
  }
  printf("DBG: %s\n", message);
  messageLen = strlen(message);
  memcpy(buffer, message, messageLen);
  coap_set_header_content_format(response, TEXT_PLAIN); /* text/plain is the default, hence this option could be omitted. */
  coap_set_header_etag(response, (uint8_t *)&messageLen, 1);
  coap_set_payload(response, buffer, messageLen);
}
