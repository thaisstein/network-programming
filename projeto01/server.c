#include <asm-generic/socket.h>
#include <assert.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "database.h"
#include "request.h"
#include "string.h"
#include "when_macros.h"

#define MAX_LINE 1024

const unsigned short SERV_PORT = 7080;
const unsigned int MAX_QUEUED_REQUESTS = 1000;
const unsigned int MAX_PARALLEL_CONNECTIONS = 10;

const unsigned int COMMANDS_LEN = 7;

void send_response(int res_fd, response_header_t header, const char *body) {
  fprintf(stderr, "INFO: Sending response...\n");
  send_header(res_fd, &header, sizeof(response_header_t));
  if (body == NULL || header.body_size == 0)
    goto end;
  send_body(res_fd, body, header.body_size);
end:
  fprintf(stderr, "INFO: Response sent.\n");
}

int execute_command(command_e command, string_t req_body, sqlite3 *db,
                    int res_fd) {
  int rc;
  fprintf(stderr, "COMMAND n°%d\n", command);

  film_t film;
  int id, count;
  string_t pid, pyear;              // String view on req_body
  string_t res_body = EMPTY_STRING; // Allocated string
  response_header_t res_header = {NO_ERROR, 0, 0};
  switch (command) {
  case CREATE_FILM:
    film.title = string_split(BODY_FIELD_SEPARATOR, &req_body);
    film.genre = string_split(BODY_FIELD_SEPARATOR, NULL);
    film.director = string_split(BODY_FIELD_SEPARATOR, NULL);
    pyear = string_split(BODY_FIELD_SEPARATOR, NULL);
    if (0 != string_to_integer(pyear, &film.year))
      goto invalid_id;
    rc = database_insert_film(db, film, &id);
    res_header.count = id;
    break;
  case REMOVE_FILM:
    pid = string_split(BODY_FIELD_SEPARATOR, &req_body);
    if (0 != string_to_integer(pid, &id))
      goto invalid_id;
    rc = database_delete_film(db, id);
    break;
  case ADD_GENRE:
    pid = string_split(BODY_FIELD_SEPARATOR, &req_body);
    if (0 != string_to_integer(pid, &id))
      goto invalid_id;
    film.genre = string_split(BODY_FIELD_SEPARATOR, NULL);
    rc = database_add_genre(db, id, film.genre);
    break;
  case LIST_TITLES:
    rc = database_list_titles(db, &res_body, &count);
    res_header.count = count;
    break;
  case LIST_FILMS:
    rc = database_list_films(db, &res_body, &count);
    res_header.count = count;
    break;
  case GET_FILM:
    if (0 != string_to_integer(req_body, &id))
      goto invalid_id;
    rc = database_get_film(db, id, &res_body);
    res_header.count = 1;
    break;
  case LIST_BY_GENRE:
    database_list_by_genre(db, req_body, &res_body, &count);
    res_header.count = count;
    break;
  default:
    fprintf(stderr, "WARNING: unknown command: %hu\n", command);
    return -1;
  }
  // Set header depending on the return code of database function
  switch (rc) {
  case DATABASE_ERROR_NO_ERROR:
    res_header.body_size = res_body.len;
    break;
  case DATABASE_ERROR_NOT_FOUND:
    res_header.code = ERROR_NOT_FOUND;
    break;
  default:
    res_header.code = INTERNAL_ERROR;
    break;
  }
  // Send response header and body
  send_response(res_fd, res_header, res_body.str);
  string_deinit(&res_body);
  return 0;
invalid_id:
  fprintf(stderr, "WARNING: id should be an integer: %s\n", pid.str);
  return -1;
}

void *respond_to_request(void *arg) {
  int res_fd = (int)(uintptr_t)arg;
  request_header_t header;
  char *buffer = NULL;
  string_t body = EMPTY_STRING;
  sqlite3 *db;
  db = database_create_connection("streaming.db");
  when_null_jmp(db, disconnect, "Failed to connect to database. Exiting.\n");

  // Read headers until connection is closed
  while (0 == receive_header(res_fd, &header, sizeof(request_header_t))) {
    fprintf(stderr, "INFO: Header received.\n");
    if (header.body_size > 0) {
      buffer = receive_body(res_fd, header.body_size);
      when_null_jmp(buffer, close, "Error: Failed to receive request body.\n");
      // Frees the previous body and take ownership of the new one
      string_init_take(&body, buffer, header.body_size);
      fprintf(stderr, "INFO: Body received.\n");
    }
    // Execute the command given by header and body and write response to res_fd
    execute_command(header.command, body, db, res_fd);
  }
  // Frees the last body
  string_deinit(&body);
disconnect:
  database_close_connection(db);
close:
  close(res_fd);
  return NULL;
}

int main(void) {
  // Creation of the server socket
  struct sockaddr_in servaddr;
  int sock_fd = socket(PF_INET, SOCK_STREAM, 0);
  int option = 1;
  setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));
  if (-1 == sock_fd) {
    perror("socket");
    return EXIT_FAILURE;
  }

  // Listen on SERV_PORT
  memset(&servaddr, 0, sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  servaddr.sin_port = htons(SERV_PORT);
  if (-1 == bind(sock_fd, (struct sockaddr *)&servaddr, sizeof(servaddr))) {
    perror("bind");
    goto error;
  }
  if (-1 == listen(sock_fd, SOMAXCONN)) {
    perror("listen");
    goto error;
  }

  // ACCEPT INCOMING REQUESTS
  struct sockaddr_in cliaddr;
  pthread_t thread;
  while (1) {
    socklen_t clilen = sizeof(cliaddr);
    int res_fd = accept(sock_fd, (struct sockaddr *)&cliaddr, &clilen);
    if (-1 == res_fd) {
      perror("accept");
      goto error;
    }
    fprintf(stderr, "INFO: A new client connected\n");
    // Create a new thread and pass it the socket file descriptor
    pthread_create(&thread, NULL, respond_to_request,
                   (void *)(uintptr_t)res_fd);
    pthread_detach(thread);
  }

  close(sock_fd);
  return EXIT_SUCCESS;

error:
  close(sock_fd);
  return EXIT_FAILURE;
}
