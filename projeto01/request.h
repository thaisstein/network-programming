#ifndef REQUEST_H
#define REQUEST_H

#include <stddef.h>
#include <stdint.h>

#define BODY_RECORD_SEPARATOR '\x1E'
#define BODY_FIELD_SEPARATOR '\x1F'

enum command : uint16_t {
  CREATE_FILM,
  REMOVE_FILM,
  ADD_GENRE,
  LIST_TITLES,
  LIST_FILMS,
  GET_FILM,
  LIST_BY_GENRE,
};

typedef enum command command_e;

typedef struct request_header {
  command_e command;
  uint16_t body_size;
} request_header_t;

typedef struct request_header request_header_t;

enum response_code : uint16_t {
  NO_ERROR,
  INTERNAL_ERROR,
  ERROR_NOT_FOUND,
};

typedef enum response_code response_code_e;

struct response_header {
  response_code_e code;
  uint16_t count;
  uint16_t body_size;
};

typedef struct response_header response_header_t;

int send_header(int fd, void *header, size_t header_size);

int receive_header(int fd, void *header, size_t header_size);

int send_body(int fd, const char *body, size_t body_size);

char *receive_body(int fd, size_t body_size);

#endif // !REQUEST_H
