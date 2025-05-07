#ifndef FILM_H
#define FILM_H

#include "string.h"

struct film {
  int id;
  string_t title;
  string_t genre;
  string_t director;
  int year;
};

typedef struct film film_t;

#endif // !FILM_H
