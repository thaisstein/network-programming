#include "database.h"
#include "request.h"
#include "string.h"
#include "when_macros.h"
#include <err.h>
#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char *CREATION_REQ = "        \
CREATE TABLE IF NOT EXISTS films (  \
    title TEXT,                     \
    genre TEXT,                     \
    director TEXT,                  \
    year INT                        \
);";

sqlite3 *database_create_connection(const char *filename) {
  sqlite3 *db;

  // Create a sqlite connection
  const int flags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE;
  int rc = sqlite3_open_v2(filename, &db, flags, NULL);
  when_false_jmp(SQLITE_OK == rc, error, "Cannot open database: %s\n",
                 sqlite3_errmsg(db));

  // Create the database table
  char *errmsg;
  rc = sqlite3_exec(db, CREATION_REQ, NULL, NULL, &errmsg);
  when_false_jmp(SQLITE_OK == rc, error, "Failed to create table: %s\n",
                 errmsg);
  return db;
error:
  sqlite3_free(errmsg);
  sqlite3_close(db);
  return NULL;
}

void database_close_connection(sqlite3 *db) {
  int rc = sqlite3_close(db);
  if (SQLITE_OK != rc) {
    fprintf(stderr, "Error while closing database (returned %d): %s\n", rc,
            sqlite3_errmsg(db));
  }
}

int database_insert_film(sqlite3 *db, film_t film, int *id) {
  int rc;
  sqlite3_stmt *request;
  rc = sqlite3_prepare_v2(
      db,
      "INSERT INTO films (title, genre, director, year) VALUES (?, ?, ?, ?)",
      -1, &request, NULL);
  if (SQLITE_OK != rc) {
    fprintf(stderr, "Failed to prepare the request: %s\n", sqlite3_errmsg(db));
    goto error;
  }
  rc = sqlite3_bind_text(request, 1, film.title.str, film.title.len, NULL);
  if (SQLITE_OK != rc)
    goto fail2bind;
  rc = sqlite3_bind_text(request, 2, film.genre.str, film.genre.len, NULL);
  if (SQLITE_OK != rc)
    goto fail2bind;
  rc =
      sqlite3_bind_text(request, 3, film.director.str, film.director.len, NULL);
  if (SQLITE_OK != rc)
    goto fail2bind;
  rc = sqlite3_bind_int(request, 4, film.year);
  if (SQLITE_OK != rc)
    goto fail2bind;

  rc = sqlite3_step(request);
  when_false_jmp(SQLITE_DONE == rc, error,
                 "Failed to evaluate the request: %s\n", sqlite3_errmsg(db));
  if (id != NULL)
    *id = sqlite3_last_insert_rowid(db);
  sqlite3_finalize(request);
  return DATABASE_ERROR_NO_ERROR;
fail2bind:
  fprintf(stderr, "Failed bind parameter: %s", sqlite3_errmsg(db));
error:
  sqlite3_finalize(request);
  return DATABASE_INTERNAL_ERROR;
}

int database_delete_film(sqlite3 *db, int rowid) {
  int rc;
  sqlite3_stmt *request;
  rc = sqlite3_prepare_v2(db, "DELETE FROM films WHERE rowid = ?", -1, &request,
                          NULL);
  when_false_jmp(SQLITE_OK == rc, error, "Failed to prepare the request: %s\n",
                 sqlite3_errmsg(db));
  rc = sqlite3_bind_int(request, 1, rowid);
  if (SQLITE_OK != rc)
    goto fail2bind;

  rc = sqlite3_step(request);
  when_false_jmp(SQLITE_DONE == rc, error,
                 "Failed to evaluate the request: %s\n", sqlite3_errmsg(db));
  sqlite3_finalize(request);
  int count = sqlite3_changes(db);
  if (count == 0)
    return DATABASE_ERROR_NOT_FOUND;
  return DATABASE_ERROR_NO_ERROR;
fail2bind:
  fprintf(stderr, "Failed bind parameter: %s", sqlite3_errmsg(db));
error:
  sqlite3_finalize(request);
  return DATABASE_INTERNAL_ERROR;
}

int database_add_genre(sqlite3 *db, int id, const string_t genre) {
  int rc;
  sqlite3_stmt *select_req, *update_req;
  int error = DATABASE_INTERNAL_ERROR;

  rc = sqlite3_exec(db, "BEGIN TRANSACTION;", NULL, NULL, NULL);
  when_false_ret(SQLITE_OK == rc, DATABASE_INTERNAL_ERROR,
                 "Failed to begin transaction: %s\n", sqlite3_errmsg(db));

  // Get the genre of the film given by id
  rc = sqlite3_prepare_v2(db, "SELECT genre FROM films WHERE rowid = ?", -1,
                          &select_req, NULL);
  when_false_jmp(SQLITE_OK == rc, select_error,
                 "Failed to prepare the request: %s\n", sqlite3_errmsg(db));
  rc = sqlite3_bind_int(select_req, 1, id);
  when_false_jmp(SQLITE_OK == rc, select_error,
                 "Failed to bind parameter: %s\n", sqlite3_errmsg(db));
  rc = sqlite3_step(select_req);
  if (SQLITE_DONE == rc) {
    fprintf(stderr, "WARNING: Film nº%d not found\n", id);
    error = DATABASE_ERROR_NOT_FOUND;
    goto select_error;
  }
  when_false_jmp(SQLITE_ROW == rc, select_error,
                 "Failed to evaluate the request: %s\n", sqlite3_errmsg(db));

  const char *current_genre = (const char *)sqlite3_column_text(select_req, 0);

  // Append new genre to current genre separated by a comma
  string_t new_genre;
  string_init_view(&new_genre, current_genre, strlen(current_genre));
  string_join(&new_genre, ',', genre);

  // Finalize the request (this will free current_genre)
  sqlite3_finalize(select_req);

  rc = sqlite3_prepare_v2(db, "UPDATE films SET genre = ? WHERE rowid = ?", -1,
                          &update_req, NULL);
  if (SQLITE_OK != rc) {
    fprintf(stderr, "Failed to prepare the request: %s\n", sqlite3_errmsg(db));
    goto update_error;
  }
  rc = sqlite3_bind_text(update_req, 1, new_genre.str, new_genre.len, NULL);
  if (SQLITE_OK != rc)
    goto fail2bind;
  rc = sqlite3_bind_int(update_req, 2, id);
  if (SQLITE_OK != rc)
    goto fail2bind;

  rc = sqlite3_step(update_req);
  when_false_jmp(SQLITE_DONE == rc, update_error,
                 "Failed to evaluate the request: %s\n", sqlite3_errmsg(db));
  sqlite3_finalize(update_req);

  rc = sqlite3_exec(db, "COMMIT", NULL, NULL, NULL);
  when_false_jmp(SQLITE_OK == rc, rollback,
                 "Failed to commit transaction: %s\n", sqlite3_errmsg(db));
  string_deinit(&new_genre);
  return DATABASE_ERROR_NO_ERROR;
select_error:
  sqlite3_finalize(select_req);
  goto rollback;
fail2bind:
  fprintf(stderr, "Failed bind parameter: %s", sqlite3_errmsg(db));
update_error:
  sqlite3_finalize(update_req);
rollback:
  string_deinit(&new_genre);
  sqlite3_exec(db, "ROLLBACK", NULL, NULL, NULL);
  return error;
}

struct columns_args {
  string_t *result;
  int *rowcnt;
};

static int push_columns(void *arg, int n, char **columns,
                        [[maybe_unused]] char **labels) {
  struct columns_args *args = arg;
  char sep;
  string_t right;
  for (int i = 0; i < n; i++) {
    sep = (i == 0 ? BODY_RECORD_SEPARATOR : BODY_FIELD_SEPARATOR);
    string_init_view(&right, columns[i], strlen(columns[i]));
    string_join(args->result, sep, right);
  }
  if (NULL != args->rowcnt)
    *args->rowcnt += 1;
  return 0;
}

int database_list_titles(sqlite3 *db, string_t *body, int *count) {
  int rc;
  char *errmsg;
  struct columns_args args = {.result = body, .rowcnt = count};
  rc = sqlite3_exec(db, "SELECT rowid, title FROM films", push_columns, &args,
                    &errmsg);
  when_false_ret(SQLITE_OK == rc, DATABASE_INTERNAL_ERROR,
                 "ERROR: Failed to list films (%s)\n", sqlite3_errmsg(db));
  return DATABASE_ERROR_NO_ERROR;
}

int database_list_films(sqlite3 *db, string_t *body, int *count) {
  int rc;
  char *errmsg;
  struct columns_args args = {.result = body, .rowcnt = count};
  rc = sqlite3_exec(db, "SELECT rowid, title, genre, director, year FROM films",
                    push_columns, &args, &errmsg);
  when_false_ret(SQLITE_OK == rc, DATABASE_INTERNAL_ERROR,
                 "ERROR: Failed to list films (%s)\n", sqlite3_errmsg(db));
  return DATABASE_ERROR_NO_ERROR;
}

int database_get_film(sqlite3 *db, unsigned id, string_t *body) {
  int rc;
  sqlite3_stmt *request;
  rc = sqlite3_prepare_v2(
      db,
      "SELECT rowid, title, genre, director, year FROM films WHERE rowid = ?",
      -1, &request, NULL);
  when_false_ret(SQLITE_OK == rc, DATABASE_INTERNAL_ERROR,
                 "ERROR: Failed to prepare statement (%s)\n",
                 sqlite3_errmsg(db));
  rc = sqlite3_bind_int(request, 1, id);
  when_false_jmp(SQLITE_OK == rc, error, "ERROR: Failed to bind id\n");
  rc = sqlite3_step(request);
  when_true_jmp(SQLITE_DONE == rc, not_found,
                "WARNING: No row returned for id %d\n", id);
  when_false_jmp(SQLITE_ROW == rc, error,
                 "ERROR: Failed to execute statement (%s)\n",
                 sqlite3_errmsg(db));
  int column_count = sqlite3_column_count(request);
  char **columns = malloc(column_count * sizeof(char *));
  for (int i = 0; i < column_count; i++)
    columns[i] = (char *)sqlite3_column_text(request, i);
  struct columns_args args = {body, NULL};
  push_columns(&args, column_count, columns, NULL);
  sqlite3_finalize(request);
  return DATABASE_ERROR_NO_ERROR;
not_found:
  sqlite3_finalize(request);
  return DATABASE_ERROR_NOT_FOUND;
error:
  sqlite3_finalize(request);
  return DATABASE_INTERNAL_ERROR;
}

int database_list_by_genre(sqlite3 *db, string_t genre, string_t *body,
                           int *count) {
  int rc;
  sqlite3_stmt *request;
  *count = 0;
  rc = sqlite3_prepare_v2(db,
                          "SELECT rowid, title, genre, director, year FROM "
                          "films WHERE genre LIKE ?",
                          -1, &request, NULL);
  when_false_ret(SQLITE_OK == rc, DATABASE_INTERNAL_ERROR,
                 "ERROR: Failed to prepare statement (%s)\n",
                 sqlite3_errmsg(db));
  // Allocate room for matching chars and null terminating byte
  char *pattern = malloc(genre.len + 3);
  snprintf(pattern, genre.len + 3, "%%%s%%", genre.str);
  rc = sqlite3_bind_text(request, 1, pattern, -1, NULL);
  when_false_jmp(SQLITE_OK == rc, error, "ERROR: Failed to bind id\n");
  struct columns_args args = {body, count};
  while (SQLITE_ROW == (rc = sqlite3_step(request))) {
    int column_count = sqlite3_column_count(request);
    char **columns = malloc(column_count * sizeof(char *));
    for (int i = 0; i < column_count; i++)
      columns[i] = (char *)sqlite3_column_text(request, i);
    push_columns(&args, column_count, columns, NULL);
  }
  sqlite3_finalize(request);
  return DATABASE_ERROR_NO_ERROR;
error:
  sqlite3_finalize(request);
  return DATABASE_INTERNAL_ERROR;
}
