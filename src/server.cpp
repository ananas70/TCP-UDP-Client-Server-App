#include <iostream>
#include <stdio.h>
#include <string.h>
#include <sstream>
#include <cmath>
#include <unistd.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <sys/poll.h>
#include <vector>
#include <algorithm>

#include "types.h"

using namespace std;

vector <struct pollfd> pfds;    // Vector in care stocăm file descriptorii pentru multiplexare


vector <struct client_data> clients;    // Vector in care stocăm toți clientii TCP (subscriberii)



void build_notification(client_notification* client_pckt, struct udp_pckt* message) {
    uint32_t data_32;
    uint16_t data_16;
    int sign;

    switch (message->data_type) {
        case 0:
        {
            // INT
            strcpy(client_pckt->data_type, "INT");

            // Octet de semn urmat de un uint32_t formatat conform network byte order

            data_32 = ntohl(*(uint32_t *)(message->content + 1)); 
            sign = message->content[0];
            if (sign)
                data_32 = - data_32;

            sprintf(client_pckt->content, "%d", data_32);
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

            double x = ntohl(*(uint32_t*)(message->content + 1));
            sign = message->content[0];
            if (sign == 1)
                x = - x;
            int exp = message->content[5];
            sprintf(client_pckt->content, "%lf", x / pow(10, exp));
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
            cerr << "Unknown data type = ["<< message->data_type<<"] sent bu UDP client\n";
            break;
        }
    }
}


bool match_topic(string pattern, string topic) {
    // Verifică dacă un topic cu wildcard (pattern) se potrivește cu un topic fără wildcard (topic)

    // Citim fiecare cuvânt individual (separat prin '/') din cele două string-uri
    stringstream pattern_stream(pattern), topic_stream(topic);

    string pattern_word, topic_word;    // Cuvintele individuale
    while (getline(pattern_stream, pattern_word, '/')) {
        if (!getline(topic_stream, topic_word, '/')) {
            // Dacă topic e mai scurt decât pattern, întoarcem fals
            return false;
        }
        if (pattern_word != "*" && pattern_word != "+" && pattern_word != topic_word) {
            // Dacă ambele sunt cuvinte (non - wildcard) și nu se potrivesc, întoarcem fals
            return false;
        }

        if (pattern_word == "+") {
            // Dacă cuvântul curent din pattern e wildcard-ul '+', sărim un pas
            continue;
        }

        if (pattern_word == "*") {
            // Dacă cuvântul din pattern e wildcard-ul '*'
            if(!getline(pattern_stream, pattern_word, '/'))
                // Dacă steluța e ultimul cuvânt din pattern
                return true;

            // Acum, pattern_word are stocat în el primul cuvânt de după steluță
            // Îl vom folosi pentru verificare

            bool found_next = false;
            // Iterăm în topic până dăm de pattern_word
            while (getline(topic_stream, topic_word, '/')) {
                if(topic_word == pattern_word) {
                    //Resetare la începutul for-ului
                    found_next = true;
                    break;                
                }
            }

            if(!found_next)
                return false;
        }
    }

    // Dacă topic e prea lung (a mai rămas conținut în el)
    if(getline(topic_stream, topic_word, '/'))
        return false;
    return true;
}


void send_notification_to_client(client_data client, char* topic, client_notification client_pckt) {

    for(auto& subscription : client.subscriptions) {
        bool ok = false;

            // Vedem dacă are wildcard sau nu
            if(subscription.find_first_of("*+") != string::npos) {
                // ARE WILDCARD

                if((string(topic) == "*" || match_topic(subscription, string(topic))) && client.connected)
                    ok = true;
            }
            else {
                // NU ARE WILDCARD

                if (string(topic) == subscription && client.connected)
                    ok = true;
            }

            if(ok) {
                
                int rc = send(client.fd, (char*) &client_pckt, sizeof(client_pckt), 0);
                if (rc <= 0) {
                    cerr << "Send error\n";
                    return;
                }
                return; // Iese din funcție pentru a nu trimite de două ori pe același topic
            }
    }
}


void UDP_connection(int udp_sock, struct sockaddr_in udp_addr) {
    /* 
        Primesc și parsez datele primite, iau topic-ul și notific clienții
        La primirea unui mesaj UDP valid, serverul trebuie să asigure trimiterea acestuia către
    toți clienții TCP care sunt abonați la topic-ul respectiv.

    */

    socklen_t udp_socklen = sizeof(udp_addr);
    int LEN = sizeof(udp_pckt);
    udp_pckt* message = new udp_pckt;
    memset(message, 0 , sizeof(udp_pckt));

    int bytesread = recvfrom(udp_sock, message, LEN, 0 , (struct sockaddr *) &udp_addr, &udp_socklen);

    if(bytesread <= 0) {
        cerr << "Error while receiving UDP client's msg\n";
        close(udp_sock);
        exit(1);
    }

    // Construim notificarea (client_notification) către clienții TCP abonați la topic

    struct client_notification client_pckt;
    memset(&client_pckt, 0, sizeof(client_notification));

    strncpy(client_pckt.topic, message->topic, TOPIC_LEN);
    build_notification(&client_pckt, message);

    // Trimite notificarea celor care sunt abonațî
    for(auto& client : clients)
        send_notification_to_client(client, message->topic, client_pckt);

}

void TCP_connection(int tcp_sock) {
    // tcp_sock e socket-ul pasiv (cel de listen)

    struct sockaddr_in cli_addr;
    socklen_t cli_socklen = sizeof(cli_addr);

    memset(&cli_addr, 0, cli_socklen);

    int cli_sock = accept(tcp_sock, (struct sockaddr *) &cli_addr, &cli_socklen);

    if(cli_sock < 0) {
        cerr << "Eroor: accept - ser\n";
        close(tcp_sock);
        exit(1);
    }

    // Facem sockeții reutilizabili
    int flag = 1;
    setsockopt(tcp_sock, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(int));
    
    // Dezactivăm algoritmul lui Nagle
    flag = 1;
    setsockopt(tcp_sock, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(int));
    
    // Preluăm id-ul clientului

    char* id_client = new char[ID_LEN];

    int bytes_read = recv(cli_sock, id_client, ID_LEN, 0);
    if(bytes_read <= 0) {
        cerr << "Nothing received\n";
        return;
    }

    char *resized_id_client = new char[bytes_read];
    std::copy(id_client, id_client + ID_LEN, resized_id_client);
    delete[] resized_id_client;

    bool found_client = false;

    long unsigned int i;
    // vedem dacă clientul deja există stocat în clients
    for(i = 0 ; i < clients.size() && !found_client ; i ++)
        if(strcmp(id_client, clients[i].id) == 0)
            found_client = true;

    if(!found_client) {
        // Client NOU
        struct client_data new_client;

        pfds.push_back({cli_sock, POLLIN, 0});
        strcpy(new_client.id, id_client);
        new_client.connected = true;
        new_client.fd = cli_sock;

        clients.push_back(new_client);

        //New client <ID_CLIENT> connected from IP:PORT.
        printf("New client %s connected from %s:%hu.\n", id_client, inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));
    }

    else {
        i--;
        // Clientul deja există. Verificăm să vedem dacă nu cumva e deja conectat
        
        // Refolosim contorul i unde s-a oprit căutarea în for

        // Dacă e deja conectat, nu i se permite să se conecteze
        if(clients[i].connected == true){
            printf("Client %s already connected.\n", id_client);
            close(cli_sock);
        }

        // Dacă s-a reconectat, adăugăm iar conexiunea
        else {
            pfds.push_back({cli_sock, POLLIN, 0});
            clients[i].connected = true;
            clients[i].fd = cli_sock;
            printf("New client %s connected from %s:%d.\n", id_client, inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));
        }
    }

    // Șterge la final id-ul alocat dinamic
    delete[] id_client; 

}

bool STDIN_message() {
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

    if(strncmp(buff, "exit", 4) == 0)
        return true;
    else 
        cerr << "Unsupported command";

    return false;
}


struct client_data* get_client_by_fd(int fd) {
    // Caută un client conectat (căutându-l după file descriptor)
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


void subscribe(struct client_data* client, string wanted_topic, char type) {
    auto it = find(client->subscriptions.begin(),client->subscriptions.end(), wanted_topic);

    if(type == 's') {
        // subscribe
        if(it != client->subscriptions.end())
            return; // e deja abonat

        // altfel, adăugăm abonamentul
        client->subscriptions.push_back(wanted_topic);
    }
    else {
        //unsubscribe
        if (it != client->subscriptions.end())
            client->subscriptions.erase(it);
    }
}


void unsubscribe_wildcard(struct client_data* client, string wanted_topic) {
    // Produce efectul lui unsubscribe dacă, ca topic, s-a primit un șir cu wildcards
    if(wanted_topic == "*") {
        // unsubscribe la toate topic-urile
        client->subscriptions.clear();
        return;
    }
    
    // Unsubscribe doar la topic-urile care se potrivesc (match_topic)
    int size = (client->subscriptions).size();

    for(int i = 0; i < size; i++) {
        if(match_topic(wanted_topic, client->subscriptions[i])) {
            client->subscriptions.erase(client->subscriptions.begin() + i);
            --size;
            --i;
        }
    }
}


void handle_client_request(struct client_request* request, struct client_data* client) {

    string wanted_topic = string(request->topic);

    // Vedem dacă are wildcard sau nu
    if(wanted_topic.find_first_of("*+") != string::npos && request->type == 'u')
        unsubscribe_wildcard(client, string(request->topic));
    else 
        subscribe(client,string(request->topic), request->type);


}

void SUBSCRIBER_connection(pollfd pfd) {
    char buff[SUBSCRIBER_CMD_LEN];
    memset(buff, 0 , SUBSCRIBER_CMD_LEN);

    struct client_data* found_client = get_client_by_fd(pfd.fd);

    int flag = 1;
    setsockopt(pfd.fd, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(int));

    int bytes_read = recv(pfd.fd, buff, sizeof(client_request) + 1, 0);
    if(bytes_read < 0) {
        return;
    }

    if(bytes_read == 0) {
        printf("Client %s disconnected.\n", found_client->id);

        // Ștergem din clients și din pfds

        remove_pfd(&pfd);
        close(pfd.fd);
        pfd.fd = -1;
        // dam si unsubscribe???
        found_client->connected = false;
        found_client->fd = -1;
        return;
    }

    else {
        if(buff == NULL) {
        return;
        }
            // SUBSCRIBE sau UNSUBSCRIBE
            // (un)subscribe <TOPIC>
            client_request* request = (client_request*)buff;
            // printRequest(request);
            handle_client_request(request, found_client);
    }
}



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

    /* Curățare*/
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
        close(tcp_sock);
        exit(1);
    }

    // Facem sockeții reutilizabili
    int flag = 1;
    setsockopt(tcp_sock, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(int));

    flag = 1;
    setsockopt(udp_sock, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(int));

    // Dezactivăm Nagle pentru socket-ul TCP de listen
    flag = 1;
    setsockopt(tcp_sock, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(int));

    /* Setăm portul și IP-ul pentru TCP */
    tcp_addr.sin_family = AF_INET;
    tcp_addr.sin_port = htons(server_port);
    tcp_addr.sin_addr.s_addr = INADDR_ANY;

    /* Setăm portul și IP-ul pentru UDP */
    udp_addr.sin_family = AF_INET;
    udp_addr.sin_port = htons(server_port);
    udp_addr.sin_addr.s_addr = INADDR_ANY;

    /* Facem bind */
    if(bind(tcp_sock, (struct sockaddr*)&tcp_addr, sizeof(tcp_addr)) < 0) {
        cerr << "[SERV] Couldn't bind to the port\n";
        close(tcp_sock);
        close(udp_sock);
        exit(1);
    }
    if(bind(udp_sock, (struct sockaddr*)&udp_addr, sizeof(udp_addr)) < 0) {
        cerr << "[SERV] Couldn't bind to the port\n";
        close(tcp_sock);
        close(udp_sock);
        exit(1);
    }

    /* Facem listen pe socket-ul TCP pasiv*/

    if(listen(tcp_sock, INT8_MAX) < 0) {
        cerr << "Error while listening\n";
        close(tcp_sock);
        close(udp_sock);
        exit(1);
    }

    // De acum încolo, serverul poate da accept clienților pe socketul tcp_sock

    // Actualizăm lista de file descriptori pfds
    pfds.push_back({tcp_sock, POLLIN, 0});
    pfds.push_back({udp_sock, POLLIN, 0});
    pfds.push_back({0, POLLIN, 0}); // STDIN

    int rc; // pentru erori
    bool stop_server = false;

    while(!stop_server) {

        rc = poll(pfds.data(), pfds.size(), -1);
        if(rc < 0) {
            cerr << "Poll error\n";
            close(tcp_sock);
            close(udp_sock);
            exit(1);
        }

        // Vedem de pe ce conexiune trebuie să citim date noi
        // 4 cazuri: TCP, UDP, STDIN și SUBSCRIBER
        for(auto &pfd : pfds) 
            if(pfd.revents & POLLIN) {

                // Evaluăm file descriptor-ul pfd.fd

                if(pfd.fd == udp_sock)
                    UDP_connection(udp_sock, udp_addr);

                else if(pfd.fd == tcp_sock)
                    TCP_connection(tcp_sock);

                else if(pfd.fd == 0) {   
                    
                    if(STDIN_message() == true){
                        // oprim server-ul
                        stop_server = true;
                        break;
                    }
                }
                
                else {
                    //Subscriber (client tcp)
                    SUBSCRIBER_connection(pfd);
                    break;
                }
            }
		
        if(stop_server == true)
            break;
    }

    // close
    close(tcp_sock);
    close(udp_sock);
    return 0;
}