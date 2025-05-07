#include "request.h"
#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

int send_header(int fd, void *header, size_t header_size) {
  ssize_t rc = write(fd, header, header_size);
  if (rc != (ssize_t)header_size) {
    if (rc < 0)
      perror("write header");
    else if (rc == 0)
      fprintf(stderr, "ERROR: Failed to send header\n");
    else
      fprintf(stderr, "ERROR: Failed to send header (%zu bytes out of %zu)\n",
              rc, header_size);
    return -1;
  }
  return 0;
}

int receive_header(int fd, void *header, size_t header_size) {
  ssize_t rc = read(fd, header, header_size);
  if (rc != (ssize_t)header_size) {
    if (rc < 0)
      perror("read header");
    else if (rc == 0)
      fprintf(stderr, "ERROR: Failed to receive header (connection closed)\n");
    else
      fprintf(stderr,
              "ERROR: Failed to receive header (%zu bytes out of %zu)\n", rc,
              header_size);
    return -1;
  }
  return 0;
}

int send_body(int fd, const char *body, size_t body_size) {
  int rc;
  size_t body_sent = 0;
  while (body_sent < body_size) {
    rc = write(fd, body, body_size);
    if (rc < 0 && errno == EAGAIN)
      continue;
    else if (rc < 0) {
      perror("write body");
      goto error;
    } else if (rc == 0) {
      fprintf(stderr, "ERROR: Failed to send body\n");
      goto error;
    }
    body_sent += rc;
  }
  return 0;
error:
  return -1;
}

char *receive_body(int fd, size_t body_size) {
  size_t body_read = 0;
  int rc;
  char *body = calloc(body_size + 1, 1);
  if (body == NULL)
    return NULL;
  while (body_read < body_size) {
    rc = read(fd, body + body_read, body_size - body_read);
    if (rc < 0 && errno == EINTR)
      continue;
    else if (rc < 0) {
      perror("read");
      goto error;
    } else if (rc == 0) {
      fprintf(stderr, "WARNING: Connection prematurely closed.\n");
      goto error;
    }
    body_read += rc;
  }
  return body;
error:
  return NULL;
}
