#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <sqlite3.h>
#include "movie_database.h"

#define PORT 8080
#define MAX_CLIENTS 10

sqlite3 *global_database;

void handle_client_request(int client_socket) {
    char buffer[1024];
    int operation;

    while (recv(client_socket, &operation, sizeof(int), 0) > 0) {
        switch(operation) {
            case 1: { // Cadastrar novo filme
                Movie movie;
                recv(client_socket, &movie, sizeof(Movie), 0);
                int result = add_movie(global_database, &movie);
                send(client_socket, &result, sizeof(int), 0);
                break;
            }
            case 2: { // Adicionar gênero
                int movie_id;
                char genre[MAX_GENRE_LENGTH];
                recv(client_socket, &movie_id, sizeof(int), 0);
                recv(client_socket, genre, MAX_GENRE_LENGTH, 0);
                int result = add_genre_to_movie(global_database, movie_id, genre);
                send(client_socket, &result, sizeof(int), 0);
                break;
            }
            case 5: { // Listar todos os filmes
                Movie *movies;
                int count;
                get_all_movies(global_database, &movies, &count);
                send(client_socket, &count, sizeof(int), 0);
                for (int i = 0; i < count; i++) {
                    send(client_socket, &movies[i], sizeof(Movie), 0);
                }
                free(movies);
                break;
            }
            case 6: { // Buscar filme por ID
                int movie_id;
                recv(client_socket, &movie_id, sizeof(int), 0);
                Movie movie;
                int result = get_movie_by_id(global_database, movie_id, &movie);
                send(client_socket, &result, sizeof(int), 0);
                if (result == 0) {
                    send(client_socket, &movie, sizeof(Movie), 0);
                }
                break;
            }
            case 7: { // Listar filmes por gênero
                char genre[MAX_GENRE_LENGTH];
                recv(client_socket, genre, MAX_GENRE_LENGTH, 0);
                Movie *movies;
                int count;
                get_movies_by_genre(global_database, genre, &movies, &count);
                send(client_socket, &count, sizeof(int), 0);
                for (int i = 0; i < count; i++) {
                    send(client_socket, &movies[i], sizeof(Movie), 0);
                }
                free(movies);
                break;
            }
            case 8: { // Remover gênero de filme
                int movie_id;
                char genre[MAX_GENRE_LENGTH];
                recv(client_socket, &movie_id, sizeof(int), 0);
                recv(client_socket, genre, MAX_GENRE_LENGTH, 0);
                int result = remove_genre_from_movie(global_database, movie_id, genre);
                send(client_socket, &result, sizeof(int), 0);
                break;
            }
        }
    }
    close(client_socket);
}

// Resto do código do servidor permanece o mesmo