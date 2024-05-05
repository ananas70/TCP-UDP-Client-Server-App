#include <iostream>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <vector>
#include <sys/poll.h>
#include <netinet/tcp.h>
#include <types.h>
#include <unistd.h>

using namespace std;
// SUBSCRIBER = CLIENT TCP 

vector <struct pollfd> pfds;    // vector in care stocam file descriptorii pentru multiplexare
                                // alocat dinamic pentru eficienta

bool STDIN_message() {
    // returneaza 1 daca primeste exit, 0 altminteri

    char buff[BUF_LEN];
    memset(buff, 0, BUF_LEN);
    int rc = read(0, buff, BUF_LEN);
    if(rc < 0) {
        cerr << "Error STDIN\n";
        return false;
    }

    if(strncmp(buff, "exit", 4) == 0) {
        //exit server routine
        return true;
    }




}








int main(int argc, char *argv[]) {

    setvbuf(stdout, NULL, _IONBF, BUFSIZ);  //buffering la afisare dezactivat
    //nu mai folosesti cout dacă l-ai setat pe asta asa
    if(argc != 4) {
        cerr << "Wrong subscriber usage, use ./subscriber <ID_CLIENT> <IP_SERVER> <PORT_SERVER>\n";
        exit(1);
    }

    // colectare informatii server
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(atoi(argv[3]));
    inet_aton(argv[2], &serv_addr.sin_addr);


    // creare socket TCP
    int tcp_sock = socket(AF_INET, SOCK_STREAM, 0);
    if(tcp_sock < 0){
        cerr << "[CLIENT] Error while creating TCP socket\n";
        exit(1);
    }

    //dezactivam nagle
    int flag = 1;
    setsockopt(tcp_sock, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(int));

    // make reusable
    flag = 1;
    setsockopt(tcp_sock, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(int));

    // conectare la server
    if(connect(tcp_sock, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        cerr << "error connect\n";
        exit(1);
    }

    if(send(tcp_sock, argv[1], 1 + strlen(argv[1]), 0) < 0) {   //nuj de ce e +1
        cerr << "error send\n";
        exit(1);
    }

    // Actualizam lista de file descriptori pfds
    pfds.push_back({tcp_sock, POLLIN, 0});
    pfds.push_back({0, POLLIN, 0}); // STDIN
    
        int rc; // for errors
    bool stop_subscriber = false;
    while(!stop_subscriber) {
        rc = poll(pfds.data(), pfds.size(), -1);    //.data() - intoarce adresa de start a vectorului
        if(rc < 0) {
            cerr << "Poll error\n";
            exit(1);
        }

        // vedem ce conexiuni au noi date ce trebuie citite
        // 4 cazuri: TCP, UDP, STDIN (verifica cazul - comandă neacceptată), SUBSCRIBER
        for(auto &pfd : pfds) 
            if(pfd.revents & POLLIN) {

                // Evaluam file descriptor-ul pfd.fd

                if(pfd.fd == tcp_sock) {

                    TCP_connection(tcp_sock, tcp_addr);
                }

                else {   
                    //STDIN
                    bool stop = STDIN_message();
                    if(stop == true){
                        // oprim server-ul
                        stop_server = true;
                        break;
                    }

                }

            }
    }

    //close chestii

}