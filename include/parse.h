#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define SUCCESS 0

//Header field
typedef struct
{
	char header_name[4096];
	char header_value[4096];
} Request_header;

// //HTTP Request Header
// typedef struct
// {
// 	char http_version[50];
// 	char http_method[50];
// 	char http_uri[4096];
// 	Request_header *headers;
// 	int header_count;
// } Request;

typedef struct {
  /** PRIVATE **/
  unsigned char type : 2;     /* enum http_parser_type */
  unsigned char flags : 6;    /* F_* values from 'flags' enum; semi-public */
  unsigned char state;        /* enum state from http_parser.c */
  unsigned char header_state; /* enum header_state from http_parser.c */
  unsigned char index;        /* index into current matcher */

//   uint32_t nread;          /* # bytes read in various scenarios */
  int content_length; /* # bytes in body (0 if no Content-Length header) */

  /** READ-ONLY **/
  unsigned short http_major;
  unsigned short http_minor;
  unsigned short status_code; /* responses only */
  char http_version[50];
  char method[50];       /* requests only */
  char http_uri[4096];
	Request_header *headers;
	int header_count;
} http_parser;

http_parser* parse(char *buffer, int size,int socketFd);

// functions decalred in parser.y
int yyparse();
void set_parsing_options(char *buf, size_t i, http_parser *request);
