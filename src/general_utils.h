#ifndef GENERAL_UTILS_H
#define GENERAL_UTILS_H

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

#include "subscription.h"

#define BUF_LEN 1600
#define TOPIC_LEN 50
#define CONTENT_LEN 1500
#define DATA_TYPE_LEN 11
#define ID_LEN 10
#define SUBSCRIBER_CMD_LEN 70

using namespace std;


// Pachet primit de către clienții UDP
struct udp_pckt {
    char topic[TOPIC_LEN];
    uint8_t data_type;
    char content[CONTENT_LEN];
};

// Notificare trimisă de server către client TCP
struct client_notification {
    char topic[TOPIC_LEN];
    char data_type[DATA_TYPE_LEN];
    char content[CONTENT_LEN];
};

// Informații despre un client
struct client_data {
    char id[ID_LEN];
    bool connected;
    int fd;
    vector<string> subscriptions;   // topic-urile la care e abonat
};

// Solicitare a clientului de (dez)abonare
struct client_request {
    char type;
    char topic[TOPIC_LEN];
};


void build_notification(client_notification* client_pckt, struct udp_pckt* message);
void send_notification_to_client(client_data client, char* topic, client_notification client_pckt);
void handle_client_request(struct client_request* request, struct client_data* client);



#endif  // GENERAL_UTILS_H