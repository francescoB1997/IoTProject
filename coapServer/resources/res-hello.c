#include "coap-engine.h"
#include "stdio.h"
#include "sys/log.h"
#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_INFO

static void res_get_handler (coap_message_t *, coap_message_t *, uint8_t *, uint16_t , int32_t *);

RESOURCE(res_hello,
    "title=\"Hello world\";rt=\"Text\"",
    res_get_handler, // GET Handler
    NULL, // POST Handler
    NULL, // PUT Handler
    NULL); // DELETE Handler

static void res_get_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset) 
{
    printf("res_get_handler\n");
    const char *len = NULL;
  /* Some data that has the length up to REST_MAX_CHUNK_SIZE. For more, see the chunk resource. */
  char const *const message = "Hello World by Funghinisss!";
  int length = 27; /*           |<-------->| */

  /* The query string can be retrieved by rest_get_query() or parsed for its key-value pairs. */
  if(coap_get_query_variable(request, "len", &len)) {
    length = atoi(len);
    if(length < 0) {
      length = 0;
    }
    if(length > REST_MAX_CHUNK_SIZE) {
      length = REST_MAX_CHUNK_SIZE;
    }
    memcpy(buffer, message, length);
  } else {
    memcpy(buffer, message, length);
  }
  LOG_INFO("res_get_handler: %s\n", message);
  coap_set_header_content_format(response, TEXT_PLAIN); /* text/plain is the default, hence this option could be omitted. */
  coap_set_header_etag(response, (uint8_t *)&length, 1);
  coap_set_payload(response, buffer, length);
}


