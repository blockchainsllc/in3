#include <stdio.h>  /* printf, sprintf */
#include <stdlib.h> /* exit, atoi, malloc, free */
#include <string.h> /* memcpy, memset */
#include <unistd.h> /* read, write, close */
#ifdef _WIN32
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib") //Winsock Library
#else
#include <netdb.h>      /* struct hostent, gethostbyname */
#include <netinet/in.h> /* struct sockaddr_in, struct sockaddr */
#include <sys/socket.h> /* socket, connect */
#endif
#include "../../core/client/client.h"
#include "in3_http.h"
void error(const char* msg) {
  perror(msg);
  exit(0);
}

in3_ret_t send_http(char** urls, int urls_len, char* payload, in3_response_t* result) {
  for (int n = 0; n < urls_len; n++) {
    struct hostent*    server;
    struct sockaddr_in serv_addr;
    int                bytes, sent, received, total;
    char               message[strlen(payload) + 200], response[4096], *url = urls[n], host[256];

    // parse url
    // http://test.com/path

    if (strncmp(url, "http://", 7)) {
      sb_add_chars(&result[n].error, "invalid url must sart with http");
      continue;
    }

    // parse url
    char* path = strchr(url + 7, '/');
    if (path)
      strncpy(host, url + 7, path - (url + 7));
    else {
      path = "/";
      strcpy(host, url + 7);
    }
    int portno = 80;

    sprintf(message, "POST %s HTTP/1.0\r\nHost: %s\r\nContent-Type: application/json\r\nContent-Length: %d\r\n\r\n%s", path, host, (int) strlen(payload), payload);
    total = strlen(message);

/* create the socket */
#ifdef _WIN32
    WSADATA wsa;
    SOCKET  s;

    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
      sb_add_chars(&result[n].error, "no socket availabe");
      continue;
    }

    //Create a socket
    if ((s = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
      sb_add_chars(&result[n].error, "Could not create socket");
      continue;
    }

    server                    = gethostbyname(host);
    serv_addr.sin_addr.s_addr = inet_addr(server->h_addr);
    serv_addr.sin_family      = AF_INET;
    serv_addr.sin_port        = htons(portno);
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port   = htons(portno);
    memcpy(&serv_addr.sin_addr.s_addr, server->h_addr, server->h_length);
    //Connect to remote server
    if (connect(s, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0) {
      sb_add_chars(&result[n].error, "Connection failed");
      continue;
    }

    if (send(s, message, strlen(message), 0) < 0) {
      sb_add_chars(&result[n].error, "Send failed");
      continue;
    }

    //Receive a reply from the server
    if ((received = recv(s, response, 2000, 0)) == SOCKET_ERROR) {
      sb_add_chars(&result[n].error, "receive failed");
      continue;
    }

    //Add a NULL terminating character to make it a proper string before printing
    response[received] = '\0';
    sb_add_chars(&result[n].result, response);

    closesocket(s);
    WSACleanup();
#else
    int sockfd;
    server = gethostbyname(host);
    if (server == NULL) {
      sb_add_chars(&result[n].error, "no such host");
      continue;
    }
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
      sb_add_chars(&result[n].error, "ERROR opening socket");
      continue;
    }
    /* fill in the structure */
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port   = htons(portno);
    memcpy(&serv_addr.sin_addr.s_addr, server->h_addr_list[0], server->h_length);
    /* connect the socket */
    if (connect(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0) {
      sb_add_chars(&result[n].error, "ERROR connecting");
      continue;
    }
    /* send the request */

    sent = 0;
    do {
      bytes = write(sockfd, message + sent, total - sent);
      if (bytes < 0) {
        sb_add_chars(&result[n].error, "ERROR writing message to socket");
        continue;
      }
      if (bytes == 0)
        break;
      sent += bytes;
    } while (sent < total);
    /* receive the response */
    memset(response, 0, sizeof(response));
    total    = sizeof(response) - 1;
    received = 0;
    do {
      memset(response, 0, sizeof(response));
      bytes = recv(sockfd, response, 1024, 0);
      if (bytes < 0) {
        sb_add_chars(&result[n].error, "ERROR reading response from socket");
        continue;
      }
      if (bytes == 0)
        break;
      sb_add_chars(&result[n].result, response);
      received += bytes;
    } while (1);

    if (received == total) {
      sb_add_chars(&result[n].error, "ERROR storing complete response from socket");
      continue;
    }

    /* close the socket */
    close(sockfd);

#endif

    // now evaluate the response

    char *res = result[n].result.data, *header = strstr(res, "\r\n\r\n"), *body = header + 4;
    if (!header) {
      sb_add_chars(&result[n].error, "ERROR invalid response");
      continue;
    }
    *header = 0;
    header  = strstr(res, "\r\n");
    if (!header) {
      sb_add_chars(&result[n].error, "ERROR invalid response");
      continue;
    }
    *header = 0;
    header  = strtok(res, " ");
    if (header == NULL || (strcmp(header, "HTTP/1.1") && strcmp(header, "HTTP/1.0"))) {
      sb_add_chars(&result[n].error, "ERROR invalid HTTP Version");
      continue;
    }
    header     = strtok(NULL, " ");
    int status = header ? atoi(header) : 0;
    if (status < 200 || status >= 400) {
      sb_add_chars(&result[n].error, "ERROR failed request");
      continue;
    }

    memmove(res, body, result[n].result.len - (body - res) + 1);
    result[n].result.len -= body - res;
  }

  return 0;
}