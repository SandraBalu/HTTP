# CLIENT WEB COMMUNICATION WITH REST API


## General Descrption 

This C client application is designed to communicate with a server running a 
library management system. It supports commands for user registration, login, 
library access, and book management, ensuring efficient and secure interactions


## Key Functionalities

The application includes several key functionalities. Command parsing is 
centralized, allowing user input to be dispatched to specific handler functions.
It uses epoll to manage the server connection's time-to-live, automatically 
handling reconnections as needed. Error handling is robust, with the 
implementation of error codes and flags to manage connection issues, including
automatic reconnection logic. JSON parsing is handled by the Parson library, 
which facilitates easy integration with potential GUI implementations. The 
application employs dynamic memory allocation and ensures memory safety throughout
its operations, preventing memory leaks and buffer overflows.


## Usage Workflow

When the user enters a command, the command parser identifies the command and 
calls the corresponding handler function. The handler function formats an HTTP 
request and sends it to the server. The server's response is then received, 
parsed, and appropriate actions are taken, such as displaying information or 
handling errors.


## Resources

The Parson library, a lightweight JSON library used for parsing and handling 
JSON data, is an integral part of this application. More information about 
Parson can be found at [Parson](https://github.com/kgabis/parson). The client 
is based on the code provided in the 9th lab, which can be found at 
[Lab 9](https://pcom.pages.upb.ro/labs/lab9/lecture.html).
