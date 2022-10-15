/*********************************************************
 * Program: tcpping - Utility for TCP based ping.        *
 * Author:  Joseph Colton <josephcolton@gmail.com>       *
 * License: GNU General Public License 3.0               *
 *          https://www.gnu.org/licenses/gpl-3.0.en.html *
 *********************************************************/

#include <time.h>      // Clock
#include <stdio.h>     // printf
#include <stdlib.h>    // exit
#include <unistd.h>    // sleep
#include <arpa/inet.h> // inet_addr()
#include <netdb.h>     // hostent, gethostbyname()
#include <string.h>    // strncpy
#include <errno.h>     // errno
#include <fcntl.h>     // Non-blocking
#include <signal.h>    // Handle SIGINT, SIGTERM

/*************************
 * Globals and Constants *
 *************************/
const char version[] = "1.0.3";
const int FALSE = 0;
const int TRUE = 1;
const int LEN = 256;   // Maximum hostname size
int timeout = 3;       // Seconds before timeout
int terminate = FALSE; // SIGTERM, SIGINT triggered

/*********************************************
 * tcp_ping - Single tcp ping to ipaddr:port *
 *                                           *
 * Returns the RTT in milliseconds.          *
 * Returns -1 on timeout.                    *
 * Returns -2 on failure connecting.         * 
 *********************************************/
double tcp_ping(char *ipaddr, int port) {
  int sock;
  long arg;
  double rtt, diff_sec, diff_nsec;
  struct sockaddr_in address;
  struct timespec timestamp1, timestamp2;
  fd_set fdset;
  struct timeval tv; // Time value for timeout checks
  int status;
  int optval;
  socklen_t optlen;
  
  // socket create and verification
  sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock == -1) {
    printf("Socket creation failed!\n");
    exit(0);
  }

  // Set socket as non-blocking
  arg = fcntl(sock, F_GETFL, NULL); // Get current args
  arg |= O_NONBLOCK;                // Add the non-blocking option
  fcntl(sock, F_SETFL, arg);
  
  // Set timeout
  tv.tv_sec = timeout;
  tv.tv_usec = 0;

  // Set packet information
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = inet_addr(ipaddr);
  address.sin_port = htons(port);

  // Read clock before sending/connecting
  clock_gettime(CLOCK_MONOTONIC_RAW, &timestamp1);

  // Connect the client socket to server socket
  status = connect(sock, (struct sockaddr *)&address, sizeof(address));

  // Should be a negative status unless the tcp handshake is really fast. :)
  if (status < 0) {
     if (errno == EINPROGRESS) {
        do {
           FD_ZERO(&fdset);
           FD_SET(sock, &fdset);
           status = select(sock+1, NULL, &fdset, NULL, &tv);
           if (status < 0 && errno != EINTR) {
	     // Error connecting
             return -2;
           } else if (status > 0) {
	     // Connected
	     optlen = sizeof(int);
	     getsockopt(sock, SOL_SOCKET, SO_ERROR, (void*)(&optval), &optlen);
	     break;
           } else {
	     // Timeout
	     return -1;
           }
        } while (1);
     } else {
       // Failure to connect
       return -2;
     }
  }

  // Read clock after sending/connecting (after SYN and ACK)
  clock_gettime(CLOCK_MONOTONIC_RAW, &timestamp2);

  // Close the connection
  close(sock);

  // Calculate elapsed time
  diff_sec = timestamp2.tv_sec - timestamp1.tv_sec;
  diff_nsec = timestamp2.tv_nsec - timestamp1.tv_nsec;
  rtt = diff_sec * 1000;
  rtt += diff_nsec / 1000000;

  // Return elapsed time
  return rtt;
}

/*****************************************************
 * signal_handler - Tells the program to exit        *
 *                                                   *
 * Sets the terminate variable's value to true.      *
 * This causes the infinite loop to terminate early. *
 *****************************************************/
void signal_handler(int signum) {
  terminate = TRUE;
}

/*************************************
 * usage - Print the usage statement *
 *                                   *
 * Displays the usage statement.     *
 *************************************/
void usage(char *binary) {
  printf("Usage:\n\n");
  printf("\t%s [OPTIONS] HOSTNAME\n\n", binary);
  printf("OPTIONS:\n");
  printf("\t-c, --count COUNT    Number of tcp pings (default: unlimited)\n");
  printf("\t-p, --port PORT      TCP port number (default: 443)\n");
  printf("\t-i, --interval SEC   Number of seconds between pings (default: 1)\n");
  printf("\t-s, --skip COUNT     Number of pings to skip in statistics (default: 0)\n");
  printf("\t-t, --timeout SEC    Number of seconds to wait for timeout (default: 3)\n");
  printf("\t-d, --display all    Display all pings and statistics (default)\n");
  printf("\t              stat   Display only ending statistics\n");
  printf("\t              clean  Display clean minimal statistics for parsing\n");
  printf("\t-h, --help           Display this help message\n");
  printf("\t-v, --version        Display version information\n");
  printf("\n");
}

/**************************************************
 * main - Main program function                   *
 *                                                *
 * Gets information from cli, looks up addresses, *
 * runs the tcp ping function, then displays the  *
 * statistical results.                           *
 **************************************************/
int main(int argc, char *argv[]) {
  struct hostent *host;  // Host entity
  struct in_addr h_addr; // Address Struct
  char hostname[LEN];    // Hostname
  char ipaddr[LEN];      // IP Address
  int port = 443;        // TCP Port Number
  int count = 0;         // Number of pings
  // Statistics
  double stat_sum = 0;
  double stat_min = 0, stat_max = 0, stat_ave = 0;
  int stat_count = 0;
  int ping_count = 0; int ping_success = 0; int ping_fail = 0;
  double ping_loss = 0.0;
  double total_time, diff_sec, diff_nsec;
  struct timespec mainstamp1, mainstamp2; // Keep track of complete elapsed run time
  int display = 0;  // 0 = All pings and stats, 1 = stats only, 2 = clean
  int interval = 1; // Number of seconds between pings
  int skip = 0;     // Number of pings to skip and ignore from stats

  // Signal interception
  struct sigaction action;
  action.sa_handler = signal_handler;
  sigaction(SIGTERM, &action, NULL); // SIGTERM default kill -15
  sigaction(SIGINT, &action, NULL);  // SIGINT Ctrl-C
  
  // Parse arguments
  int status = 0;
  for (int i=1; i < argc; i++) {
    // Process Options
    if (argv[i][0] == '-') {
      // Port number
      if ((strncmp(argv[i], "-p", LEN) == 0) || (strncmp(argv[i], "--port", LEN) == 0)) {
	i++;
	if (i < argc) {
	  port = atoi(argv[i]);
	} else {
	  status = -1;
	  printf("Parse Error: Missing port number.\n");
	  break;
	}
      }
      // Ping count
      if ((strncmp(argv[i], "-c", LEN) == 0) || (strncmp(argv[i], "--count", LEN) == 0)) {
	i++;
	if (i < argc) {
	  count = atoi(argv[i]);
	} else {
	  status = -1;
	  printf("Parse Error: Missing ping count number.\n");
	  break;
	}
      }
      // Skip count
      if ((strncmp(argv[i], "-s", LEN) == 0) || (strncmp(argv[i], "--skip", LEN) == 0)) {
	i++;
	if (i < argc) {
	  skip = atoi(argv[i]);
	} else {
	  status = -1;
	  printf("Parse Error: Missing skip/ignore count number.\n");
	  break;
	}
      }
      // Interval seconds
      if ((strncmp(argv[i], "-i", LEN) == 0) || (strncmp(argv[i], "--interval", LEN) == 0)) {
	i++;
	if (i < argc) {
	  interval = atoi(argv[i]);
	} else {
	  status = -1;
	  printf("Parse Error: Missing interval seconds.\n");
	  break;
	}
      }
      // Timeout seconds
      if ((strncmp(argv[i], "-t", LEN) == 0) || (strncmp(argv[i], "--timeout", LEN) == 0)) {
	i++;
	if (i < argc) {
	  timeout = atoi(argv[i]);
	} else {
	  status = -1;
	  printf("Parse Error: Missing timeout seconds.\n");
	  break;
	}
      }
      // Display settings
      if ((strncmp(argv[i], "-d", LEN) == 0) || (strncmp(argv[i], "--display", LEN) == 0)) {
	i++;
	if (i < argc) {
	  // Display option
	  if (strncmp(argv[i], "all", LEN) == 0) { display = 0; continue; }
	  if (strncmp(argv[i], "stat", LEN) == 0) { display = 1; continue; }
	  if (strncmp(argv[i], "clean", LEN) == 0) { display = 2; continue; }
	  status = -1;
	  printf("Parse Error: Missing display setting.\n");
	  break;	  
	} else {
	  status = -1;
	  printf("Parse Error: Missing count number.\n");
	  break;
	}
      }
      // Help options
      if ((strncmp(argv[i], "-h", LEN) == 0) || (strncmp(argv[i], "--help", LEN) == 0)) {
	break;
      }
      // Version Infomation
      if ((strncmp(argv[i], "-v", LEN) == 0) || (strncmp(argv[i], "--version", LEN) == 0)) {
	printf("tcpping %s\n", version);
	return 0;
      }
    }
    // Finished Options
    else {
      if (status > 0) {
	status = -1;
	printf("Parse Error: Cannot determine HOSTNAME.\n");
	break;
      } else {
	strncpy(hostname, argv[i], LEN);
	status = 1;
      }
    }
  }
  
  // Verify arguments
  if (status < 1) {
    usage(argv[0]);
    exit(0);
  }

  // Get the hostname from the cli
  if ((host = gethostbyname(hostname)) == NULL) {
    printf("Lookup for '%s' failed.\n", hostname);
    exit(1);
  }
  h_addr.s_addr = *((unsigned long *) host->h_addr_list[0]);
  strncpy(ipaddr, inet_ntoa(h_addr), LEN);
  
  // Start ping process
  if (display == 0 || display == 1)
    printf("TCP PING %s (%s) tcp port %d\n", hostname, ipaddr, port);

  // Read clock before starting tcp pinging
  clock_gettime(CLOCK_MONOTONIC_RAW, &mainstamp1);

  int countdown = count;
  double rtt;
  while(!terminate && (count == 0 || countdown)) {
    // single ping
    rtt = tcp_ping(ipaddr, port);

    // Display RTT latency
    if (display == 0) {
      if (rtt > 0) {
	if (skip) printf("%s: %.3f ms (skip: %d)\n", ipaddr, rtt, skip);
	else printf("%s: %.3f ms\n", ipaddr, rtt);
      } else {
	if (rtt == -1) {
	  if (skip) printf("%s: timeout(%d) (skip: %d)\n", ipaddr, timeout, skip);
	  else printf("%s: timeout(%d)\n", ipaddr, timeout);
	}
	if (rtt == -2) {
	  if (skip) printf("%s: connection error (skip: %d)\n", ipaddr, skip);
	  else printf("%s: connection error\n", ipaddr);
	}
      }
    }

    // Update statistics
    if (skip) {
      skip--;
    } else {
      ping_count++;
      if (rtt > 0) {
	ping_success++;
	stat_sum += rtt;
	stat_count++;
	if (stat_count == 1) {
	  stat_min = stat_max = rtt;
	}
	if (rtt < stat_min) stat_min = rtt;
	if (rtt > stat_max) stat_max = rtt;
	stat_ave = stat_sum / stat_count;
      } else {
	ping_fail++;
      }
      ping_loss = (double)ping_fail / (double)ping_count * 100;
    }
    
    // Update countdown
    countdown--;
    if (countdown) sleep(interval);
  }

  // Read clock after stopping tcp pinging
  clock_gettime(CLOCK_MONOTONIC_RAW, &mainstamp2);
  diff_sec = mainstamp2.tv_sec - mainstamp1.tv_sec;
  diff_nsec = mainstamp2.tv_nsec - mainstamp1.tv_nsec;
  total_time = diff_sec * 1000;
  total_time+= diff_nsec / 1000000;

  // Display statistics
  if (display == 0 || display == 1) {
    printf("--- %s tcp ping statistics ---\n", hostname);
    printf("%d pings, %d success, %d failed, %0.1f%% loss, total time: %.3f ms\n",
	   ping_count, ping_success, ping_fail, ping_loss, total_time);
    printf("rtt min/ave/max/range = %0.3f/%0.3f/%0.3f/%0.3f ms\n", stat_min, stat_ave, stat_max, stat_max - stat_min);
  }
  if (display == 2) {
    printf("Pings: %d\n", ping_count);
    printf("Min: %0.3f\n", stat_min);
    printf("Max: %0.3f\n", stat_max);
    printf("Ave: %0.3f\n", stat_ave);
    printf("Loss: %0.1f\n", ping_loss);
  }
  return 0;
}
