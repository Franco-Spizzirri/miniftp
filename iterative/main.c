#include "arguments.h"
#include "server.h"
#include "utils.h"
#include "signals.h"
#include <stdio.h>
#include <stdlib.h>     // EXIT_*
#include <string.h>
#include <unistd.h>     // for close()
#include <arpa/inet.h>  // for inet_ntoa()
#include <errno.h>
#include <libdaemon/daemon.h>  // Fixed typo: libdeamon -> libdaemon
#include "log.h"      
#include "config.h"   

int main(int argc, char **argv) {
  struct arguments args;

  if (parse_arguments(argc, argv, &args) != 0)
    return EXIT_FAILURE;

  // Identity for the daemon and logs
  daemon_pid_file_ident = daemon_log_ident = "ftp_server";

  printf("Starting server on %s:%d\n", args.address, args.port);
  
  pid_t existing_pid;   // Check for running instance
  if ((existing_pid = daemon_pid_file_is_running()) >= 0) {
    fprintf(stderr, "Daemon is already running (PID: %d)\n", existing_pid);
    return EXIT_FAILURE;
  }

  // Initialize return value for Parent-Child communication
  if (daemon_retval_init() < 0) {
    fprintf(stderr, "Error initializing libdaemon\n");
    return EXIT_FAILURE;
  }

  // Initial Fork
  pid_t pid = daemon_fork();

  if (pid < 0) { // Error
    daemon_retval_done();
    return EXIT_FAILURE;
  } 
    
  if (pid > 0) { // Parent Process: Wait for child to confirm success
    int res = daemon_retval_wait(10); 
    return res < 0 ? EXIT_FAILURE : res;
  }

  /* --- DAEMON PROCESS START (Child) --- */

  // Create PID file and close standard streams
  daemon_pid_file_create();
  log_init("ftp_server"); // Initialize syslog

  // Initialize server logic
  int listen_fd = server_init(args.address, args.port);
  if (listen_fd < 0) {
    log_write(LOG_ERR, "Error starting socket on %s:%d", args.address, args.port);
    daemon_retval_send(1); // Notify error to parent
    goto finish;
  }

  // Success signal to parent: The daemon is now independent
  daemon_retval_send(0);
  setup_signals();

  log_write(LOG_INFO, "FTP Server daemonized at %s:%d", args.address, args.port);
  
  while(1) {
    struct sockaddr_in client_addr;
    int new_socket = server_accept(listen_fd, &client_addr);

    if (new_socket < 0)
      continue;

    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip));
    log_write(LOG_INFO, "Connection accepted from %s:%d", client_ip, ntohs(client_addr.sin_port));

    server_loop(new_socket);

    log_write(LOG_INFO, "Connection closed with %s", client_ip);
  }

finish:
  log_write(LOG_INFO, "Closing daemon");
  daemon_pid_file_remove();
  log_close();
  return EXIT_SUCCESS;
}
