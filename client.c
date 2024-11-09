#include <stdio.h>      /* printf, fgets, sprintf */
#include <stdlib.h>     /* exit, atoi, malloc, free */
#include <unistd.h>     /* read, write, close */
#include <string.h>     /* memcpy, memset */
#include <sys/socket.h> /* socket, connect */
#include <netinet/in.h> /* struct sockaddr_in, struct sockaddr */
#include <netdb.h>      /* struct hostent, gethostbyname */
#include <arpa/inet.h>
#include <sys/epoll.h>

#include "helpers.h"
#include "requests.h"
#include "buffer.h"
#include "client.h"
#include <sys/select.h>
#include <stdarg.h>

void read_input(const char* prompt, char* buffer, size_t max_size) {
    printf("%s", prompt);
    if (fgets(buffer, max_size, stdin) != NULL) {
        size_t len = strlen(buffer);
        if (len > 0 && buffer[len - 1] == '\n') {
            buffer[len - 1] = '\0';
        }
    }
}

char* create_json_string(const char* format, ...) {
    char* json_string = (char*)malloc(BUFSIZ);
    va_list args;
    va_start(args, format);
    vsprintf(json_string, format, args);
    va_end(args);
    return json_string;
}

char* handle_response(int sockfd, char *message, char *expected, char *cookie, char *token) {
    send_to_server(sockfd, message);
    free(message);

    char *response = receive_from_server(sockfd);
    if (strcmp(response, "") == 0) {
        free(response);
        return NULL;
    }

    if (strstr(response, "error") != NULL) {
        free(response);
        printf("ERROR : %s failed\n", expected);
        return NULL;
    }

    if (cookie) {
        char *cookie_start = strstr(response, "Set-Cookie: ");
        cookie_start += strlen("Set-Cookie: ");
        char *cookie_end = strstr(cookie_start, ";");
        *cookie_end = '\0';
        memcpy(cookie, cookie_start, strlen(cookie_start) + 1);
    }

    if (token) {
        char *token_start = strstr(response, "{\"token\":\"");
        token_start += strlen("{\"token\":\"");
        char *token_end = strstr(token_start, "\"}");
        *token_end = '\0';
        memcpy(token, token_start, strlen(token_start) + 1);
    }

    return response;
}

int reg_func(int sockfd, char *username, char *password, char *cookie) {
    char* json_string = create_json_string("{\"username\":\"%s\",\"password\":\"%s\"}", username, password);
    char *message = compute_post_request(HOST, "/api/v1/tema/auth/register", "application/json", &json_string, strlen(json_string), NULL, 0, NULL, 0);

    char *response = handle_response(sockfd, message, "Registration", NULL, NULL);
    free(json_string);

    if (response == NULL) {
        return COMMAND_FAILURE;
    }

    printf("SUCCESS : Registered successfully\n");
    free(response);
    return COMMAND_SUCCESS;
}

int login_func(int sockfd, char *username, char *password, char *cookie) {
    char* json_string = create_json_string("{\"username\":\"%s\",\"password\":\"%s\"}", username, password);
    char *message = compute_post_request(HOST, "/api/v1/tema/auth/login", "application/json", &json_string, strlen(json_string), NULL, 0, NULL, 0);

    char *response = handle_response(sockfd, message, "Login", cookie, NULL);
    free(json_string);

    if (response == NULL) {
        return COMMAND_FAILURE;
    }

    printf("SUCCESS : Logged in successfully\n");
    free(response);
    return COMMAND_SUCCESS;
}

int handle_library(int sockfd, const char *command, char *cookie, char *token) {
    char *message = NULL;
    char *response = NULL;

    if (strcmp(command, "enter_library") == 0) {
        message = compute_get_request(HOST, "/api/v1/tema/library/access", NULL, cookie, 1, NULL, 0);
        response = handle_response(sockfd, message, "Enter library", NULL, token);
    } else if (strcmp(command, "get_books") == 0) {
        message = compute_get_request(HOST, "/api/v1/tema/library/books", NULL, cookie, 1, token, 1);
        response = handle_response(sockfd, message, "Get books", NULL, NULL);
    }

    if (response == NULL) {
        return COMMAND_FAILURE;
    }

    if (strcmp(command, "get_books") == 0) {
        char *json_start = strstr(response, "[");
        printf("%s\n", json_start);
    } else {
        printf("SUCCESS : %s successfully\n", "Entered library");
    }

    free(response);
    return COMMAND_SUCCESS;
}

int handle_book(int sockfd, char *command, char *cookie, char *token, struct Book *book) {
    char *message = NULL;
    char *response = NULL;

    if (strcmp(command, "add_book") == 0) {
        char* json_string = create_json_string("{\"title\":\"%s\",\"author\":\"%s\",\"genre\":\"%s\",\"page_count\":%s,\"publisher\":\"%s\"}",
                                                book->title, book->author, book->genre, book->page_count, book->publisher);
        message = compute_post_request(HOST, "/api/v1/tema/library/books", "application/json", &json_string, strlen(json_string), cookie, 1, token, 1);
        response = handle_response(sockfd, message, "Add book", NULL, NULL);
        free(json_string);

    } else if (strcmp(command, "get_book") == 0) {
        message = compute_get_request(HOST, "/api/v1/tema/library/books/", book->id, cookie, 1, token, 1);
        response = handle_response(sockfd, message, "Get book", NULL, NULL);

    } else if (strcmp(command, "get_books") == 0) {
        message = compute_get_request(HOST, "/api/v1/tema/library/books", NULL, cookie, 1, token, 1);
        response = handle_response(sockfd, message, "Get books", NULL, NULL);

    } else if (strcmp(command, "delete_book") == 0) {
        message = compute_delete_request(HOST, "/api/v1/tema/library/books/", book->id, cookie, 1, token, 1);
        response = handle_response(sockfd, message, "Delete book", NULL, NULL);
    }

    if (response == NULL) {
        return COMMAND_FAILURE;
    }

    if (strcmp(command, "get_books") == 0 || strcmp(command, "get_book") == 0) {
        char *json_start = strstr(response, strcmp(command, "get_books") == 0 ? "[" : "{");
        printf("%s\n", json_start);
    } else {
        printf("SUCCESS : %s successfully\n", strcmp(command, "add_book") == 0 ? "Book added" : "Book deleted");
    }

    free(response);
    return COMMAND_SUCCESS;
}

int reconnect_serv(int *sockfd) {
    close(*sockfd); // Close the current socket connection

    *sockfd = open_connection(SERV_ADDR, SERV_PORT, AF_INET, SOCK_STREAM, 0);
    if (*sockfd < 0) {
        perror("open_connection");
        return -1;
    }

    return 0;
}

CommandStatus compute_command(char *command, int sockfd, struct Client *client, struct Book *book) {
    if (strcmp(command, "register") == 0) {
        if (strcmp(client->cookie, "") != 0) {
            printf("ERROR : You are already logged in\n");
            return COMMAND_FAILURE;
        }

        read_input("username=", client->username, MAX_USERNAME);
        read_input("password=", client->password, MAX_PASSWORD);
        
        if (strchr(client->username, ' ') != NULL) {
            printf("ERROR : Username cannot contain spaces\n");
            memset(client, 0, sizeof(*client));
            return COMMAND_FAILURE;
        }
        
        if (strchr(client->password, ' ') != NULL) {
            printf("ERROR : Password cannot contain spaces\n");
            memset(client, 0, sizeof(*client));
            return COMMAND_FAILURE;
        }

        return reg_func(sockfd, client->username, client->password, NULL);

    } else if (strcmp(command, "login") == 0) {
        read_input("username=", client->username, MAX_USERNAME);
        read_input("password=", client->password, MAX_PASSWORD);

        if (strchr(client->username, ' ') != NULL) {
            printf("ERROR : Username cannot contain spaces\n");
            memset(client, 0, sizeof(*client));
            return COMMAND_FAILURE;
        }
        
        if (strchr(client->password, ' ') != NULL) {
            printf("ERROR : Password cannot contain spaces\n");
            memset(client, 0, sizeof(*client));
            return COMMAND_FAILURE;
        }

        return login_func(sockfd, client->username, client->password, client->cookie);

    } else if (strcmp(command, "enter_library") == 0 || strcmp(command, "get_books") == 0) {
        if (strlen(client->cookie) == 0) {
            printf("ERROR : You must be logged in to perform this action\n");
            return COMMAND_FAILURE;
        }
        return handle_library(sockfd, command, client->cookie, client->token);

    } else if (strcmp(command, "get_book") == 0 || strcmp(command, "add_book") == 0 || strcmp(command, "delete_book") == 0) {
        if (strlen(client->cookie) == 0) {
            printf("ERROR : You must be logged in to perform this action\n");
            return COMMAND_FAILURE;
        }
        if (strlen(client->token) == 0) {
            printf("ERROR : You must enter the library to perform this action\n");
            return COMMAND_FAILURE;
        }

        if (strcmp(command, "get_book") == 0 || strcmp(command, "delete_book") == 0) {
            read_input("id=", book->id, MAX_ID);

            if (strspn(book->id, "0123456789") != strlen(book->id)) {
                printf("ERROR : Id must contain only digits\n");
                memset(book->id, 0, MAX_ID);
                return COMMAND_FAILURE;
            }
        }

        if (strcmp(command, "add_book") == 0) {
            read_input("title=", book->title, MAX_TITLE);
            read_input("author=", book->author, MAX_AUTHOR);
            read_input("genre=", book->genre, MAX_GENRE);
            read_input("publisher=", book->publisher, MAX_PUBLISHER);
            read_input("page_count=", book->page_count, MAX_PAGE_COUNT);

            if (strspn(book->page_count, "0123456789") != strlen(book->page_count)) {
                printf("ERROR : Page count must contain only digits\n");
                memset(book, 0, sizeof(*book));
                return COMMAND_FAILURE;
            }
        }

        return handle_book(sockfd, command, client->cookie, client->token, book);

    } else if (strcmp(command, "logout") == 0) {
        if (strlen(client->cookie) == 0) {
            printf("ERROR : You must be logged in to logout\n");
            return COMMAND_FAILURE;
        }
        printf("SUCCESS: Successfully logged out!\n");
        memset(client, 0, sizeof(*client));
        return COMMAND_SUCCESS;

    } else if (strcmp(command, "exit") == 0) {
        return COMMAND_EXIT;
    } else {
        printf("ERROR : Invalid command\n");
        return COMMAND_INVALID;
    }
}

int main(int argc, char *argv[]) {
    setvbuf(stdout, NULL, _IONBF, BUFSIZ);
    char *command = calloc(COMMAND_LEN, sizeof(char));

    Client client;
    Book book;

    int sockfd = open_connection(SERV_ADDR, SERV_PORT, AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("open_connection");
        exit(EXIT_FAILURE);
    }

    fd_set readfds;
    int maxfd = sockfd;

    while (1) {
        FD_ZERO(&readfds);
        FD_SET(STDIN_FILENO, &readfds);
        FD_SET(sockfd, &readfds);

        int activity = select(maxfd + 1, &readfds, NULL, NULL, NULL);
        if (activity < 0) {
            perror("select");
            free(command);
            close(sockfd);
            exit(EXIT_FAILURE);
        }

        if (FD_ISSET(STDIN_FILENO, &readfds)) {
            read(STDIN_FILENO, command, COMMAND_LEN);
            command[strlen(command) - 1] = '\0';

            CommandStatus status = compute_command(command, sockfd, &client, &book);
            while (status == COMMAND_RECONNECT) {
                reconnect_serv(&sockfd);
                status = compute_command(command, sockfd, &client, &book);
            }
            if (status == COMMAND_EXIT) {
                free(command);
                close(sockfd);
                return 0;
            }
            memset(command, 0, COMMAND_LEN);
            fflush(STDIN_FILENO);
        }

        if (FD_ISSET(sockfd, &readfds)) {
            if (reconnect_serv(&sockfd) < 0) {
                free(command);
                perror("reconnect_serv");
                close(sockfd);
                exit(EXIT_FAILURE);
            }
        }
    }

    free(command);
    close(sockfd);
    return 0;
}
