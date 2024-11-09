#ifndef CLIENT_H_
#define CLIENT_H_

//
// connection stuff
//
#define SERV_ADDR "34.246.184.49"
#define SERV_PORT 8080
#define HOST "34.254.242.81:8080"
#define COMMAND_LEN 100

//
// data structures
// 
// client structure and size limits
#define MAX_USERNAME 100
#define MAX_PASSWORD 100
#define MAX_COOKIE 4097
#define MAX_TOKEN 4097

typedef struct Client
{
    int sockfd;
    char username[MAX_USERNAME];
    char password[MAX_PASSWORD];
    char cookie[MAX_COOKIE];
    char token[MAX_TOKEN];
} Client;

// book structure and size limits
#define MAX_ID 11
#define MAX_TITLE 100
#define MAX_AUTHOR 100
#define MAX_GENRE 100
#define MAX_PUBLISHER 100
#define MAX_PAGE_COUNT 11

typedef enum {
    COMMAND_SUCCESS,
    COMMAND_FAILURE,
    COMMAND_RECONNECT,
    COMMAND_EXIT,
    COMMAND_INVALID
} CommandStatus;

typedef struct Book
{
    char id[MAX_ID];
    char title[MAX_TITLE];
    char author[MAX_AUTHOR];
    char genre[MAX_GENRE];
    char publisher[MAX_PUBLISHER];
    char page_count[MAX_PAGE_COUNT];
} Book;


// Read user input with prompt
void read_input(const char* prompt, char* buffer, size_t max_size);

// Create a JSON string from format and arguments
char* create_json_string(const char* format, ...);

// Handle server response and extract cookie/token if present
char* handle_response(int sockfd, char *message, char *expected, char *cookie, char *token);

// Function to handle registration
int reg_func(int sockfd, char *username, char *password, char *cookie);

/// Function to handle login
int login_func(int sockfd, char *username, char *password, char *cookie);

// Handle book related commands
int handle_book(int sockfd, char *command, char *cookie, char *token, struct Book *book);

// Reconnect to the server
int reconnect_serv(int *sockfd);

// Compute the command and execute corresponding function
CommandStatus compute_command(char *command, int sockfd, struct Client *client, struct Book *book);

#endif /* CLIENT_H_ */
