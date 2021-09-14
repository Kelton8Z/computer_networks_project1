/******************************************************************************
* echo_server.c                                                               *
*                                                                             *
* Description: This file contains the C source code for an echo server.  The  *
*              server runs on a hard-coded port and simply write back anything*
*              sent to it by connected clients.  It does not support          *
*              concurrent clients.                                            *
*                                                                             *
* Authors: Athula Balachandran <abalacha@cs.cmu.edu>,                         *
*          Wolf Richter <wolf@cs.cmu.edu>                                     *
*                                                                             *
*******************************************************************************/

#include <netinet/in.h>
#include <netinet/ip.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include "parse.h"

#define ECHO_PORT 9999
#define BUF_SIZE 4096

int close_socket(int sock)
{
    if (close(sock))
    {
        fprintf(stderr, "Failed closing socket.\n");
        return 1;
    }
    return 0;
}

int main(int argc, char* argv[])
{
    int sock, client_sock;
    ssize_t readret;
    socklen_t cli_size;
    struct sockaddr_in addr, cli_addr;
    char buf[BUF_SIZE];

    fprintf(stdout, "----- Echo Server -----\n");
    
    /* all networked programs must create a socket */
    if ((sock = socket(PF_INET, SOCK_STREAM, 0)) == -1)
    {
        fprintf(stderr, "Failed creating socket.\n");
        return EXIT_FAILURE;
    }

    addr.sin_family = AF_INET;
    if (argc > 1){
        int port = atoi(argv[1]);
        addr.sin_port = htons(port);
    }else{
        addr.sin_port = htons(ECHO_PORT);
    }
    addr.sin_addr.s_addr = INADDR_ANY;

    int optval = 1;
    int optlen = sizeof(optval);
    setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, (const char*)&optval, optlen);

    /* servers bind sockets to ports---notify the OS they accept connections */
    if (bind(sock, (struct sockaddr *) &addr, sizeof(addr))==-1)
    {
        close_socket(sock);
        fprintf(stderr, "Failed binding socket.\n");
        return EXIT_FAILURE;
    }

    if (listen(sock, 5))
    {
        close_socket(sock);
        fprintf(stderr, "Error listening on socket.\n");
        return EXIT_FAILURE;
    }

    struct timeval tv;
    tv.tv_sec = 5;
    tv.tv_usec = 0;
    fd_set readfds;
    fd_set sockets, sockets_to_process;
    FD_ZERO(&sockets);
    FD_SET(sock, &sockets);
    int max_socket = sock;

    /* finally, loop waiting for input and then write it back */
    while (1)
    {
        cli_size = sizeof(cli_addr);
        
        sockets_to_process = sockets;
        if (select(max_socket, &sockets_to_process, NULL, NULL, NULL)<0){
            perror("select error");
            return EXIT_FAILURE;
        };

        char BAD_REQUEST_RESPONSE[28] = "HTTP/1.1 400 Bad Request\r\n\r\n";
        // printf("sock\n");
        // printf("%d", sock);
        // printf("max_socket\n");
        // printf("%d", max_socket);

        for (int i=0; i < max_socket; i++){
            if (FD_ISSET(i, &sockets_to_process)){
                if (i==sock){
                    client_sock = accept(sock, (struct sockaddr *) &cli_addr, &cli_size);
                    if ((client_sock) == -1)
                    {
                        close(sock);
                        fprintf(stderr, "Error accepting connection.\n");
                        return EXIT_FAILURE;
                    }
                    FD_SET(client_sock, &sockets);
                    if (client_sock > max_socket){
                        max_socket = client_sock;
                    }
                }else{
                    readret = 0;

                    readret = recv(i, buf, BUF_SIZE, 0);
                    if (readret <= 0)
                    {
                        close_socket(i);
                        close_socket(sock);
                        fprintf(stderr, "Error reading from client socket.\n");
                        return EXIT_FAILURE;
                    }

                    if (parse(buf, BUF_SIZE, i)==NULL){
                        memcpy(buf, BAD_REQUEST_RESPONSE, sizeof(BAD_REQUEST_RESPONSE));
                        readret = sizeof(BAD_REQUEST_RESPONSE);
                    }
                    printf("%s\n", buf);
                    if (send(i, buf, readret, 0) != readret)
                    {
                        close_socket(i);
                        close_socket(sock);
                        fprintf(stderr, "Error sending to client.\n");
                        return EXIT_FAILURE;
                    }
                    memset(buf, 0, BUF_SIZE);

                    if (close_socket(i))
                    {
                        close_socket(sock);
                        fprintf(stderr, "Error closing client socket.\n");
                        return EXIT_FAILURE;
                    }

                    FD_CLR(i, &sockets);
                    return EXIT_SUCCESS;
                }
            }
        }
    }
    close_socket(sock);
}
