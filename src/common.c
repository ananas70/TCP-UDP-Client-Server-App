#include "common.h"
#include <sys/socket.h>
#include <sys/types.h>

/*
    TODO 1.1: Rescrieți funcția de mai jos astfel încât ea să facă primirea
    a exact len octeți din buffer.
*/
int recv_all(int sockfd, void *buffer, size_t len) {
  size_t bytes_received = 0;
  size_t bytes_remaining = len;
  char *buff = buffer;
  while (bytes_remaining) {
    size_t bytes_recv = recv(sockfd, buff, bytes_remaining, 0);
    DIE(bytes_recv == -1, "recv");

    bytes_received += bytes_recv;
    buff += bytes_received;
    bytes_remaining -= bytes_recv;
  }

  return bytes_received;

}

/*
    TODO 1.2: Rescrieți funcția de mai jos astfel încât ea să facă trimiterea
    a exact len octeți din buffer.
*/

int send_all(int sockfd, void *buffer, size_t len) {
  size_t bytes_sent = 0;
  size_t bytes_remaining = len;
  char *buff = buffer;
  while (bytes_remaining) {
    size_t bytes_send = send(sockfd, buff, bytes_remaining, 0);
    DIE(bytes_send == -1, "send");

    bytes_sent += bytes_send;
    buff += bytes_send;
    bytes_remaining -= bytes_send;
  }

  return bytes_sent;

}
