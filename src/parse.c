#include "parse.h"

/**
* Given a char buffer returns the parsed request headers
*/
http_parser * parse(char *buffer, int size, int socketFd) {
  //Differant states in the state machine
	enum {
		STATE_START = 0, STATE_CR, STATE_CRLF, STATE_CRLFCR, STATE_CRLFCRLF
	};

	int i = 0, state;
	size_t offset = 0;
	char ch;
	char buf[8192];
	memset(buf, 0, 8192);

	state = STATE_START;
	while (state != STATE_CRLFCRLF) {
		char expected = 0;

		if (i == size)
			break;

		ch = buffer[i++];
		buf[offset++] = ch;

		switch (state) {
		case STATE_START:
		case STATE_CRLF:
			expected = '\r';
			break;
		case STATE_CR:
		case STATE_CRLFCR:
			expected = '\n';
			break;
		default:
			state = STATE_START;
			continue;
		}

		if (ch == expected)
			state++;
		else
			state = STATE_START;

	}

    //Valid End State
	if (state == STATE_CRLFCRLF) {
		http_parser *request = (http_parser *) malloc(sizeof(http_parser));
        request->header_count=0;
        //TODO You will need to handle resizing this in parser.y
        request->headers = (Request_header *) malloc(sizeof(Request_header)*1);
		// request->http_uri = (char *) malloc(sizeof(request->http_uri));
		// request->http_version = (char *) malloc(sizeof(request->http_version));

		yyrestart();
		set_parsing_options(buf, i, request);
		printf("Got to end state\n");
		yyparse();
		printf("%s%d\n", "parse.c content length: ", request->content_length);
		return request;
		// if (yyparse() == SUCCESS) {
        //     return request;
		// }
	}
    //TODO Handle Malformed Requests
    printf("Parsing Failed\n");
	// snprintf(buf, sizeof(buf), "HTTP/1.1 400 Bad Request\r\n\r\n");
	return NULL;
}

