#include "request.h"
#include "string.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define MAX_LINE 1024
#define FIELD_MAX_LEN 1024

const char *COMMAND_HELPER_TXT = "\
0) CREATE_FILM      \n\
1) REMOVE_FILM      \n\
2) ADD_GENRE        \n\
3) LIST_TITLES      \n\
4) LIST_FILMS       \n\
5) GET_FILM         \n\
6) LIST_BY_GENRE    \
";

void display_body(size_t body_size, char *body) {
  char field[FIELD_MAX_LEN];
  if (body_size == 0)
    return;
  unsigned start = 0;
  char newline = 1;
  // Read until body_size + 1 (send_body puts a '\0' at body_size + 1)
  for (unsigned i = 0; i < body_size; i++) {
    if (body[i] == BODY_FIELD_SEPARATOR || body[i] == BODY_RECORD_SEPARATOR) {
      const char *fmt = (newline ? "| %s |" : " %s |");
      newline = (body[i] == BODY_RECORD_SEPARATOR);
      body[i] = '\0';
      snprintf(field, FIELD_MAX_LEN, fmt, body + start);
      printf("%s", field);
      if (newline)
        printf("\n");
      start = i + 1;
    }
  }
  printf(" %s |\n", body + start);
}

void display_response(response_header_t header, char *body) {

  switch (header.code) {
  case NO_ERROR:
    fprintf(stderr, "The command ran successfuly on the server\n");
    display_body(header.body_size, body);
    break;
  case INTERNAL_ERROR:
    fprintf(stderr, "An internal server error occured.\n");
    break;
  case ERROR_NOT_FOUND:
    fprintf(stderr, "Film not found.\n");
    break;
  default:
    fprintf(stderr, "Unknown error code: %d\n", header.code);
    break;
  }
}

int perform_request(int req_fd, request_header_t req_header,
                    const char *req_body, response_header_t *res_header,
                    char **res_body) {
  // Send request header
  if (0 != send_header(req_fd, &req_header, sizeof(request_header_t)))
    return -1;
  // Send request body
  if (0 != send_body(req_fd, req_body, req_header.body_size))
    return -1;
  fprintf(stderr, "INFO: Waiting for response...\n");
  // Receive response header
  if (0 != receive_header(req_fd, res_header, sizeof(response_header_t)))
    return -1;
  if (res_header->body_size == 0) {
    *res_body = NULL;
    goto end;
  }
  // Receive response body
  if (NULL == (*res_body = receive_body(req_fd, res_header->body_size)))
    return -1;
end:
  fprintf(stderr, "INFO: Response received.\n");
  return 0;
}

static int getuint(unsigned *n) {
  int rc = scanf("%u", n);
  while ((getchar()) != '\n')
    ;
  return rc;
}

static int getfield(char *field) {
  const char *rc = fgets(field, FIELD_MAX_LEN, stdin);
  if (rc == NULL)
    return -1;
  int len = strlen(rc);
  if (field[len - 1] == '\n')
    field[len - 1] = '\0';
  return 0;
}

int main(int argc, char *argv[]) {
  int sock_fd;
  struct sockaddr_in servaddr;
  char *port_delimiter, *endptr;
  unsigned long port;

  // Parse address and port to connect to from command line
  port_delimiter = strstr(argv[1], ":");
  if (argc != 2 || NULL == port_delimiter) {
    fprintf(stderr, "Usage: ./client <address>:<port>");
    goto error;
  }
  *port_delimiter = '\0';
  port = strtoul(port_delimiter + 1, &endptr, 10);
  if (port == 0 || port >= (1 << 16)) {
    fprintf(stderr, "Invalid port number: %lu from: \"%s\"\n", port,
            port_delimiter + 1);
    goto error;
  }

  // Create socket and connect to server
  sock_fd = socket(PF_INET, SOCK_STREAM, 0);
  memset(&servaddr, 0, sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_port = htons(port);
  if (1 != inet_pton(AF_INET, argv[1], &servaddr.sin_addr)) {
    fprintf(stderr, "Invalid address: %s\n", argv[1]);
    goto error;
  }
  if (-1 == connect(sock_fd, (struct sockaddr *)&servaddr, sizeof(servaddr))) {
    perror("connect");
    goto error;
  }

  unsigned rc, command, id, year;
  size_t body_size = 0;
  char title[FIELD_MAX_LEN];
  char genre[FIELD_MAX_LEN];
  char director[FIELD_MAX_LEN];
  char body[3 * FIELD_MAX_LEN + 5];
  response_header_t res_header;
  char *res_body;
  while (1) {
    puts(COMMAND_HELPER_TXT);
    printf("Enter command id: ");
    rc = getuint(&command);
    if (1 != rc)
      continue;
    switch (command) {
    case CREATE_FILM:
      printf("Title: ");
      getfield(title);
      printf("Genre: ");
      getfield(genre);
      printf("Director: ");
      getfield(director);
      printf("Year: ");
      if (1 != getuint(&year) || year > 9999) {
        fprintf(stderr, "The year should be an integer between 0 and 9999\n");
        continue;
      }
      body_size = snprintf(body, 3 * FIELD_MAX_LEN + 5, "%s\x1F%s\x1F%s\x1F%d",
                           title, genre, director, year);
      break;
    case REMOVE_FILM:
      printf("Film id to remove: ");
      if (1 != getuint(&id)) {
        fprintf(stderr, "Invalid index.\n");
        continue;
      }
      body_size = snprintf(body, 10, "%d", id);
      break;
    case ADD_GENRE:
      printf("Film id to modify: ");
      if (1 != getuint(&id)) {
        fprintf(stderr, "Invalid index.\n");
        continue;
      }
      printf("Genres to add (comma separated): ");
      getfield(genre);
      body_size = snprintf(body, 3 * FIELD_MAX_LEN + 5, "%d\x1F%s", id, genre);
      break;
    case LIST_TITLES:
    case LIST_FILMS:
      break;
    case GET_FILM:
      printf("Film id to get: ");
      if (1 != scanf("%u", &id)) {
        fprintf(stderr, "Invalid index.\n");
        continue;
      }
      body_size = snprintf(body, 10, "%d", id);
      break;
    case LIST_BY_GENRE:
      printf("Genre: ");
      getfield(genre);
      body_size = snprintf(body, 3 * FIELD_MAX_LEN + 5, "%s", genre);
      break;
    default:
      fprintf(stderr, "WARNING: unknown command: %hu\n", command);
      continue;
    }
    perform_request(sock_fd, (request_header_t){command, body_size}, body,
                    &res_header, &res_body);
    display_response(res_header, res_body);
    free(res_body);
  }
  return EXIT_SUCCESS;
error:
  return EXIT_FAILURE;
}
