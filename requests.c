// https://gitlab.cs.pub.ro/pcom/pcom-laboratoare-public/-/blob/master/lab9/requests.c

#include <stdlib.h>     /* exit, atoi, malloc, free */
#include <stdio.h>
#include <unistd.h>     /* read, write, close */
#include <string.h>     /* memcpy, memset */
#include <sys/socket.h> /* socket, connect */
#include <netinet/in.h> /* struct sockaddr_in, struct sockaddr */
#include <netdb.h>      /* struct hostent, gethostbyname */
#include <arpa/inet.h>
#include "helpers.h"
#include "requests.h"

char *compute_get_request(char *host, char *url, char *query_params,
                            char *cookies, int cookies_count, char *token, int token_count)
{
    char *message = calloc(BUFLEN, sizeof(char));
    char *line = calloc(LINELEN, sizeof(char));

    // Step 1: write the method name, URL, request params (if any) and protocol type
    if (query_params != NULL) {
        sprintf(line, "GET %s%s HTTP/1.1", url, query_params);
    } else {
        sprintf(line, "GET %s HTTP/1.1", url);
    }

    compute_message(message, line);

    // Step 2: add the host
    sprintf(line, "HOST: %s", host);
    compute_message(message, line);


    // Step 3 (optional): add headers and/or cookies, according to the protocol format
    if (token != NULL && token_count > 0) {
        sprintf(line, "Authorization: Bearer %s", token);
        compute_message(message, line);
    }

    if (cookies != NULL && cookies_count > 0) {
        sprintf(line, "Cookie: %s", cookies);
        compute_message(message, line);
    }

     // Step 4: add final new line
    compute_message(message, "");
    free(line);
    return message;
}

char *compute_post_request(char *host, char *url, char* content_type, char **body_data,
                            int body_data_fields_count, char *cookies, int cookies_count, char *token, int token_count)
{
    char *message = calloc(BUFLEN, sizeof(char));
    char *line = calloc(LINELEN, sizeof(char));

    // Step 1: write the method name, URL and protocol type
    sprintf(line, "POST %s HTTP/1.1", url);
    compute_message(message, line);
    
    // Step 2: add the host
    sprintf(line, "HOST: %s", host);
    compute_message(message, line);


    /* Step 3: add necessary headers (Content-Type and Content-Length are mandatory)
            in order to write Content-Length you must first compute the message size
    */

    if(token != NULL && token_count > 0) {
        sprintf(line, "Authorization: Bearer %s", token);
        compute_message(message, line);
    }

    sprintf(line, "Content-Type: %s", content_type);
    compute_message(message, line);

    if (*body_data != NULL) {
        int size = strlen(*body_data);
        sprintf(line, "Content-Length: %d", size);
        compute_message(message, line);
        
        strcat(message, "\r\n");
        strcat(message, *body_data);
    }
    
     // Step 4 (optional): add cookies
    if (cookies != NULL && cookies_count > 0) {
        sprintf(line, "Cookie: %s", cookies);
        compute_message(message, line);
    }

    // Step 5: add new line at end of header
    compute_message(message, "");

    free(line);
    return message;
}

char *compute_delete_request(char *host, char *url, char *query_params,
                            char *cookies, int cookies_count, char *token, int token_count)
{
    char *message = calloc(BUFLEN, sizeof(char));
    char *line = calloc(LINELEN, sizeof(char));

    // Step 1: write the method name, URL, and protocol type
    if (query_params != NULL) {
        sprintf(line, "DELETE %s%s HTTP/1.1", url, query_params);
    } else {
        sprintf(line, "DELETE %s HTTP/1.1", url);
    }

    compute_message(message, line);

    // Step 2: add the host
    sprintf(line, "HOST: %s", host);
    compute_message(message, line);

    // Step 3 (optional): add headers and/or cookies, according to the protocol format
    if (cookies != NULL && cookies_count > 0) {
        sprintf(line, "Cookie: %s", cookies);
        compute_message(message, line);
    }
    
    if(token != NULL && token_count > 0) {
        sprintf(line, "Authorization: Bearer %s", token);
        compute_message(message, line);
    }

    // Step 4: add final new line
    compute_message(message, "");

    free(line);
    return message;
}