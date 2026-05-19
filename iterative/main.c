#include "arguments.h"
#include "server.h"
#include "utils.h"
#include "signals.h"
#include "log.h"

#include <stdio.h>
#include <stdlib.h>     // EXIT_*
#include <string.h>
#include <unistd.h>     // for close()
#include <arpa/inet.h>  // for inet_ntoa()
#include <errno.h>

int main(int argc, char **argv) {
  struct arguments args;

  // iniciamos el syslog
  log_init("ftp_server_simulator");

  if (parse_arguments(argc, argv, &args) != 0){
    log_write(LOG_ERR, "Error parsing arguments");
    log_close();
    return EXIT_FAILURE;
  }

  log_write(LOG_INFO, "Starting server on %s:%d", args.address, args.port);
  //printf("Starting server on %s:%d\n", args.address, args.port);

  int listen_fd = server_init(args.address, args.port);
  if (listen_fd < 0){
    log_write(LOG_ERR, "Failed to initialize server socket");
    log_close();
    return EXIT_FAILURE;
  }

  // Guardamos el socket en la variable global de signals por si acaso
  server_socket = listen_fd;
  setup_signals();

  while(1) {
    struct sockaddr_in client_addr;
    int new_socket = server_accept(listen_fd, &client_addr);
    if (new_socket < 0)
      continue;

    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip));
    log_write(LOG_INFO, "Connection from %s:%d accepted", client_ip, ntohs(client_addr.sin_port));
    //printf("Connection from %s:%d accepted\n", client_ip, ntohs(client_addr.sin_port));

    server_loop(new_socket);

    log_write(LOG_INFO, "Connection from %s:%d closed", client_ip, ntohs(client_addr.sin_port));
    //printf("Connection from %s:%d closed\n", client_ip, ntohs(client_addr.sin_port));
  }
  
  // NEVER GO HERE
  close_fd(listen_fd, "listening socket");

  // https://en.cppreference.com/w/c/program/EXIT_status
  return EXIT_SUCCESS;
}