#include "common.h"
#include <sys/socket.h>
#include <sys/types.h>
#include <stdio.h>
#include <netinet/in.h>

/*
    TODO 1.1: Rescrieți funcția de mai jos astfel încât ea să facă primirea
    a exact len octeți din buffer.
*/
int recv_all(int sockfd, void *buffer, size_t len)
{
  size_t bytes_received = 0;
  size_t bytes_remaining = len;
  char *buff = (char *)buffer;
  while (bytes_remaining)
  {
    size_t bytes_recv = recv(sockfd, buff, bytes_remaining, 0);
    if(bytes_recv <=0 )
      {fprintf(stderr, "error recv\n");
      return bytes_received; }
    bytes_received += bytes_recv;
    buff += bytes_received;
    bytes_remaining -= bytes_recv;
  }

  /*
    TODO: Returnam exact cati octeti am citit
  */
  return bytes_received;
}

/*
    TODO 1.2: Rescrieți funcția de mai jos astfel încât ea să facă trimiterea
    a exact len octeți din buffer.
*/

int send_all(int sockfd, void *buffer, size_t len) {
int bufsize = 0;
setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, &bufsize, sizeof(int));
  size_t bytes_sent = 0;
  size_t bytes_remaining = len;
  char *buff = (char *)buffer;
  while (bytes_remaining) {
    int bytes_send = send(sockfd, buff, bytes_remaining, 0);
    if(bytes_send == -1) {
      fprintf(stderr, "error send_all");
      return 0; //nuj
    }

    bytes_sent += bytes_send;
    buff += bytes_send;
    bytes_remaining -= bytes_send;
  }

  return bytes_sent;

}
