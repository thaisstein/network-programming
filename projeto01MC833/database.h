#ifndef DATABASE_H
#define DATABASE_H

#include "film.h"
#include <sqlite3.h>

#define DATABASE_ERROR_NO_ERROR 0
#define DATABASE_ERROR_NOT_FOUND 1
#define DATABASE_INTERNAL_ERROR 2

sqlite3 *database_create_connection(const char *filename);
void database_close_connection(sqlite3 *db);
int database_insert_film(sqlite3 *db, film_t film, int *id);
int database_delete_film(sqlite3 *db, int id);
int database_add_genre(sqlite3 *db, int id, const string_t genre);
int database_list_titles(sqlite3 *db, string_t *body, int *count);
int database_list_films(sqlite3 *db, string_t *body, int *count);
int database_get_film(sqlite3 *db, unsigned id, string_t *body);
int database_list_by_genre(sqlite3 *db, string_t genre, string_t *body,
                           int *count);

#endif // !DATABASE_H
