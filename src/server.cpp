#include <iostream>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <vector>
#include <sys/poll.h>
#include "common.h"
#include "types.h"
#include <cmath>
#include <netinet/tcp.h>
#include <unistd.h>
#include <experimental/optional>
#include <algorithm>

using namespace std;
// SUBSCRIBER = CLIENT TCP 
FILE *history = fopen("src/history", "wt");

vector <struct pollfd> pfds;    // vector in care stocam file descriptorii pentru multiplexare
                                // alocat dinamic pentru eficienta

vector <struct client_data> clients;    // clientii TCP (subscriber-ii)


void printClients() {
    fprintf(history, "--------Printing all clients:-----------\n");
    for (size_t i = 0; i < clients.size(); ++i) {
        const auto& client = clients[i];
        fprintf(history, "Index: %zu\n", i);
        fprintf(history, "Client ID: %s\n", client.id);
        fprintf(history, "Connected: %s\n", (client.connected ? "Yes" : "No"));
        fprintf(history, "File Descriptor: %d\n", client.fd);
        fprintf(history, "Subscriptions:\n");
        for (const auto& sub : client.subscriptions) {
            fprintf(history, "- %s\n", sub.c_str());
        }
        fprintf(history, "\n");
    }
    fprintf(history, "----------------------------------\n");
}

void build_notification(client_notification* client_pckt, struct sockaddr_in udp_addr, struct udp_pckt* message) {
    client_pckt->ip_client_udp = udp_addr.sin_addr.s_addr;  //nuj daca e bine // mai facem ntohl??
    fprintf(history, "\t Initializat IP\n");
    fflush(history);
    client_pckt->port_client_udp = udp_addr.sin_port;
    fprintf(history, "\t Initializat PORT\n");
    fflush(history);
    strcpy(client_pckt->topic, message->topic);
    fprintf(history, "\t Initializat TOPIC\n");
    fflush(history);
    fprintf(history, "\t Construit pachet UDP\n");
    fflush(history);
    uint32_t data_32;
    uint16_t data_16;
    int sign;

    fprintf(history, "\t Analiza :\n");
    fflush(history);
    switch (message->data_type) {
    case 0:
    {
        // INT
        strcpy(client_pckt->data_type, "INT");
        // Octet de semn urmat de un uint32_t formatat conform network byte order

        data_32 = ntohl(*(uint32_t *)(message->content + 1)); 
        sign = message->content[0];
        if (sign == 1)
            data_32 = - data_32;

        // Using snprintf to safely convert uint32_t to a string
        sprintf(client_pckt->content, "%u", data_32);
        break;
    }
    case 1:
    {
        // SHORT REAL
        strcpy(client_pckt->data_type, "SHORT_REAL");

        // uint16_t reprezentand modulul numarului inmultit cu 100
        data_16 = ntohs(*(uint16_t *)(message->content));
        sprintf(client_pckt->content, "%.2f", (data_16/100.0));
        break;
    }
    case 2:
    {
        // FLOAT
        strcpy(client_pckt->data_type, "FLOAT");
        /* Un octet de semn, urmat de un uint32_t (in network order) reprezentand modulul numarului obtinut din
        alipirea partii intregi de partea zecimala a numarului, urmat de un uint8_t ce reprezinta modulul puterii 
        negative a lui 10 cu care trebuie inmultit modulul pentru a obtine numarul original (in modul) */
        memcpy(&data_32, message->content + 1, sizeof(uint32_t));
        data_32 = ntohl(data_32);
        sign = message->content[0];
        if (sign == 1)
            data_32 = - data_32;
        int exp = message->content[5];
        sprintf(client_pckt->content, "%lf", data_32 / pow(10, exp));
        break;
    }
    case 3:
    {
        // STRING 
        strcpy(client_pckt->data_type, "STRING");

        /*Sir de maxim 1500 de caractere, terminat cu \0 sau delimitat de finalul datagramei pentru lungimi
        mai mici*/
        strcpy(client_pckt->content, message->content);
        break; 
    }
    default:
    {
        cerr << "Unknown data type sent bu UDP client\n";
        break;
    }
    }

}

void printNotification(const client_notification* notification) {
    printf("<%s>:%d - %s - %s - %s\n",
           inet_ntoa(*((struct in_addr*)&notification->ip_client_udp)),
           ntohs(notification->port_client_udp),
           notification->topic,
           notification->data_type,
           notification->content);
}


void UDP_connection(int udp_sock, struct sockaddr_in udp_addr) {
    // Primesti si parsezi datele primite, iei topic-ul si datele despre el si notifici clientii
    /* La primirea unui mesaj UDP valid, serverul trebuie sa asigure trimiterea acestuia catre
    toti clientii TCP care sunt abonati la topic-ul respectiv.*/
    // mai trebuie sa verificam ca sunt activi???

    socklen_t udp_socklen = sizeof(udp_addr);
    char buff[BUF_LEN];
    memset(buff, 0 , BUF_LEN);

    int bytesread = recvfrom(udp_sock, buff, BUF_LEN, 0 , (struct sockaddr *) &udp_addr, &udp_socklen);
    //Receive data from the UDP client
    if(bytesread < 0) {
        cerr << "Error while receiving UDP client's msg\n";
        exit(1);
    }

    fprintf(history, "\t Read UDP buffer, bytesread = %d, buffer = %s, size = %d \n", bytesread, buff, strlen(buff));
    fflush(history);
    struct udp_pckt* message = reinterpret_cast<udp_pckt*>(buff);
    fprintf(history, "\t Initializat message\n");
    fflush(history);
    // Construim notificarea catre clientii TCP abonati la topic - client_notification

    struct client_notification client_pckt;
    fprintf(history, "\t Initializat client_pckt\n");
    fflush(history);
    //<IP_CLIENT_UDP>:<PORT_CLIENT_UDP> - <TOPIC> - <TIP_DATE> - <VALOARE_MESAJ>

    build_notification(&client_pckt, udp_addr, message);

    fprintf(history, "\t Trimite catre clienti pentru topic-ul [%s]\n", client_pckt.topic);
    fflush(history);
    printClients();
    // Trimite notificarea (vezi la ce sunt abonati nebunii aia)
    for(auto& client : clients) {
        fprintf(history, "\t\tClientul din for  = %s\n", client.id);
        fflush(history);
        for (const auto& sub : client.subscriptions) {
            fprintf(history, "[%s]\n", sub.c_str());
            fflush(history);
        }
        for(auto& subscription : client.subscriptions) {
            if (string(client_pckt.topic) == subscription && client.connected) {
                
                fprintf(history, "\t\t\tFound client!! = %s with fd = %d\n", client.id, client.fd);
                fprintf(history, "\t\t\tSending a message to him\n");
                fflush(history);
                int rc = send(client.fd, (char*) &client_pckt, sizeof(client_pckt), 0);
                if (rc <= 0) {
                    cerr << "Send error\n";
                    return;
                }
                fprintf(history, "\t\t\tSent [%d] data to him\n", rc);
                fflush(history);
            }
        }
    }
}

void TCP_connection(int tcp_sock) {
    fprintf(history, "im in TCP connection\n");
    // tcp_sock e cel pasiv (cel de listen)
    struct sockaddr_in cli_addr;
    socklen_t cli_socklen = sizeof(cli_addr);

    memset(&cli_addr, 0, cli_socklen);

    int cli_sock = accept(tcp_sock, (struct sockaddr *) &cli_addr, &cli_socklen);

    if(cli_sock < 0) {
        cerr << "Can't accept\n";
        exit(1);
    }

    // Facem socket-ii reutilizabili
    int flag = 1;
    setsockopt(tcp_sock, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(int));
    // dezactivam algoritmul lui Nagle
    // int flag = 1;
    // setsockopt(cli_sock, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(int));
    flag = 1;
    setsockopt(tcp_sock, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(int));
    
    fprintf(history, "\t Accepted client\n");
    fflush(history);
    fprintf(history, "\t Nagle\n");
    fflush(history);

    // preluam id-ul clientului

    char* id_client = new char[ID_LEN]; //nuj cat trebuie luat

    fprintf(history, "\t Vom citi id-ul clientului\n");
    fflush(history);

    int bytes_read = recv(cli_sock, id_client, ID_LEN, 0);
    if(bytes_read <= 0) {
        cerr << "Nothing received\n";
        return;
    }

    fprintf(history, "\t Am citit id-ul clientului\n");
    fflush(history);

    char *resized_id_client = new char[bytes_read];
    std::copy(id_client, id_client + ID_LEN, resized_id_client);
    // id_client = resized_id_client;
    delete[] resized_id_client;

    bool found_client = false;
    long unsigned int i;
    // vedem daca deja exista in baza noastra de date
    for(i = 0 ; i < clients.size() && !found_client ; i ++)
        if(strcmp(id_client, clients[i].id) == 0)
            found_client = true;

    if(!found_client) {
            fprintf(history, "\t New client\n");
            fflush(history);
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
        printf("New client %s connected from %s:%hu.\n", id_client, inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));
    }

    else {
        i--;
        fprintf(history, "\t client already existing\n");
            fflush(history);
            fprintf(history, "\t client INDEX = %d\n", i);
            fflush(history);
            printClients();
        // Clientul deja exista. Verificam sa vedem daca nu cumva e deja conectat (sau nu, poate s-a reconectat)
        
        // Refolosim contorul i unde s-a oprit cautarea in for

        // Daca e deja conectat, mars acas
        if(clients[i].connected == true){
            fprintf(history, "\t deja conectat, mars acas\n");
            fflush(history);
            printf("Client %s already connected.\n", id_client);
            //exit routine -- eventual completezi
            close(cli_sock);
        }
        

        // S-a reconectat. Adaugam iar conexiunea
        else {
            fprintf(history, "\t S-a reconectat. Adaugam iar conexiunea\n");
            fflush(history);
            pfds.push_back({cli_sock, POLLIN, 0});
            clients[i].connected = true;
            clients[i].fd = cli_sock;
            // printf("New client %s connected from %s:%d.\n", id_client, inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));
        }
    }

    // sterge la final id-ul alocat dinamic
    delete[] id_client; 
    fprintf(history, "\t Exit TCP_connection\n");
    fflush(history);

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
    fprintf(history, "\t Exit STDIN_message\n");
    fflush(history);
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

void printRequest(const client_request* request) {
    fprintf(history, "---------------Client Request:---------------\n");
    fprintf(history, "Type: %c\n", request->type);
    fprintf(history, "Topic: %s\n", request->topic);
    fprintf(history, "\n");
    fprintf(history, "---------------------------------------------\n");
}

void handle_client_request(struct client_request* request, struct client_data* client) {
    fprintf(history, "I'm in handle_client_request \n");
    fflush(history);
    printRequest(request);
    printClients();
    auto it = find(client->subscriptions.begin(),client->subscriptions.end(), string(request->topic));

    if(request->type == 's') {
        // subscribe
        if(it != client->subscriptions.end())
            return; // e deja abonat

        // altfel, adaugam abonamentul (m-as abona la inima ta)
        client->subscriptions.push_back(string(request->topic));
    }
    else {
        //unsubscribe
        if (it != client->subscriptions.end())
            client->subscriptions.erase(it);
    }
    printClients();
    fprintf(history, "Exit handle_client_request \n");
        fflush(history);

}

void SUBSCRIBER_connection(pollfd pfd) {
    char buff[SUBSCRIBER_CMD_LEN];
    memset(buff, 0 , SUBSCRIBER_CMD_LEN);

    fprintf(history, "\t Finding client by id \n");
    fflush(history);

    struct client_data* found_client = get_client_by_fd(pfd.fd);
    // if(found_client == NULL) {
    //     cerr<<"error finding client";
    //     fprintf(history, "\t Error finding client with get_client_by_fd \n");
    //     fflush(history);
    //     return;
    // }
    fprintf(history, "\t Reading data from client \n");
    fflush(history);


    int flag = 1;
    setsockopt(pfd.fd, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(int));


    int bytes_read = recv(pfd.fd, buff, sizeof(client_request) + 1, 0);
    if(bytes_read < 0) {
        fprintf(history, "\t Error recv \n");
        fflush(history);
        return;
    }
    if(bytes_read == 0) {
        fprintf(history, "\t Disconnected client \n");
        fflush(history);
        printf("Client %s disconnected.\n", found_client->id);

        // stergem din clients si din pfds

        remove_pfd(&pfd);
        close(pfd.fd);
        pfd.fd = -1;
        // dam si unsubscribe???
        found_client->connected = false;
        found_client->fd = -1;
        return;
    }

    else {
    fprintf(history, "\t Connected client. Parsing command.\n");
    fprintf(history, "\t BUFFER = %s, length = %d \n", buff, strlen(buff));
    fprintf(history, "\t bytes read = %d \n", bytes_read);
        fflush(history);

    if(buff == NULL) {
    fprintf(history, "\tEmpty buffer\n");
    fflush(history);
    return;
    }
        // SUBSCRIBE sau UNSUBSCRIBE
        // (un)subscribe <TOPIC>
        client_request* request = (client_request*)buff;
        // printRequest(request);
        fprintf(history, "\t request->type = %c\n", request->type);
        fflush(history);
        fprintf(history, "\t request->topic = %s\n", request->topic);
        fflush(history);
        fprintf(history, "\t BUFFER = %s, length = %d \n", buff, strlen(buff));
        fprintf(history, "\t Going to handle_client_request \n");
        fflush(history);
        handle_client_request(request, found_client);

    }
        fprintf(history, "\t Exit SUBSCRIBER_connection\n\n\n");
        fflush(history);
}



int main(int argc, char *argv[]) {
    // ??? in loc de exit(1) peste tot ar trebui sa dai terminate_server si sa inchizi toti socket-ii si dupa aia sa dai exit(1)?

    setvbuf(stdout, NULL, _IONBF, BUFSIZ);  //buffering la afișare dezactivat
    //
    if (history == nullptr) {
        // Handle error if unable to open the file
        perror("Error opening fileeeeeee");
        return 1;
    }
    //
    fprintf(history, "SERVER initiated: \n");
    fflush(history);

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
    // flag = 1;
    // setsockopt(tcp_sock, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(int));
    flag = 1;
    setsockopt(tcp_sock, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(int));

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
    fprintf(history, "Entering while loop: \n");
    fflush(history);
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
                    fprintf(history, "UDP_connection \n");
                    fflush(history);

                    UDP_connection(udp_sock, udp_addr);
                }

                else if(pfd.fd == tcp_sock) {
                    fprintf(history, "TCP_connection\n");
                    fflush(history);

                    TCP_connection(tcp_sock);
                }

                else if(pfd.fd == 0) {   
                    //STDIN
                    fprintf(history, "STDIN message\n");
                    fflush(history);
                    if(STDIN_message() == true){
                        // oprim server-ul
                        stop_server = true;
                        break;
                    }

                }
                
                else {
                    //Subscriber (client tcp)
                    fprintf(history, "SUBSCRIBER_connection\n");
                    fflush(history);
                    SUBSCRIBER_connection(pfd);
                    break;
                }
            }
            // erase closed file descriptors from the vector
		pfds.erase(
			remove_if(pfds.begin(), pfds.end(), [](pollfd p_fd) { return p_fd.fd == -1; }),
			pfds.end());
        if(stop_server == true)
            break;
    }

    //close chestii
    close(tcp_sock);
    close(udp_sock);
    fclose(history);
    return 0;
}