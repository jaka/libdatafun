#include <stdio.h>
#include "datafun.h"

/*
  data.sparkfun.com auth data

Public URL: http://data.sparkfun.com/streams/XGlw6YJlj2ho6mYDZ45O
Public Key: XGlw6YJlj2ho6mYDZ45O
Private Key: 1J6EwvZ6dbcm49bWP52o
Delete Key: 7ylE7Y8lV2HJRWn9lryA
*/
const char publicKey[] = "XGlw6YJlj2ho6mYDZ45O";
const char privateKey[] = "1J6EwvZ6dbcm49bWP52o";
const char hostname[] = "data.sparkfun.com";
const char port[] = "80";

int main(int argc, char **argv) {

  int response;
  char *request = NULL;
  struct ldf_field *fields = NULL;

  /* Set demo "temp" fields. */
  ldf_add_field(&fields, "indoor_temp", "27.65");
  ldf_add_field(&fields, "outdoor_temp", "33.53");

  /* Update second field. */
  ldf_update_field(&fields, "outdoor_temp", "31.52");

  /* Send data to server. */
  ldf_create_request(&request, publicKey, privateKey, fields);
  printf("Sending request %s.\n", request);
  response = ldf_send_request(request, hostname, port);
  free(request);

  /* Free fields. */
  ldf_free_fields(fields);

  if ( response == 1 ) {
    printf("OK.\n");
  } else {
    fprintf(stderr, "An error happened.\n");
  }

  return response;
}
