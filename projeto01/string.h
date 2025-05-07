#ifndef STRING_H
#define STRING_H

#include "when_macros.h"
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

/**
 * @file string.h
 * @brief Managed string with length
 * defines functions to create, free and manipulate string buffers
 * @ingroup string
 */

#define EMPTY_STRING_INIT {.str = NULL, .len = 0, .allocated = 0}
#define EMPTY_STRING (string_t){.str = NULL, .len = 0, .allocated = 0}

/**
 * @defgroup string Managed strings
 * @{
 */

/**
 * @typedef string_t
 * @brief Typedef for the string structure
 *
 */
typedef struct string string_t;

/**
 * @struct string
 * @brief A managed string
 */
struct string {
  char *str;      /**< Start of the string */
  size_t len;     /**< Length of the string */
  char allocated; /**< Does the string need to be freed */
};

static inline void string_init_view(string_t *str, const char *view,
                                    size_t len) {
  *str = (string_t){.str = (char *)view, .len = len, .allocated = 0};
}

static inline void string_init_take(string_t *str, char *buffer, size_t len) {
  if (str->allocated)
    free(str->str);
  *str = (string_t){.str = buffer, .len = len, .allocated = 1};
}

static inline void string_deinit(string_t *string) {
  if (string->allocated)
    free(string->str);
  *string = EMPTY_STRING;
}

static inline string_t string_split(char token, const string_t *string) {
  static string_t remaining = EMPTY_STRING_INIT;
  string_t result;
  if (string != NULL)
    remaining = *string;
  if (remaining.str == NULL || remaining.len == 0)
    return EMPTY_STRING;
  for (unsigned i = 0; i < remaining.len; i++) {
    if (remaining.str[i] == token) {
      string_init_view(&result, remaining.str, i);
      remaining.len -= i + 1;
      remaining.str += i + 1;
      return result;
    }
  }
  result = remaining;
  remaining = EMPTY_STRING;
  return result;
}

static inline void string_join(string_t *left, char token,
                               const string_t right) {
  // If right string is empty nothing happens
  if (right.str == NULL || right.len == 0)
    return;
  // If left string is empty it becomes a view on the right string
  if (left->str == NULL) {
    left->str = right.str;
    left->len = right.len;
    left->allocated = 0;
    return;
  }
  // Realloc space for both string + token + null terminator
  size_t len = left->len + right.len + 2;
  if (left->allocated) {
    left->str = (char *)realloc(left->str, len);
  } else {
    char *buffer = (char *)malloc(len);
    memcpy(buffer, left->str, left->len);
    left->str = buffer;
  }
  left->str[left->len] = token;
  memcpy(left->str + left->len + 1, right.str, right.len);
  left->str[left->len + 1 + right.len] = '\0';
  left->len += 1 + right.len;
  return;
}

static inline int string_to_integer(string_t string, int *value) {
  char *endptr;
  *value = strtol(string.str, &endptr, 10);
  if (*value == 0 && string.str + string.len != endptr)
    return -1;
  return 0;
}

#endif // !STRING_H
