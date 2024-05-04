#include <iostream>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <vector>
#include <sys/poll.h>
using namespace std;
// SUBSCRIBER = CLIENT TCP 

vector <struct pollfd> pfds;    // vector in care stocam file descriptorii pentru multiplexare
                                // alocat dinamic pentru eficienta
int nfds;



int main(int argc, char *argv[]) {

    setvbuf(stdout, NULL, _IONBF, BUFSIZ);  //buffering la afișare dezactivat

    //Verificare pornire corectă server: ./server <PORT_DORIT>
    if(argc != 2) {
        printf("Wrong server usage, use ./server <PORT>");
        exit(1);
    }
    

    int server_port = atoi(argv[1]);
    int tcp_sock, udp_sock;
    struct sockaddr_in tcp_addr, udp_addr;

    /* Clean buffers and structures*/
    memset(&tcp_addr, 0, sizeof(tcp_addr));
    memset(&udp_addr, 0, sizeof(udp_addr));

    // Creare socket TCP (cel pasiv, pe care face listen)
    tcp_sock = socket(AF_INET, SOCK_STREAM, 0);
    if(tcp_sock < 0){
        cerr << "[SERV] Error while creating TCP socket\n";
        exit(1);
    }

    // Creare socket UDP
    udp_sock = socket(AF_INET, SOCK_DGRAM, 0);
    if(udp_sock < 0){
        cerr << "[SERV] Error while creating TCP socket\n";
        exit(1);
    }

    /* Set port and IP for TCP */
    tcp_addr.sin_family = AF_INET;
    tcp_addr.sin_port = htons(server_port);
    tcp_addr.sin_addr.s_addr = INADDR_ANY;

    /* Set port and IP for UDP */
    udp_addr.sin_family = AF_INET;
    udp_addr.sin_port = htons(server_port);
    udp_addr.sin_addr.s_addr = INADDR_ANY;

    /* Bind to the set port and IP */
    if(bind(tcp_sock, (struct sockaddr*)&tcp_addr, sizeof(tcp_addr)) < 0) {
        cerr << "[SERV] Couldn't bind to the port\n";
        exit(1);
    }
    if(bind(udp_sock, (struct sockaddr*)&udp_addr, sizeof(udp_addr)) < 0) {
        cerr << "[SERV] Couldn't bind to the port\n";
        exit(1);
    }

    /* Listen for clients pe socket-ul TCP pasiv*/

    if(listen(tcp_sock, INT8_MAX) < 0) {
        cerr << "Error while listening\n";
        exit(1);
    }
    // De acum incolo, serverul poate da accept la clienti pe socketul tcp_sock

    // Actualizam lista de file descriptori pfds
    pfds.push_back({tcp_sock, POLLIN, 0});
    pfds.push_back({udp_sock, POLLIN, 0});
    pfds.push_back({0, POLLIN, 0}); // STDIN
    nfds = 3;

    int rc; // for errors

    while(1) {
        rc = poll(pfds.data(), pfds.size(), -1);
        if(rc < 0) {
            cerr << "Poll error\n";
            exit(1);
        }




    }


}