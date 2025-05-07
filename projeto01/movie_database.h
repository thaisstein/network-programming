#ifndef MOVIE_DATABASE_H
#define MOVIE_DATABASE_H

#include <sqlite3.h>

#define MAX_TITLE_LENGTH 100
#define MAX_DIRECTOR_LENGTH 100
#define MAX_GENRE_COUNT 50
#define MAX_GENRE_LENGTH 100

typedef struct {
    int id;
    char title[MAX_TITLE_LENGTH];
    char director[MAX_DIRECTOR_LENGTH];
    char genres[MAX_GENRE_COUNT][MAX_GENRE_LENGTH];
    int release_year;
    int genre_count;
} Movie;

/* Funções de inicialização e gerenciamento do banco de dados */
int init_database(sqlite3 **db);
void close_database(sqlite3 *db);

/* Operações de Registro e Modificação */ 

// Cadastrar um novo filme
int add_movie(sqlite3 *db, Movie *movie);
// Adiciona novo gênero ao filme
int add_genre_to_movie(sqlite3 *db, int movie_id, const char *genre);
// Remover filme pelo indicador
int remove_movie(sqlite3 *db, int movie_id);

/* Operações de Consulta */ 

// Lista com o identificador e o título de cada filme
int list_movie_titles(sqlite3 *db, char ***titles, int *count);
//Listar informações de um filme específico
int get_movie_by_id(sqlite3 *db, int movie_id, Movie *result);
// Listar todos os filmes de um determinado gênero
int get_movies_by_genre(sqlite3 *db, const char *genre, Movie **results, int *result_count);

// Lista com informações sobre todos os filmes
int get_all_movies(sqlite3 *db, Movie **results, int *result_count);
int get_movie_genres(sqlite3 *db, int movie_id, char ***genres, int *genre_count); //TODO take out?

#endif // MOVIE_DATABASE_H