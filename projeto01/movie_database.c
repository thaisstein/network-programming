#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>
#include "movie_database.h"

#define MAX_GENRE_COUNT 50

int init_database(sqlite3 **db) {
    int rc = sqlite3_open("movies.db", db);
    if (rc) {
        fprintf(stderr, "Erro ao abrir banco de dados: %s\n", sqlite3_errmsg(*db));
        return rc;
    }

    // Criar tabelas se não existirem
    const char *create_movies_table = 
        "CREATE TABLE IF NOT EXISTS movies ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "title TEXT NOT NULL, "
        "director TEXT, "
        "release_year INTEGER);";

    const char *create_genres_table = 
        "CREATE TABLE IF NOT EXISTS movie_genres ("
        "movie_id INTEGER, "
        "genre TEXT, "
        "UNIQUE(movie_id, genre), "
        "FOREIGN KEY(movie_id) REFERENCES movies(id) ON DELETE CASCADE);";

    char *err_msg = 0;
    rc = sqlite3_exec(*db, create_movies_table, 0, 0, &err_msg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
        return rc;
    }

    rc = sqlite3_exec(*db, create_genres_table, 0, 0, &err_msg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
        return rc;
    }

    return SQLITE_OK;
}


int add_movie(sqlite3 *db, Movie *movie) {
    sqlite3_stmt *stmt;
    const char *sql = "INSERT INTO movies (title, director, release_year) VALUES (?, ?, ?);";
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    
    sqlite3_bind_text(stmt, 1, movie->title, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, movie->director, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 3, movie->release_year);

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        fprintf(stderr, "Erro ao inserir filme: %s\n", sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        return -1;
    }

    int movie_id = sqlite3_last_insert_rowid(db);
    sqlite3_finalize(stmt);

    // Adicionar gêneros
    for (int i = 0; i < movie->genre_count; i++) {
        add_genre_to_movie(db, movie_id, movie->genres[i]);
    }

    return movie_id;
}

int add_genre_to_movie(sqlite3 *db, int movie_id, const char *genre) {
    const char *sql = "INSERT OR IGNORE INTO movie_genres (movie_id, genre) VALUES (?, ?);";
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    
    sqlite3_bind_int(stmt, 1, movie_id);
    sqlite3_bind_text(stmt, 2, genre, -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        fprintf(stderr, "Erro ao adicionar gênero: %s\n", sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        return -1;
    }

    sqlite3_finalize(stmt);
    return 0;
}

int remove_genre_from_movie(sqlite3 *db, int movie_id, const char *genre) {
    const char *sql = "DELETE FROM movie_genres WHERE movie_id = ? AND genre = ?;";
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    
    sqlite3_bind_int(stmt, 1, movie_id);
    sqlite3_bind_text(stmt, 2, genre, -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        fprintf(stderr, "Erro ao remover gênero: %s\n", sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        return -1;
    }

    sqlite3_finalize(stmt);
    return 0;
}

int get_movie_by_id(sqlite3 *db, int movie_id, Movie *result) {
    const char *sql = 
        "SELECT id, title, director, release_year FROM movies WHERE id = ?;";
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    
    sqlite3_bind_int(stmt, 1, movie_id);

    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        result->id = sqlite3_column_int(stmt, 0);
        strcpy(result->title, (const char*)sqlite3_column_text(stmt, 1));
        strcpy(result->director, (const char*)sqlite3_column_text(stmt, 2));
        result->release_year = sqlite3_column_int(stmt, 3);
        
        // Obter gêneros
        char **genres;
        int genre_count;
        get_movie_genres(db, movie_id, &genres, &genre_count);
        
        result->genre_count = genre_count;
        for (int i = 0; i < genre_count; i++) {
            strcpy(result->genres[i], genres[i]);
            free(genres[i]);
        }
        free(genres);

        sqlite3_finalize(stmt);
        return 0;
    }

    sqlite3_finalize(stmt);
    return -1;
}

int get_movie_genres(sqlite3 *db, int movie_id, char ***genres, int *genre_count) {
    const char *sql = "SELECT genre FROM movie_genres WHERE movie_id = ?;";
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    
    sqlite3_bind_int(stmt, 1, movie_id);

    *genre_count = 0;
    *genres = malloc(sizeof(char*) *MAX_GENRE_COUNT);

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        (*genres)[*genre_count] = strdup((const char*)sqlite3_column_text(stmt, 0));
        (*genre_count)++;
    }

    sqlite3_finalize(stmt);
    return 0;
}

int get_movies_by_genre(sqlite3 *db, const char *genre, Movie **results, int *result_count) {
    const char *sql = 
        "SELECT m.id, m.title, m.director, m.release_year "
        "FROM movies m "
        "JOIN movie_genres mg ON m.id = mg.movie_id "
        "WHERE mg.genre = ?;";
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    
    sqlite3_bind_text(stmt, 1, genre, -1, SQLITE_STATIC);

    *result_count = 0;
    *results = malloc(sizeof(Movie) * 100);  // Alocação inicial para 100 filmes

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        Movie *movie = &((*results)[*result_count]);
        movie->id = sqlite3_column_int(stmt, 0);
        strcpy(movie->title, (const char*)sqlite3_column_text(stmt, 1));
        strcpy(movie->director, (const char*)sqlite3_column_text(stmt, 2));
        movie->release_year = sqlite3_column_int(stmt, 3);
        
        // Obter gêneros para este filme
        char **genres;
        int genre_count;
        get_movie_genres(db, movie->id, &genres, &genre_count);
        
        movie->genre_count = genre_count;
        for (int i = 0; i < genre_count; i++) {
            strcpy(movie->genres[i], genres[i]);
            free(genres[i]);
        }
        free(genres);
        
        (*result_count)++;
    }

    sqlite3_finalize(stmt);
    return 0;
}

int get_all_movies(sqlite3 *db, Movie **results, int *result_count) {
    const char *sql = "SELECT id, title, director, release_year FROM movies;";
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);

    *result_count = 0;
    *results = malloc(sizeof(Movie) * 100);  // Alocação inicial para 100 filmes

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        Movie *movie = &((*results)[*result_count]);
        movie->id = sqlite3_column_int(stmt, 0);
        strcpy(movie->title, (const char*)sqlite3_column_text(stmt, 1));
        strcpy(movie->director, (const char*)sqlite3_column_text(stmt, 2));
        movie->release_year = sqlite3_column_int(stmt, 3);
        
        // Obter gêneros para este filme
        char **genres;
        int genre_count;
        get_movie_genres(db, movie->id, &genres, &genre_count);
        
        movie->genre_count = genre_count;
        for (int i = 0; i < genre_count; i++) {
            strcpy(movie->genres[i], genres[i]);
            free(genres[i]);
        }
        free(genres);
        
        (*result_count)++;
    }

    sqlite3_finalize(stmt);
    return 0;
}

// Restante do código permanece o mesmo...