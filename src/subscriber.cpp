#include <iostream>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>

#include <vector>
#include <string>
#include <sys/socket.h>
#include <sys/poll.h>

#include "types.h"

using namespace std;


vector <struct pollfd> pfds;    // Vector in care stocam file descriptorii pentru multiplexare


void build_request(struct client_request* request, char* buff) {
    char *p = strtok(buff, " ");
    if(p == NULL) {
        // Comanda nula
        cerr << "No command received\n";
        return;
    }

    if(strcmp(p, "subscribe") == 0)
        request->type = 's';
    else if(strcmp(p, "unsubscribe") == 0)
        request->type = 'u';
    else {
        cerr << "Wrong command oops\n";
        return;
    }
    
    // Preluare topic
    p = strtok(NULL, " ");
    if(p == NULL) {
        cerr << "No topic received\n";
        return;
    }

    strcpy(request->topic,p);
    request->topic[strlen(request->topic) - 1]  = '\0';
}


void submit_request(char* buff, int tcp_sock) {
    /*
    Comenzi acceptate:
        subscribe <TOPIC> 
        unsubscribe <TOPIC> 
    */

    struct client_request request;
    build_request(&request, buff);

    // Trimitere către server
    if (send(tcp_sock, (char*) &request, sizeof(request),0) <= 0) {
        cerr << "Error send\n";
        return;
    }

    // Afișări
    if(request.type == 's')
        cout <<"Subscribed to topic "<< request.topic << endl;
    else
        cout <<"Unsubscribed from topic "<< request.topic << endl;

}


bool STDIN_message(int tcp_sock) {
    /* 
    Returnează:
        - 1, daca primește comanda "exit"
        - 0, altfel
    */

    char buff[BUF_LEN];
    memset(buff, 0, BUF_LEN);
    int rc = read(0, buff, BUF_LEN);

    if(rc < 0) {
        cerr << "Error STDIN\n";
        return false;
    }

    buff[rc] = '\0';
    if(strncmp(buff, "exit", 4) == 0)
        return true;

    // S-a primit o comandă de subscribe sau unsubscribe
    submit_request(buff, tcp_sock);
    return false;

}


void parse_server_msg(int tcp_sock) {
    int LEN = sizeof(client_notification) + 1;
    char buff[LEN];
    memset(buff, 0, LEN);
    int rc = recv(tcp_sock, buff, sizeof(client_notification), 0);

    if(rc < 0) {
        cerr << "Error recv\n";
        return;
    }

    if(rc == 0) {
        cerr << "Server ended connection :( \n";
        close(tcp_sock);
        exit(1);
    }
 
    // Afișare mesaj primit
    client_notification* notif = (client_notification*) buff;

    // <TOPIC> - <TIP_DATE> - <VALOARE_MESAJ>

    string topic = string(notif->topic, 50);
    cout << topic.c_str()<< " - " << notif->data_type << " - " << notif->content << endl; 

}

int main(int argc, char *argv[]) {
    setvbuf(stdout, NULL, _IONBF, BUFSIZ);  //  Buffering la afișare dezactivat

    if(argc != 4) {
        cerr << "Wrong subscriber usage, use ./subscriber <ID_CLIENT> <IP_SERVER> <PORT_SERVER>\n";
        exit(1);
    }

    // Colectare informații server
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(atoi(argv[3]));
    inet_aton(argv[2], &serv_addr.sin_addr);


    // Creare socket TCP
    int tcp_sock = socket(AF_INET, SOCK_STREAM, 0);
    if(tcp_sock < 0){
        cerr << "[CLIENT] Error while creating TCP socket\n";
        exit(1);
    }


    // Dezactivarea algoritmului Nagle
    int flag = 1;
    setsockopt(tcp_sock, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(int));

    // Facem socket-ul reutilizabil
    flag = 1;
    setsockopt(tcp_sock, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(int));


    // Conectare la server
    if(connect(tcp_sock, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        cerr << "error connect\n";
        exit(1);
    }


    // Trimiterea id-ului către server
    if(send(tcp_sock, argv[1], 1 + strlen(argv[1]), 0) <= 0) {
        cerr << "error send\n";
        exit(1);
    }


    // Actualizăm lista de file descriptori pfds
    pfds.push_back({tcp_sock, POLLIN, 0});
    pfds.push_back({0, POLLIN, 0}); // STDIN

    int rc; // pentru erori
    bool stop_subscriber = false;

    while(!stop_subscriber) {
        rc = poll(pfds.data(), pfds.size(), -1);

        if(rc < 0) {
            cerr << "Poll error\n";
            exit(1);
        }

        // Vedem ce conexiuni au noi date ce trebuie citite
        // 4 cazuri: TCP, UDP, STDIN și SUBSCRIBER
        for(auto &pfd : pfds) 
            if(pfd.revents & POLLIN) {

                // Evaluăm file descriptor-ul pfd.fd

                if(pfd.fd == tcp_sock) {
                    // Am primit un mesaj pe socket-ul TCP prin care comunicăm cu server-ul
                    parse_server_msg(tcp_sock);
                }

                else {   
                    //Am primit un mesaj de la STDIN
                    if(STDIN_message(tcp_sock) == true){
                        // oprim server-ul
                        stop_subscriber = true;
                        break;
                    }
                }
            }
    }

    close(tcp_sock);
    return 0;
    
}
