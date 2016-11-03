/* libdatafun C header */

#ifndef LIBDATAFUN_H
#define LIBDATAFUN_H

#include <ctype.h>	/* isspace() */
#include <stdlib.h>	/* malloc() */
#include <string.h>	/* strlen() */
#include <unistd.h>	/* close() */
#include <netdb.h>	/* getaddrinfo() */
#include <netinet/in.h>	/* */
#include <sys/socket.h>	/* socket() */
#include <sys/types.h>

/* libdatafun data fields (stream columns) */
struct ldf_field {
  char* name;
  char* value;
  struct ldf_field *next;
};

#define LDF_EHOSTNOTFOUND -1
#define LDF_ECONNECTFAIL -2
#define LDF_ECANNOTSEND -3

#define LDF_BREAKINGSTRING "\r\n\r\n"
#define LDF_BREAKINGSTRING_LENGTH (sizeof(LDF_BREAKINGSTRING) - 1)

void ldf_add_field(struct ldf_field **fields, const char *name, const char *value);
void ldf_update_field(struct ldf_field **, const char *, const char *);
int ldf_remove_field(struct ldf_field **fields, const char *name);
void ldf_free_fields(struct ldf_field *fields);

int ldf_create_request(char** request, const char* public_key, const char* private_key, struct ldf_field *fields);
int ldf_send_request(const char *request, const char *hostname, const char *port);

#endif
