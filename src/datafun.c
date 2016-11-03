/* libdatafun C code */

#include "datafun.h"

static char to_hex(char code) {
  static const char hex[] = "0123456789abcdef";
  return hex[code & 0x0f];
}

static char *_ldf_trim_encode(const char* i_str) {

  const char *r_ptr, *first_ptr, *last_ptr;
  char *w_ptr, *o_str;
  int i, j;
  int i_str_len, o_str_len;

  if ( i_str == NULL )
    return NULL;

  /* Find first nonwhitespace from beginning. */
  r_ptr = i_str;
  while ( *r_ptr && isspace(*r_ptr) ) {
    r_ptr++;
  }
  first_ptr = r_ptr;
  last_ptr = r_ptr;

  i = 0;
  j = 0;
  i_str_len = 0;
  o_str_len = 0;
  /* Find lengths of input and output string. */
  while ( *r_ptr ) {
    i++;
    if ( !isalnum(*r_ptr) && (*r_ptr != '-') && (*r_ptr != '_') && (*r_ptr != '.') && (*r_ptr != '~') && (*r_ptr != ' ') )
      j += 2;
    if ( !isspace(*r_ptr) ) {
      i_str_len += i;
      o_str_len += j;
      i = 0;
      j = 0;
      last_ptr = r_ptr;
    }
    r_ptr++;
  }
  o_str_len += i_str_len;

  o_str = malloc((o_str_len + 1) * sizeof(char));
  if ( !o_str )
    return NULL;

  /* Encode string. */
  last_ptr++;
  r_ptr = first_ptr;
  w_ptr = o_str;
  while ( *r_ptr && (r_ptr != last_ptr) ) {
    if ( isalnum(*r_ptr) || (*r_ptr == '-') || (*r_ptr == '_') || (*r_ptr == '.') || (*r_ptr == '~') )
      *(w_ptr++) = *r_ptr;
    else if ( *r_ptr == ' ' )
      *w_ptr++ = '+';
    else {
      *w_ptr++ = '%';
      *w_ptr++ = to_hex((*r_ptr) >> 4);
      *w_ptr++ = to_hex((*r_ptr) & 0x0f);
    }
    r_ptr++;
  }
  *w_ptr = 0;

  return o_str;
}

static inline void _ldf_free_field(struct ldf_field *field) {
  free(field->name);
  free(field->value);
  free(field);
}

static struct ldf_field *_ldf_find(struct ldf_field *fields, const char *name) {

  struct ldf_field *cur_field;

  for ( cur_field = fields; cur_field != NULL; cur_field = cur_field->next ) {
    if ( !strcmp(cur_field->name, name) )
      return cur_field;
  }
  return NULL;

}

static int _ldf_http_get(const char *hostname, const char *port, const char *page) {

  /* Some parts from http://beej.us/guide/bgnet/output/html/singlepage/bgnet.html */

  int rv;
  int sockfd;
  struct addrinfo hints, *servinfo, *p;

  char *buffer;
  int buflen;
  int i, r_bytes;

  unsigned int i_breakingstring;
  const char breakingstring[LDF_BREAKINGSTRING_LENGTH] = LDF_BREAKINGSTRING;

  /* Set hints for getaddrinfo. */
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  /* Resolve hostname. */
  if ( getaddrinfo(hostname, port, &hints, &servinfo) < 0 ) {
    return LDF_EHOSTNOTFOUND;
  }

  /* Loop through all the results and connect to the first we can. */
  for ( p = servinfo; p != NULL; p = p->ai_next ) {
    if ( (sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0 ) {
      continue;
    }
    if ( connect(sockfd, p->ai_addr, p->ai_addrlen) < 0 ) {
      close(sockfd);
      continue;
    }
    break;
  }
  freeaddrinfo(servinfo);

  if ( p == NULL ) {
    return LDF_ECONNECTFAIL;
  }

  /* Create buffer for HTTP request. */
  buflen = strlen(page);
  buflen += strlen(hostname);
  buflen += 26;
  buffer = malloc(buflen * sizeof(char));

  /* Fill buffer with data. */
  strcpy(buffer, "GET ");
  strcat(buffer, page);
  strcat(buffer, " HTTP/1.0\r\nHost: ");
  strcat(buffer, hostname);
  strcat(buffer, "\r\n\r\n");

  /* Send buffer to server. */
  if ( send(sockfd, buffer, buflen, 0) != buflen ) {
    free(buffer);
    close(sockfd);
    return LDF_ECANNOTSEND;
  }

  /* Find end of HTTP headers: BREAKINGSTRING */
  i_breakingstring = 0;
  i = 0;
  while ( (r_bytes = recv(sockfd, buffer, buflen, 0)) > 0 ) {
    for ( i = 0; i < r_bytes; i++ ) {
      if ( buffer[i] == breakingstring[i_breakingstring] ) {
        i_breakingstring++;
        if ( i_breakingstring == LDF_BREAKINGSTRING_LENGTH ) {
          /* BREAKINGSTRING was found completely. */
          break;
        }
      } else {
        i_breakingstring = 0;
      }
    }
    if ( i_breakingstring == LDF_BREAKINGSTRING_LENGTH ) {
      break;
    }
  }

  /* Get first char of HTTP body. */
  rv = 0;
  i++;
  if ( i < r_bytes ) {
    if ( buffer[i] == '1' ) {
      rv = 1;
    }
  } else if ( (r_bytes = recv(sockfd, buffer, buflen, 0)) > 0 ) {
    if ( buffer[0] == '1' ) {
      rv = 1;
    }
  }

  free(buffer);
  close(sockfd);

  return rv;

}

/* Public functions */

int ldf_send_request(const char *request, const char *hostname, const char *port) {
  return _ldf_http_get(hostname, port, request);
}

int ldf_create_request(char** request, const char* public_key, const char* private_key, struct ldf_field *fields) {

  int len;
  struct ldf_field *cur_field;

  /* Determine length of request string. */
  len = strlen(public_key);
  len += strlen(private_key);

  for ( cur_field = fields; cur_field != NULL; cur_field = cur_field->next ) {
    len += strlen(cur_field->name);
    len += strlen(cur_field->value);
    len += 2;
  }

  /* /input/
     ?private_key=
     0x00 */
  len += 21;

  *request = malloc(len * sizeof(char));

  /* Fill request string. */
  strcpy(*request, "/input/");
  strcat(*request, public_key);
  strcat(*request, "?private_key=");
  strcat(*request, private_key);

  for ( cur_field = fields; cur_field != NULL; cur_field = cur_field->next ) {
    strcat(*request, "&");
    strcat(*request, cur_field->name);
    strcat(*request, "=");
    strcat(*request, cur_field->value);
  }

  return len;
}

void ldf_add_field(struct ldf_field **fields, const char *name, const char *value) {

  struct ldf_field *new_field;

  new_field = malloc(sizeof(struct ldf_field));
  new_field->name = _ldf_trim_encode(name);
  new_field->value = _ldf_trim_encode(value);
  new_field->next = *fields;
  *fields = new_field;

}

void ldf_update_field(struct ldf_field **fields, const char *name, const char *value) {

  struct ldf_field *cur_field;

  cur_field = _ldf_find(*fields, name);
  if ( cur_field != NULL ) {
    free(cur_field->value);
    cur_field->value = _ldf_trim_encode(value);
  }
  else {
    ldf_add_field(fields, name, value);
  }

}

int ldf_remove_field(struct ldf_field **fields, const char *name) {

  struct ldf_field *cur_field, *next_field;

  cur_field = *fields;
  if ( cur_field == NULL ) {
    return 0;
  }

  /* Check first data field. */
  if ( !strcmp(cur_field->name, name) ) {
    *fields = cur_field->next;
    _ldf_free_field(cur_field);
    return 1;
  }

  /* Check remaining data fields. */
  while ( cur_field->next ) {
    if ( !strcmp(cur_field->next->name, name) ) {
      next_field = cur_field->next;
      cur_field->next = next_field->next;
      _ldf_free_field(next_field);
      return 1;
    }
    cur_field = cur_field->next;
  }

  return 0;

}

void ldf_free_fields(struct ldf_field *fields) {

  struct ldf_field *cur_field, *next_field;

  next_field = fields;
  while ( next_field ) {
    cur_field = next_field;
    _ldf_free_field(cur_field);
    next_field = next_field->next;
  }

}
