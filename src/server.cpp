#include <iostream>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <vector>
#include <sys/poll.h>
#include <types.h>
#include <cmath>
#include <netinet/tcp.h>
#include <common.h>
#include <unistd.h>
#include <experimental/optional>
#include <algorithm>

using namespace std;
// SUBSCRIBER = CLIENT TCP 

vector <struct pollfd> pfds;    // vector in care stocam file descriptorii pentru multiplexare
                                // alocat dinamic pentru eficienta

vector <struct client_data> clients;    // clientii TCP (subscriber-ii)


void UDP_connection(int udp_sock, struct sockaddr_in udp_addr) {
    // Primesti si parsezi datele primite, iei topic-ul si datele despre el si notifici clientii
    /* La primirea unui mesaj UDP valid, serverul trebuie sa asigure trimiterea acestuia catre
    toti clientii TCP care sunt abonati la topic-ul respectiv.*/
    // mai trebuie sa verificam ca sunt activi???

    socklen_t udp_socklen = sizeof(udp_addr);
    char buff[BUF_LEN];
    memset(buff, 0 , BUF_LEN);

    //Receive data from the UDP client
    if(recvfrom(udp_sock, buff, BUF_LEN, 0 , (struct sockaddr *) &udp_addr, &udp_socklen) < 0) {
        cerr << "Error while receiving UDP client's msg\n";
        exit(1);
    }

    struct udp_pckt* message = (struct udp_pckt *) buff;

    // Construim notificarea catre clientii TCP abonati la topic - client_notification

    struct client_notification* client_pckt;
    memset(client_pckt, 0 , sizeof(struct client_notification));

    //<IP_CLIENT_UDP>:<PORT_CLIENT_UDP> - <TOPIC> - <TIP_DATE> - <VALOARE_MESAJ>
    client_pckt->ip_client_udp = udp_addr.sin_addr.s_addr;  //nuj daca e bine
    client_pckt->port_client_udp = udp_addr.sin_port;
    client_pckt->topic = string(message->topic, 50);

    switch (message->data_type) {
    case 0:
        // INT
        strcpy(client_pckt->data_type, "INT");
        // Octet de semn urmat de un uint32_t formatat conform network byte order

        uint32_t data = ntohl(*(uint32_t *)(message->content + 1)); 
        int sign = message->content[0];
        if (sign == 1)
            data = - data;

        // Using snprintf to safely convert uint32_t to a string
        sprintf(client_pckt->content, "%u", data);
        break;

    case 1:
        // SHORT REAL
        strcpy(client_pckt->data_type, "SHORT_REAL");

        // uint16_t reprezentand modulul numarului inmultit cu 100
        uint16_t data = ntohs(*(uint16_t *)(message->content));
        sprintf(client_pckt->content, "%.2f", (data/100.0));
        break;
    case 2:
        // FLOAT
        strcpy(client_pckt->data_type, "FLOAT");
        /* Un octet de semn, urmat de un uint32_t (in network order) reprezentand modulul numarului obtinut din
        alipirea partii intregi de partea zecimala a numarului, urmat de un uint8_t ce reprezinta modulul puterii 
        negative a lui 10 cu care trebuie inmultit modulul pentru a obtine numarul original (in modul) */
        uint32_t data;
        memcpy(&data, message->content + 1, sizeof(uint32_t));
        data = ntohl(data);
        int sign = message->content[0];
        if (sign == 1)
            data = - data;
        int exp = message->content[5];
        sprintf(client_pckt->content, "%lf", data / pow(10, exp));
        break;
    case 3:
        // STRING 
        strcpy(client_pckt->data_type, "STRING");

        /*Sir de maxim 1500 de caractere, terminat cu \0 sau delimitat de finalul datagramei pentru lungimi
        mai mici*/
        strcpy(client_pckt->content, message->content);
        break;        
    default:
        cerr << "Unknown data type sent bu UDP client\n";
        break;
    }

    // Trimite notificarea (vezi la ce sunt abonati nebunii aia)
    for(auto& client : clients) 
        for(auto& subscription : client.subscriptions) 
            if (client_pckt->topic == subscription && client.connected) {
                int rc = send(client.fd, client_pckt, sizeof(client_pckt), 0);
                if (rc < 0) {
                    cerr << "Send error\n";
                    return;
                }
            }
}

void TCP_connection(int tcp_sock, struct sockaddr_in tcp_addr) {
    // tcp_sock e cel pasiv (cel de listen)
    struct sockaddr_in cli_addr;
    socklen_t cli_socklen = sizeof(cli_addr);

    memset(&cli_addr, 0, cli_socklen);

    int cli_sock = accept(tcp_sock, (struct sockaddr *) &cli_addr, &cli_socklen);
    if(cli_sock < 0) {
        cerr << "Can't accept\n";
        exit(1);
    }

    // dezactivam algoritmul lui Nagle
    int flag = 1;
    setsockopt(cli_sock, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(int));

    // preluam id-ul clientului

    char* id_client = new char[ID_LEN]; //nuj cat trebuie luat
    int bytes_read = recv_all(cli_sock, id_client, ID_LEN);
    char *resized_id_client = new char[bytes_read];
    copy(id_client, id_client + ID_LEN, resized_id_client);
    delete[] resized_id_client;
    id_client = resized_id_client;

    bool found_client = false;
    int i;
    // vedem daca deja exista in baza noastra de date
    for(i = 0 ; i < clients.size() && !found_client ; i ++)
        if(strcmp(id_client, clients[i].id) == 0)
            found_client = true;

    if(!found_client) {
        // Client NOU
        // Ii adaugam pe el si socket-ul lui
        pfds.push_back({cli_sock, POLLIN, 0});
        struct client_data new_client;
        strcpy(new_client.id, id_client);
        new_client.connected = true;
        new_client.fd = cli_sock;
        // alte campuri aditionale ale clientului

        clients.push_back(new_client);

        //New client <ID_CLIENT> connected from IP:PORT.
        printf("New client %s connected from %s:%d\n", id_client, inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));
    }

    else {
        // Clientul deja exista. Verificam sa vedem daca nu cumva e deja conectat (sau nu, poate s-a reconectat)
        
        // Refolosim contorul i unde s-a oprit cautarea in for

        // Daca e deja conectat, mars acas
        if(clients[i].connected == true){
            printf("Client %s already connected.\n", id_client);
            //exit routine -- eventual completezi
            close(cli_sock);
        }
        

        // S-a reconectat. Adaugam iar conexiunea
        else {
            pfds.push_back({cli_sock, POLLIN, 0});
            clients[i].connected = true;
            clients[i].fd = cli_sock;
            printf("New client %s connected from %s:%d\n", id_client, inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));
        }
    }

    // sterge la final id-ul alocat dinamic
    delete[] id_client; 

}

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
    return false;
}


struct client_data* get_client_by_fd(int fd) {
    // cauta un client conectat cautandu-l dupa file descriptor
    for(auto &c : clients)
        if(c.fd == fd)
            return &c;
    return NULL;
}

void remove_pfd(struct pollfd* pfd) {
    for (auto it = pfds.begin(); it != pfds.end(); ++it) {
        if (&(*it) == pfd) {
            pfds.erase(it);
            break;
        }
    }
}


void handle_client_request(struct client_request* request, int fd, struct client_data* client) {

    auto it = find(client->subscriptions.begin(),client->subscriptions.end(), request->topic);

    if(request->type == 's') {
        // subscribe
        if(it != client->subscriptions.end())
            return; // e deja abonat

        // altfel, adaugam abonamentul (m-as abona la inima ta)
        client->subscriptions.push_back(request->topic);
    }
    else {
        //unsubscribe
        if (it != client->subscriptions.end())
            client->subscriptions.erase(it);
    }
}

void SUBSCRIBER_connection(int tcp_sock, struct sockaddr_in tcp_addr, pollfd pfd) {
    char buff[BUF_LEN];
    memset(buff, 0 , BUF_LEN);

    struct client_data* found_client = get_client_by_fd(pfd.fd);
    int bytes_read = recv_all(pfd.fd, buff, SUBSCRIBER_CMD_LEN);
    if(bytes_read == 0) {
        printf("Client %s disconnected.\n", found_client->id);

        // stergem din clients si din pfds

        remove_pfd(&pfd);
        close(pfd.fd);
        pfd.fd = -1;
        // dam si unsubscribe???
        found_client->connected = false;
        found_client->fd = -1;
    }

    else {
        // SUBSCRIBE sau UNSUBSCRIBE
        // (un)subscribe <TOPIC>
        struct client_request* request = (struct client_request*) buff;
        handle_client_request(request, pfd.fd, found_client);

    }
}


int main(int argc, char *argv[]) {
    // ??? in loc de exit(1) peste tot ar trebui sa dai terminate_server si sa inchizi toti socket-ii si dupa aia sa dai exit(1)?

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

    // Facem socket-ii reutilizabili
    int flag = 1;
    setsockopt(tcp_sock, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(int));

    flag = 1; // chiar e nevoie ?
    setsockopt(udp_sock, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(int));

    // Dezactivam Nagle pentru socket-ul TCP de listen
    flag = 1;
    setsockopt(tcp_sock, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(int));

    /* Set port and IP for TCP */
    tcp_addr.sin_family = AF_INET;
    tcp_addr.sin_port = htons(server_port);
    tcp_addr.sin_addr.s_addr = INADDR_ANY; // e bine asa sau trebuie fara s_addr?

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

    int rc; // for errors
    bool stop_server = false;
    while(!stop_server) {
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

                if(pfd.fd == udp_sock) {

                    UDP_connection(udp_sock, udp_addr);
                }

                else if(pfd.fd == tcp_sock) {

                    TCP_connection(tcp_sock, tcp_addr);
                }

                else if(pfd.fd == 0) {   
                    //STDIN
                    bool stop = STDIN_message();
                    if(stop == true){
                        // oprim server-ul
                        stop_server = true;
                        break;
                    }

                }
                
                else {
                    //Subscriber (client tcp)
                    SUBSCRIBER_connection(tcp_sock, tcp_addr, pfd);
                }
            }
    }

    //close chestii
}