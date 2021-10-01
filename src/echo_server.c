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
#include <sys/stat.h>
#include <time.h>
#include "parse.h"
#include <errno.h>


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

const char *get_file_ext(const char *filename) {
    const char *dot = strrchr(filename, '.');
    if(!dot || dot == filename) return "";
    return dot;
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
    char *www;
    if (argc > 1){
        int port = atoi(argv[1]);
        addr.sin_port = htons(port);
        www = argv[4];
    }else{
        addr.sin_port = htons(ECHO_PORT);
    }
    addr.sin_addr.s_addr = INADDR_ANY;

    int optval = 1;
    int optlen = sizeof(optval);
    setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, (const char*)&optval, optlen);

    printf("bind\n");
    /* servers bind sockets to ports---notify the OS they accept connections */
    if (bind(sock, (struct sockaddr *) &addr, sizeof(addr))==-1)
    {
        close_socket(sock);
        fprintf(stderr, "Failed binding socket.\n");
        return EXIT_FAILURE;
    }

    printf("listen\n");
    if (listen(sock, 5))
    {
        close_socket(sock);
        fprintf(stderr, "Error listening on socket.\n");
        return EXIT_FAILURE;
    }
    printf("listened\n");

    struct timeval tv;
    tv.tv_sec = 40;
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
        if (select(FD_SETSIZE, &sockets_to_process, NULL, NULL, &tv)<0){
            perror("select error");
            return EXIT_FAILURE;
        };
        char RESPONSE[BUF_SIZE];
        // char BAD_REQUEST_RESPONSE[29] = "HTTP/1.1 400 Bad Request\r\n\r\n";
        // char NOT_FOUND_RESPONSE[27] = "HTTP/1.1 404 Not found\r\n\r\n";
        // char Connection_Timeout_RESPONSE[36] = "HTTP/1.1 408 Connection timeout\r\n\r\n";
        // char Unsupported_Method_RESPONSE[38] = "HTTP/1.1 501 Method Unimplemented\r\n\r\n";
        // char BAD_VERSION_RESPONSE[36] = "HTTP/1.1 505 Bad version number\r\n\r\n";

        // printf("max_socket\n");
        // printf("%d", max_socket);

        for (int i=0; i < FD_SETSIZE; i++){
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
                    // if (client_sock > max_socket){
                    //     max_socket = client_sock;
                    // }
                }else{
                    readret = 0;
                    char content_len;
                    
                    readret = recv(i, buf, BUF_SIZE, 0);
                    if (readret <= 0)
                    {
                        if (readret==-1){
                            printf("The last error message is: %s\n", strerror(errno));
                            fprintf(stderr, "Error reading from client socket.\n");
                            close_socket(i);
                            close_socket(sock);
                            FD_CLR(i, &sockets);
                            return EXIT_FAILURE;
                        }
                    }else{     
                        printf("buf to parse %s\n", buf);
                        FILE *stream;
                        stream = fopen(www, "r");
                        http_parser *parse_res = parse(buf, BUF_SIZE, i);
                        if (parse_res==NULL){
                            strcat(RESPONSE, "HTTP/1.1 400 Bad Request\r\n\r\n");
                            readret += sizeof("HTTP/1.1 400 Bad Request\r\n\r\n");
                        }else{
                            char *dir = strcat(www, parse_res->http_uri);
                            if (parse_res->status_code==400){
                                strcat(RESPONSE, "HTTP/1.1 400 Bad Request\r\n\r\n");
                                readret += sizeof("HTTP/1.1 400 Bad Request\r\n\r\n");
                            }else if (parse_res->status_code==404 || (stream==NULL))
                            {
                                strcat(RESPONSE, "HTTP/1.1 404 Not found\r\n\r\n");
                                readret += sizeof("HTTP/1.1 404 Not found\r\n\r\n");
                            }else if (parse_res->status_code==505){
                                strcat(RESPONSE, "HTTP/1.1 505 Bad version number\r\n\r\n");
                                readret += sizeof("HTTP/1.1 505 Bad version number\r\n\r\n");
                            // }else if (parse_res==408)
                            // {
                            //     memcpy(buf, Connection_Timeout_RESPONSE, sizeof(Connection_Timeout_RESPONSE));
                            //     readret = sizeof(Connection_Timeout_RESPONSE);
                            }else if (parse_res->status_code==501 || (strcmp(parse_res->method, "POST")==0 && parse_res->content_length==NULL)){
                                strcat(RESPONSE, "HTTP/1.1 501 Method Unimplemented\r\n\r\n");
                            }else if (parse_res->status_code==200){
                                printf("%s%d\n", "parse.c content length: ", parse_res->content_length);
                                    strcat(RESPONSE, "HTTP/1.1 200 OK\r\nConnection:");
                                    readret += sizeof("HTTP/1.1 200 OK\r\nConnection:");
                            }
                            // printf("%s%s\n", "conn header", parse_res->conn_header);
                        
                        }
                        printf("%s %d\n", "content len before asprintf:", parse_res->content_length);
                        // if (asprintf(&content_len, "%d", parse_res->content_length) == -1) {
                        //     perror("asprintf");
                        // }
                        content_len = parse_res->content_length+'0';
                        // asprintf(&content_len, "%d", parse_res->content_length);
                        printf("%s %c\n", "content len:", content_len);
                        strcat(RESPONSE, parse_res->conn_header);
                        readret += sizeof(parse_res->conn_header);

                        strcat(RESPONSE, "\r\nServer: Liso/1.0");
                        readret += sizeof("\r\nServer: Liso/1.0");

                        strcat(RESPONSE, "\r\nContent-Length:");
                        readret += sizeof("\r\nContent-Length:");
                        strcat(RESPONSE, &content_len);
                        printf("%s %d\n", "readret without content :", readret);
                        readret += sizeof(content_len);

                        struct stat attr;
                        stat(i, &attr);
                        
                        char *last_modified_time = ctime(&attr.st_mtim);
                        strcat(RESPONSE, "\r\nLast-Modified: ");
                        strcat(RESPONSE, last_modified_time);
                        readret += sizeof("\r\nLast-Modified: ")+sizeof(last_modified_time);

                        time_t rawtime;
                        struct tm *info;
                        time(&rawtime);
                        /* Get GMT time */
                        info = gmtime(&rawtime);
                        // day-name, day month year hour:minute:second GMT
                        char date[BUF_SIZE];
                        char *format = "%a, %d %b %Y %T GMT";
                        // const struct tm *restrict timeptr;
                        strftime(date, BUF_SIZE, format, info);
                        strcat(RESPONSE, "\r\nDate: ");
                        strcat(RESPONSE, date);
                        readret += sizeof("\r\nDate: ")+sizeof(date);

                        // text/html, text/css, image/png, image/jpeg, image/gif, etc
                        char *content_type;
                        char *suffix;
                        suffix = get_file_ext(parse_res->http_uri);
                        if (strcmp(suffix, ".html")==0){
                            content_type = "text/html";
                        }else if (strcmp(suffix, ".css")==0){
                            content_type = "text/css";
                        }else if (strcmp(suffix, ".png")==0){
                            content_type = "image/png";
                        }else if (strcmp(suffix, ".jpg")==0){
                            content_type = "image/jpeg";
                        }else if (strcmp(suffix, ".gif")==0){
                            content_type = "image/gif";
                        }

                        strcat(RESPONSE, "\r\nContent-Type: ");
                        readret += sizeof("\r\nContent-Type: ");
                        strcat(RESPONSE, content_type);
                        readret += sizeof(content_type);
                        strcat(RESPONSE, "\r\n\r\n");
                        readret += sizeof("\r\n\r\n");
                        // printf("%s %d\n", "readret with content :", readret);
                        
                        int sent_len;
                        if (strcmp(parse_res->method, "HEAD")){ // not HEAD request

                            // open(i, O_RDONLY);
                            char content[BUF_SIZE];
                            // read(i, content, content_len);
                            
                            
                            fgets(content, BUF_SIZE, stream);
                            printf("%s %s\n", "content:", content);
                            content_len = sizeof(content);
                            
                            // strcat(RESPONSE, "\r\nmessage-body");
                            // readret += sizeof("\r\nmessage-body: ");
                            if (content_len + readret <= BUF_SIZE){
                                strcat(RESPONSE, content);
                                readret += content_len;
                                sent_len = send(i, RESPONSE, readret, 0);
                                printf("%s", "here");
                                if (sent_len != readret){
                                    // printf("%s\n", "closing server sock");
                                    // close_socket(sock);
                                    close_socket(i);
                                    fprintf(stderr, "Error sending to client.\n");
                                    return EXIT_FAILURE;
                                }
                            }else{
                                strcat(RESPONSE, content);
                                readret += content_len;
                                printf("response to send %s\n", RESPONSE);
                                printf("%s%d\n", "readret : ", readret);
                                while (readret > 0){
                                    if (readret > BUF_SIZE){
                                        sent_len = send(i, RESPONSE, BUF_SIZE, 0);
                                        printf("%s\n", "heree");

                                        if (sent_len != BUF_SIZE){
                                            // printf("%s\n", "closing server sock");
                                            // close_socket(sock);
                                            close_socket(i);
                                            fprintf(stderr, "Error sending to client.\n");
                                            return EXIT_FAILURE;
                                        }
                                    }else{
                                        sent_len = send(i, RESPONSE, readret, 0);
                                        printf("%s", "hereee");
                                        if (sent_len != readret){
                                            // printf("%s\n", "closing server sock");
                                            // close_socket(sock);
                                            close_socket(i);
                                            fprintf(stderr, "Error sending to client.\n");
                                            return EXIT_FAILURE;
                                        }
                                    }
                                    printf("%s%d\n", "sent len", sent_len);
                            
                                    readret -= BUF_SIZE;
                                }
                            }
                        }else{  // HEAD request
                            
                            
                            printf("response to send %s\n", RESPONSE);
                            printf("%s%d\n", "readret : ", readret);
                            sent_len = send(i, RESPONSE, readret, 0);
                            if (sent_len != readret){
                                // printf("%s\n", "closing server sock");
                                // close_socket(sock);
                                close_socket(i);
                                fprintf(stderr, "Error sending to client.\n");
                                return EXIT_FAILURE;
                            }
                        }
                        // free(content_len);
                        
                        
                        
                        if (parse_res && strcmp(parse_res->conn_header, "close")==0){
                            if (close_socket(i)){
                                close_socket(sock);
                                fprintf(stderr, "Error closing client socket.\n");
                                return EXIT_FAILURE;
                            }
                        }
                        memset(buf, 0, BUF_SIZE);
                    }
                }
            }
        }
    }
    close_socket(sock);
}