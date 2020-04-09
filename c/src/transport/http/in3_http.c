/*******************************************************************************
 * This file is part of the Incubed project.
 * Sources: https://github.com/slockit/in3-c
 * 
 * Copyright (C) 2018-2020 slock.it GmbH, Blockchains LLC
 * 
 * 
 * COMMERCIAL LICENSE USAGE
 * 
 * Licensees holding a valid commercial license may use this file in accordance 
 * with the commercial license agreement provided with the Software or, alternatively, 
 * in accordance with the terms contained in a written agreement between you and 
 * slock.it GmbH/Blockchains LLC. For licensing terms and conditions or further 
 * information please contact slock.it at in3@slock.it.
 * 	
 * Alternatively, this file may be used under the AGPL license as follows:
 *    
 * AGPL LICENSE USAGE
 * 
 * This program is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Affero General Public License as published by the Free Software 
 * Foundation, either version 3 of the License, or (at your option) any later version.
 *  
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY 
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A 
 * PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
 * [Permissions of this strong copyleft license are conditioned on making available 
 * complete source code of licensed works and modifications, which include larger 
 * works using a licensed work, under the same license. Copyright and license notices 
 * must be preserved. Contributors provide an express grant of patent rights.]
 * You should have received a copy of the GNU Affero General Public License along 
 * with this program. If not, see <https://www.gnu.org/licenses/>.
 *******************************************************************************/

#include <stdio.h>  /* printf, sprintf */
#include <stdlib.h> /* exit, atoi, malloc, free */
#include <string.h> /* memcpy, memset */
#include <time.h>
#include <unistd.h> /* read, write, close */
#ifdef _WIN32
// clang-format off
#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
// clang-format on
#else
#include <netdb.h>      /* struct hostent, gethostbyname */
#include <netinet/in.h> /* struct sockaddr_in, struct sockaddr */
#include <sys/socket.h> /* socket, connect */
#endif
#include "../../core/client/client.h"
#include "../../core/util/mem.h"
#include "../../core/util/utils.h"
#include "in3_http.h"

in3_ret_t send_http(in3_request_t* req) {
  if (!req->times) req->times = _malloc(sizeof(uint32_t) * req->urls_len);
  for (int n = 0; n < req->urls_len; n++) {

    struct hostent*    server;
    struct sockaddr_in serv_addr;
    int                received, bytes, sent, total;
    char *             message = alloca(strlen(req->payload) + 200), response[4096], *url = req->urls[n], host[256];
    uint64_t           start = current_ms();

    (void) received;
    (void) bytes;
    (void) sent;

    // parse url
    if (strncmp(url, "http://", 7)) {
      sb_add_chars(&req->results[n].error, "invalid url must sart with http");
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
    int   portno = 80;
    char* port   = strchr(host, ':');
    if (port) {
      *port  = 0;
      portno = atoi(port + 1);
    }

    // create message
    sprintf(message, "POST %s HTTP/1.0\r\nHost: %s\r\nContent-Type: application/json\r\nContent-Length: %d\r\n\r\n%s", path, host, (int) strlen(req->payload), req->payload);
    total = strlen(message);

/* create the socket */
#ifdef _WIN32
    (void) (total); // unused var
    WSADATA wsa;
    SOCKET  s;

    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
      sb_add_chars(&req->results[n].error, "no socket availabe");
      continue;
    }

    //Create a socket
    if ((s = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
      sb_add_chars(&req->results[n].error, "Could not create socket");
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
      sb_add_chars(&req->results[n].error, "Connection failed");
      continue;
    }

    if (send(s, message, strlen(message), 0) < 0) {
      sb_add_chars(&req->results[n].error, "Send failed");
      continue;
    }

    //Receive a reply from the server
    if ((received = recv(s, response, 2000, 0)) == SOCKET_ERROR) {
      sb_add_chars(&req->results[n].error, "receive failed");
      continue;
    }

    //Add a NULL terminating character to make it a proper string before printing
    response[received] = '\0';
    sb_add_chars(&req->results[n].result, response);

    closesocket(s);
    WSACleanup();
#else
    int sockfd;
    server = gethostbyname(host);
    if (server == NULL) {
      sb_add_chars(&req->results[n].error, "no such host");
      continue;
    }
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
      sb_add_chars(&req->results[n].error, "ERROR opening socket");
      continue;
    }
    /* fill in the structreq->ure */
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port   = htons(portno);
    memcpy(&serv_addr.sin_addr.s_addr, server->h_addr_list[0], server->h_length);
    /* connect the socket */
    if (connect(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0) {
      sb_add_chars(&req->results[n].error, "ERROR connecting");
      continue;
    }
    /* send the request */

    sent = 0;
    do {
      bytes = write(sockfd, message + sent, total - sent);
      if (bytes < 0) {
        sb_add_chars(&req->results[n].error, "ERROR writing message to socket");
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
        sb_add_chars(&req->results[n].error, "ERROR reading response from socket");
        continue;
      }
      if (bytes == 0)
        break;
      sb_add_chars(&req->results[n].result, response);
      received += bytes;
    } while (1);

    if (received == total) {
      sb_add_chars(&req->results[n].error, "ERROR storing complete response from socket");
      continue;
    }

    /* close the socket */
    close(sockfd);

#endif

    req->times[n] = (uint32_t)(current_ms() - start);

    // now evaluate the response

    char *res = req->results[n].result.data, *header = strstr(res, "\r\n\r\n"), *body = header + 4;
    if (!header) {
      sb_add_chars(&req->results[n].error, "ERROR invalid response");
      continue;
    }
    *header = 0;
    header  = strstr(res, "\r\n");
    if (!header) {
      sb_add_chars(&req->results[n].error, "ERROR invalid response");
      continue;
    }
    *header = 0;
    header  = strtok(res, " ");
    if (header == NULL || (strcmp(header, "HTTP/1.1") && strcmp(header, "HTTP/1.0"))) {
      sb_add_chars(&req->results[n].error, "ERROR invalid HTTP Version");
      continue;
    }
    header     = strtok(NULL, " ");
    int status = header ? atoi(header) : 0;
    if (status < 200 || status >= 400) {
      sb_add_chars(&req->results[n].error, "ERROR failed request");
      continue;
    }

    memmove(res, body, req->results[n].result.len - (body - res) + 1);
    req->results[n].result.len -= body - res;
  }

  return 0;
}