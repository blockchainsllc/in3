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

#include "http_server.h"
#include "../../core/client/context.h"
#include "../../core/util/colors.h"
#include "../../core/util/mem.h"
#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define MAX_CON 100

static int listenfd, clients[MAX_CON];

//client connection
static void respond(int s, in3_t* in3) {
  char* buf  = malloc(65535);
  int   rcvd = recv(clients[s], buf, 65535, 0);

  if (rcvd < 0) // receive error
    fprintf(stderr, ("recv() error\n"));
  else if (rcvd == 0) // receive socket closedMAX_CON
    fprintf(stderr, "Client disconnected upexpectedly.\n");
  else // message received
  {
    buf[rcvd] = '\0';

    char* method = strtok(buf, " \t\r\n");
    char* uri    = method ? strtok(NULL, " \t") : NULL;
    char* prot   = uri ? strtok(NULL, " \t\r\n") : NULL;
    char* rest   = prot ? strstr(prot + strlen(prot) + 1, "\n\r\n") : NULL;

    dup2(clients[s], STDOUT_FILENO);
    close(clients[s]);
    if (rest) {
      rest += 3;
      if (strlen(rest) > 2 && (rest[0] == '{' || rest[0] == '[')) {
        // execute in3
        in3_ctx_t* ctx = ctx_new(in3, rest);
        if (ctx == NULL)
          printf("HTTP/1.1 500 Not Handled\r\n\r\nInvalid request.\r\n");
        else if (ctx->error)
          printf("HTTP/1.1 500 Not Handled\r\n\r\n%s\r\n", ctx->error);
        else {
          // execute it
          fprintf(stderr, "RPC %s\n", d_get_string(ctx->request_context->result, "method")); //conceal typing and save position
          if (in3_send_ctx(ctx) == IN3_OK) {
            // the request was succesfull, so we delete interim errors (which can happen in case in3 had to retry)
            if (ctx->error) _free(ctx->error);
            ctx->error            = NULL;
            str_range_t range     = d_to_json(ctx->responses[0]);
            range.data[range.len] = 0;

            // remove in3
            char* end = strstr(range.data, ",\"in3\":");
            if (end) {
              *end   = '}';
              end[1] = 0;
            }
            range.len = strlen(range.data);
            printf("HTTP/1.1 200\r\nContent-Type: application/json; charset=utf-8\r\nContent-Length: %i\r\n\r\n%s\r\n", (int) range.len, range.data);
          } else if (ctx->error)
            printf("HTTP/1.1 500 Not Handled\r\n\r\n%s\r\n", ctx->error);
          else
            printf("HTTP/1.1 500 Not Handled\r\n\r\nCould not execute\r\n");
        }
        if (ctx)
          ctx_free(ctx);

      } else
        rest = NULL;
    }

    if (!rest)
      printf("HTTP/1.1 500 Not Handled\r\n\r\nThe server has no handler to the request.\r\n");

    // tidy up
    fflush(stdout);
    shutdown(STDOUT_FILENO, SHUT_WR);
    close(STDOUT_FILENO);
  }

  //Closing SOCKET
  shutdown(clients[s], SHUT_RDWR); //All further send and recieve operations are DISABLED...
  close(clients[s]);
  clients[s] = -1;
}

void http_run_server(const char* port, in3_t* in3) {
  struct sockaddr_in clientaddr;
  socklen_t          addrlen;
  int                s = 0;

  printf(
      "Server started %shttp://127.0.0.1:%s%s\n",
      COLORT_LIGHTGREEN, port, COLORT_RESET);

  for (int i = 0; i < MAX_CON; i++) clients[i] = -1;

  // start the serevr
  struct addrinfo hints, *res, *p;

  // getaddrinfo for host
  memset(&hints, 0, sizeof(hints));
  hints.ai_family   = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags    = AI_PASSIVE;
  if (getaddrinfo(NULL, port, &hints, &res) != 0) {
    perror("getaddrinfo() error");
    exit(1);
  }
  // socket and bind
  for (p = res; p != NULL; p = p->ai_next) {
    int option = 1;
    listenfd   = socket(p->ai_family, p->ai_socktype, 0);
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));
    if (listenfd == -1) continue;
    if (bind(listenfd, p->ai_addr, p->ai_addrlen) == 0) break;
  }
  if (p == NULL) {
    perror("socket() or bind()");
    exit(1);
  }
  freeaddrinfo(res);

  // listen for incoming connections
  if (listen(listenfd, 1000000) != 0) {
    perror("listen() error");
    exit(1);
  }
  // Ignore SIGCHLD to avoid zombie threads
  signal(SIGCHLD, SIG_IGN);

  // ACCEPT connections
  while (1) {
    addrlen    = sizeof(clientaddr);
    clients[s] = accept(listenfd, (struct sockaddr*) &clientaddr, &addrlen);

    if (clients[s] < 0) {
      perror("accept() error");
    } else {
      if (fork() == 0) {
        respond(s, in3);
        exit(0);
      }
    }

    while (clients[s] != -1) s = (s + 1) % MAX_CON;
  }
}
