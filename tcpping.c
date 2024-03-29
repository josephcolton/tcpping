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
#include <ctype.h>     // isdigit
#include <arpa/inet.h> // inet_addr()
#include <netdb.h>     // hostent, gethostbyname()
#include <string.h>    // strncpy
#include <errno.h>     // errno
#include <fcntl.h>     // Non-blocking
#include <signal.h>    // Handle SIGINT, SIGTERM

/*************************
 * Globals and Constants *
 *************************/
const char version[] = "1.0.8";
typedef enum {FALSE = 0, TRUE = 1} boolean;
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

/*****************************************************
 * is_number - Checks to see if a string is a number *
 *                                                   *
 * Reads in a string until the end and varifies that *
 * all of the characters are digits.                 *
 *****************************************************/
int is_number(char *str, int maxint) {
  int rvalue = FALSE;
  int i;
  for (i=0; i < maxint; i++) {
    if (str[i] == 0) break; // End of line
    if (isdigit(str[i])) rvalue = TRUE; // Found a digit
    if (! isdigit(str[i])) { // Found a non-digit before the end
      rvalue = FALSE;
      break;
    }
  }
  return rvalue;
}

/*************************************
 * usage - Print the usage statement *
 *                                   *
 * Displays the usage statement.     *
 *************************************/
void usage(char *binary) {
  printf("tcpping %s\n", version);
  printf("Usage:\n\n");
  printf("\t%s [OPTIONS] HOSTNAME\n\n", binary);
  printf("OPTIONS:\n");
  printf("\t-a, --audible        Audible ping sound\n");
  printf("\t-c, --count COUNT    Stop after COUNT tcp pings (default: unlimited)\n");
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
  struct hostent *host;    // Host entity
  struct in_addr h_addr;   // Address Struct
  char hostname[LEN];      // Hostname
  char ipaddr[LEN];        // IP Address
  int port = 443;          // TCP Port Number
  int count = 0;           // Number of pings
  boolean audible = FALSE; // Audible ping
  // Statistics
  int seq = 0;           // Sequence number
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
  int i;
  for (i=1; i < argc; i++) {
    // Process Options
    if (argv[i][0] == '-') {
      // Port number
      if ((strncmp(argv[i], "-p", LEN) == 0) || (strncmp(argv[i], "--port", LEN) == 0)) {
	i++;
	if (i < argc && is_number(argv[i], LEN)) {
	  port = atoi(argv[i]);
	} else {
	  status = -1;
	  printf("Parse Error: Missing port number.\n");
	  break;
	}
	continue;
      }
      // Ping count
      if ((strncmp(argv[i], "-c", LEN) == 0) || (strncmp(argv[i], "--count", LEN) == 0)) {
	i++;
	if (i < argc && is_number(argv[i], LEN)) {
	  count = atoi(argv[i]);
	} else {
	  status = -1;
	  printf("Parse Error: Missing ping count number.\n");
	  break;
	}
	continue;
      }
      // Skip count
      if ((strncmp(argv[i], "-s", LEN) == 0) || (strncmp(argv[i], "--skip", LEN) == 0)) {
	i++;
	if (i < argc && is_number(argv[i], LEN)) {
	  skip = atoi(argv[i]);
	} else {
	  status = -1;
	  printf("Parse Error: Missing skip/ignore count number.\n");
	  break;
	}
	continue;
      }
      // Interval seconds
      if ((strncmp(argv[i], "-i", LEN) == 0) || (strncmp(argv[i], "--interval", LEN) == 0)) {
	i++;
	if (i < argc && is_number(argv[i], LEN)) {
	  interval = atoi(argv[i]);
	} else {
	  status = -1;
	  printf("Parse Error: Missing interval seconds.\n");
	  break;
	}
	continue;
      }
      // Timeout seconds
      if ((strncmp(argv[i], "-t", LEN) == 0) || (strncmp(argv[i], "--timeout", LEN) == 0)) {
	i++;
	if (i < argc && is_number(argv[i], LEN)) {
	  timeout = atoi(argv[i]);
	} else {
	  status = -1;
	  printf("Parse Error: Missing timeout seconds.\n");
	  break;
	}
	continue;
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
	continue;
      }
      // Audible ping
      if ((strncmp(argv[i], "-a", LEN) == 0) || (strncmp(argv[i], "--audible", LEN) == 0)) {
	audible = TRUE;
	continue;
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
      // Option does not appear valid
      printf("Unknown option: %s\n", argv[i]);
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
  } // End of for loop
  
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
  double diff, prev_rtt = -1, jitter = 0, jitter_total = 0; // Jitter statistics
  int jitter_count = 0;
  while(!terminate && (count == 0 || countdown)) {
    // single ping
    rtt = tcp_ping(ipaddr, port);
    seq++;

    // Display audible bell (if requested)
    if (audible) printf("\a");

    // Display RTT latency
    if (display == 0) {
      if (rtt > 0) {
	if (skip) printf("%s: seq=%d time=%0.3f ms (skip: %d)\n", ipaddr, seq, rtt, skip);
	else printf("%s: seq=%d time=%0.3f ms\n", ipaddr, seq, rtt);
      } else {
	if (rtt == -1) {
	  if (skip) printf("%s: seq=%d timeout(%d) (skip: %d)\n", ipaddr, seq, timeout, skip);
	  else printf("%s: seq=%d timeout(%d)\n", ipaddr, seq, timeout);
	}
	if (rtt == -2) {
	  if (skip) printf("%s: seq=%d connection error (skip: %d)\n", ipaddr, seq, skip);
	  else printf("%s: seq=%d connection error\n", ipaddr, seq);
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
	stat_sum += rtt;  // Update sum stats
	stat_count++;     // Update total recorded stats
	if (prev_rtt == -1) { // Jitter staticistis
	  prev_rtt = rtt;
	} else {
	  diff = prev_rtt - rtt;
	  if (diff < 0) diff = 0 - diff;
	  jitter_total += diff;
	  prev_rtt = rtt;
	  jitter_count++;
	  jitter = jitter_total / jitter_count;
	}
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
    printf("%d pings, %d success, %d failed, %0.1f%% loss, total run time: %0.3f ms\n",
	   ping_count, ping_success, ping_fail, ping_loss, total_time);
    printf("rtt min/ave/max/range/jitter = %0.3f/%0.3f/%0.3f/%0.3f/%0.3f ms\n", stat_min, stat_ave, stat_max, stat_max - stat_min, jitter);
  }
  if (display == 2) {
    printf("Pings: %d\n", ping_count);
    printf("Min: %0.3f\n", stat_min);
    printf("Max: %0.3f\n", stat_max);
    printf("Ave: %0.3f\n", stat_ave);
    printf("Jitter: %0.3f\n", jitter);
    printf("Loss: %0.1f\n", ping_loss);
  }
  return 0;
}
