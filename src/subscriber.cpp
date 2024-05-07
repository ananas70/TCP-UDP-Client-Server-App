#include <iostream>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <vector>
#include <sys/poll.h>
#include <netinet/tcp.h>
#include "types.h"
#include "common.h"
#include <unistd.h>
#include <string>

using namespace std;
// SUBSCRIBER = CLIENT TCP 

FILE *history = fopen("src/history2", "wt");


vector <struct pollfd> pfds;    // vector in care stocam file descriptorii pentru multiplexare
                                // alocat dinamic pentru eficienta


void printRequest(const client_request* request) {
    fprintf(history, "---------------Client Request:---------------\n");
    fprintf(history, "Type: %c\n", request->type);
    fprintf(history, "Topic: %s\n", request->topic);
    fprintf(history, "\n");
    fprintf(history, "---------------------------------------------\n");
}



void build_request(struct client_request* request, char* buff) {
    char *p = strtok(buff, " ");
    if(p == NULL) {
        // am primit gol
        cerr << "No command received\n";
        return;
        fprintf(history, "\t\t null command received\n");
        fflush(history);
    }

    if(strcmp(p, "subscribe") == 0)
        request->type = 's';
    else if(strcmp(p, "unsubscribe") == 0)
        request->type = 'u';
    else {
        cerr << "Wrong command oops\n";
        return;
    }
    fprintf(history, "\t\t request type = %c\n\n", request->type);
    fflush(history);
    
    fprintf(history, "\t\t getting topic\n");
    fflush(history);
    // Preluare topic
    p = strtok(NULL, " ");
    if(p == NULL) {
        cerr << "No topic received\n";
        return;
    }

    if(strlen(p) > 55) {    // pune macro
        cerr << "Too long received\n";
        return;
    }

    fprintf(history, "\t\t Converting topic -> string here we go\n");
    strcpy(request->topic,p);
    request->topic[strlen(request->topic) - 1]  = '\0';
    fprintf(history, "\t\t request->topic = %s\n", request->topic);
    fflush(history);
    fprintf(history, "\t\t topic -> string DONE\n");
    fflush(history);
}


void submit_request(char* buff, int tcp_sock) {
    fprintf(history, "\t I'm in submit request function oh yeah\n");
     fprintf(history, "\t\t buffer is actually : %s\n", buff);
fflush(history);
    // Comenzi acceptate:
    // subscribe <TOPIC> 
    // unsubscribe <TOPIC>

    // pune un \0 la final in caz ca nu s-au primit fix 1500 de octeti??
    // buff[strlen(buff) - 1] = 0;

    struct client_request request;
    build_request(&request, buff);
    // Trimitere catre server
    if (send(tcp_sock, (char*) &request, sizeof(request),0) <= 0) {
        cerr << "Error send\n";
        return;
    }

    fprintf(history, "\t\t request sent\n");
    fflush(history);
    printRequest(&request);
    // Afisari
    if(request.type == 's')
        cout <<"Subscribed to topic "<< request.topic << endl;
    else
        cout <<"Unsubscribed from topic "<< request.topic << endl;

}



bool STDIN_message(int tcp_sock) {
    // returneaza 1 daca primeste exit, 0 altminteri

    // Mesajul pe care il trimiti trebuie sa ti-l construiesti tu, ba, geniule
    fprintf(history, "\t I'm in STD_message function \n");
    fflush(history);
    char buff[BUF_LEN];
    memset(buff, 0, BUF_LEN);
    fprintf(history, "\t\t Set the buffer to 0 \n");
fflush(history);
    int rc = read(0, buff, BUF_LEN);
    if(rc < 0) {
        cerr << "Error STDIN\n";
        return false;
    }
    fprintf(history, "\t\t read smth from the buffer oh yeah \n");
    fprintf(history, "\t\t buffer = %s \n", buff);
fflush(history);
    buff[rc] = '\0';

    if(strncmp(buff, "exit", 4) == 0) {
        //exit client routine
        fprintf(history, "\t\tgot exit command mars acas \n");
fflush(history);
        return true;
    }

    fprintf(history, "\t got NON-exit command\n");
fflush(history);
    // subscribe sau unsubscribe
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
        cerr << "Server ended connection\n";
        close(tcp_sock);
        exit(1);
    }
 
    // afisare mesaj primit

    client_notification* notif = (client_notification*) buff;

    // <IP_CLIENT_UDP>:<PORT_CLIENT_UDP> - <TOPIC> - <TIP_DATE> - <VALOARE_MESAJ>

    printf("%s:%hu - ", inet_ntoa(*(struct in_addr *) &notif->ip_client_udp), ntohs(notif->port_client_udp));
    cout << notif->topic<< " - " << notif->data_type << " - " << notif->content << endl; 

    fprintf(history, "\t Exit parse_server_msg\n");
    fflush(history);  
}

int main(int argc, char *argv[]) {
    setvbuf(stdout, NULL, _IONBF, BUFSIZ);  //buffering la afisare dezactivat
    fprintf(history, "\t SUBSCRIBER INITIATED\n");
    fflush(history);
    if (history == nullptr) {
        // Handle error if unable to open the file
        perror("Error opening file2");
        return 1;
    }
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

    fprintf(history, "\t colectat informatii server\n");
    fflush(history);


    // creare socket TCP
    int tcp_sock = socket(AF_INET, SOCK_STREAM, 0);
    if(tcp_sock < 0){
        cerr << "[CLIENT] Error while creating TCP socket\n";
        exit(1);
    }

    fprintf(history, "\t creat socket tcp \n");
    fflush(history);


    //dezactivam nagle
    int flag = 1;
    setsockopt(tcp_sock, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(int));
//     int bufsize = 0;
// setsockopt(tcp_sock, SOL_SOCKET, SO_SNDBUF, &bufsize, sizeof(int));
    fprintf(history, "\tdezactivat nagle \n");
    fflush(history);

    // make reusable
    flag = 1;
    setsockopt(tcp_sock, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(int));

    fprintf(history, "\t made reusable\n");
    fflush(history);

    // conectare la server
    if(connect(tcp_sock, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        cerr << "error connect\n";
        exit(1);
    }

    fprintf(history, "\tconnected to server \n");
    fflush(history);

    //trimite id
    if(send_all(tcp_sock, argv[1], 1 + strlen(argv[1])) <= 0) {   //nuj de ce e +1
        cerr << "error send\n";
        exit(1);
    }


    // Actualizam lista de file descriptori pfds
    pfds.push_back({tcp_sock, POLLIN, 0});
    pfds.push_back({0, POLLIN, 0}); // STDIN
    fprintf(history, "\t updated pfds\n");
    fflush(history);
    int rc; // for errors
    bool stop_subscriber = false;
    fprintf(history, "Starting while loop \n");
    fflush(history);
    while(!stop_subscriber) {
        fprintf(history, "\t-------------- Waiting ---------------------\n");
        fflush(history);
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
                    fprintf(history, "\t parse_server_msg \n");
                    fflush(history);
                    // Am primit un mesaj pe socket-ul tcp cu care comunicam cu server-ul
                    parse_server_msg(tcp_sock);
                    fprintf(history, "\t parse_server_msg STOPPED \n");
                    fflush(history);
                }

                else {   
                    fprintf(history, "\t STDIN_message \n");
                    fflush(history);
                    //STDIN
                    if(STDIN_message(tcp_sock) == true){
                        // oprim server-ul
                        stop_subscriber = true;
                        break;
                    }

                }

            }
    }

    close(tcp_sock);
    fclose(history);
    return 0;
}