#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "movie_database.h"

#define SERVER_IP "127.0.0.1"
#define PORT 8080

void print_menu() {
    printf("\n--- Sistema de Streaming de Filmes ---\n");
    printf("1. Cadastrar novo filme\n");
    printf("2. Adicionar gênero a filme\n");
    printf("3. Remover filme\n");
    printf("4. Listar títulos de filmes\n");
    printf("5. Listar todos os filmes\n");
    printf("6. Buscar filme por ID\n");
    printf("7. Listar filmes por gênero\n");
    printf("8. Remover gênero de filme\n");
    printf("0. Sair\n");
    printf("Escolha uma opção: ");
}

int main() {
    int client_socket;
    struct sockaddr_in server_addr;

    client_socket = socket(AF_INET, SOCK_STREAM, 0);

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
    server_addr.sin_port = htons(PORT);

    if (connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Erro na conexão");
        exit(1);
    }

    int choice;
    do {
        print_menu();
        scanf("%d", &choice);

        switch(choice) {
            case 1: { // Cadastrar filme
                Movie movie;
                memset(&movie, 0, sizeof(Movie));  // Limpar estrutura

                printf("Digite o título: ");
                scanf(" %[^\n]", movie.title);
                
                printf("Digite o diretor: ");
                scanf(" %[^\n]", movie.director);
                
                printf("Digite o ano de lançamento: ");
                scanf("%d", &movie.release_year);
                
                // Adicionar gêneros
                printf("Quantos gêneros deseja adicionar? (máx %d): ", MAX_GENRES_PER_MOVIE);
                scanf("%d", &movie.genre_count);
                
                for (int i = 0; i < movie.genre_count; i++) {
                    printf("Digite o gênero %d: ", i+1);
                    scanf(" %[^\n]", movie.genres[i]);
                }
                
                int operation = 1;
                send(client_socket, &operation, sizeof(int), 0);
                send(client_socket, &movie, sizeof(Movie), 0);
                
                int result;
                recv(client_socket, &result, sizeof(int), 0);
                printf("Filme cadastrado com ID: %d\n", result);
                break;
            }
            case 2: { // Adicionar gênero
                int movie_id;
                char genre[MAX_GENRE_LENGTH];
                printf("Digite o ID do filme: ");
                scanf("%d", &movie_id);
                printf("Digite o gênero: ");
                scanf(" %[^\n]", genre);
                
                int operation = 2;
                send(client_socket, &operation, sizeof(int), 0);
                send(client_socket, &movie_id, sizeof(int), 0);
                send(client_socket, genre, MAX_GENRE_LENGTH, 0);
                
                int result;
                recv(client_socket, &result, sizeof(int), 0);
                printf(result == 0 ? "Gênero adicionado com sucesso\n" : "Erro ao adicionar gênero\n");
                break;
            }
            case 5: { // Listar todos os filmes
                int operation = 5;
                send(client_socket, &operation, sizeof(int), 0);
                
                int count;
                recv(client_socket, &count, sizeof(int), 0);
                
                printf("Lista de Filmes:\n");
                for (int i = 0; i < count; i++) {
                    Movie movie;
                    recv(client_socket, &movie, sizeof(Movie), 0);
                    
                    printf("ID: %d\n", movie.id);
                    printf("Título: %s\n", movie.title);
                    printf("Diretor: %s\n", movie.director);
                    printf("Ano: %d\n", movie.release_year);
                    
                    printf("Gêneros: ");
                    for (int j = 0; j < movie.genre_count; j++) {
                        printf("%s%s", movie.genres[j], 
                               j < movie.genre_count - 1 ? ", " : "\n");
                    }
                    printf("\n");
                }
                break;
            }
            case 6: { // Buscar filme por ID
                int movie_id;
                printf("Digite o ID do filme: ");
                scanf("%d", &movie_id);
                
                int operation = 6;
                send(client_socket, &operation, sizeof(int), 0);
                send(client_socket, &movie_id, sizeof(int), 0);
                
                int result;
                recv(client_socket, &result, sizeof(int), 0);
                
                if (result == 0) {
                    Movie movie;
                    recv(client_socket, &movie, sizeof(Movie), 0);
                    
                    printf("Detalhes do Filme:\n");
                    printf("ID: %d\n", movie.id);
                    printf("Título: %s\n", movie.title);
                    printf("Diretor: %s\n", movie.director);
                    printf("Ano: %d\n", movie.release_year);
                    
                    printf("Gêneros: ");
                    for (int j = 0; j < movie.genre_count; j++) {
                        printf("%s%s", movie.genres[j], 
                               j < movie.genre_count - 1 ? ", " : "\n");
                    }
                }
                break;
            }
            case 7: { // Listar filmes por gênero
                char genre[MAX_GENRE_LENGTH];
                printf("Digite o gênero: ");
                scanf(" %[^\n]", genre);
                
                int operation = 7;
                send(client_socket, &operation, sizeof(int), 0);
                send(client_socket, genre, MAX_GENRE_LENGTH, 0);
                
                int count;
                recv(client_socket, &count, sizeof(int), 0);
                
                printf("Filmes do gênero %s:\n", genre);
                for (int i = 0; i < count; i++) {
                    Movie movie;
                    recv(client_socket, &movie, sizeof(Movie), 0);
                    
                    printf("ID: %d\n", movie.id);
                    printf("Título: %s\n", movie.title);
                    printf("Diretor: %s\n", movie.director);
                    printf("Ano: %d\n", movie.release_year);
                    
                    printf("Gêneros: ");
                    for (int j = 0; j < movie.genre_count; j++) {
                        printf("%s%s", movie.genres[j], 
                               j < movie.genre_count - 1 ? ", " : "\n");
                    }
                    printf("\n");
                }
                break;
            }
            case 8: { // Remover gênero de filme
                int movie_id;
                char genre[MAX_GENRE_LENGTH];
                printf("Digite o ID do filme: ");
                scanf("%d", &movie_id);
                printf("Digite o gênero a ser removido: ");
                scanf(" %[^\n]", genre);
                
                int operation = 8;
                send(client_socket, &operation, sizeof(int), 0);
                send(client_socket, &movie_id, sizeof(int), 0);
                send(client_socket, genre, MAX_GENRE_LENGTH, 0);
                
                int result;
                recv(client_socket, &result, sizeof(int), 0);
                printf(result == 0 ? "Gênero removido com sucesso\n" : "Erro ao remover gênero\n");
                break;
            }
        }
    } while (choice != 0);

    close(client_socket);
    return 0;
}