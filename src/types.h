#ifndef TYPES_H
#define TYPES_H

#include <stdint.h>
#include <vector>
#include <string>

#define BUF_LEN 1600
#define TOPIC_LEN 51    //e o naroada fix cu len = 50
#define CONTENT_LEN 1500
#define DATA_TYPE_LEN 10
#define ID_LEN 10
#define SUBSCRIBER_CMD_LEN 70

using namespace std;

struct udp_pckt {
    char topic[TOPIC_LEN];
    uint8_t data_type;
    char content[CONTENT_LEN];
};


struct client_notification {
    //<IP_CLIENT_UDP>:<PORT_CLIENT_UDP> - <TOPIC> - <TIP_DATE> - <VALOARE_MESAJ>
    char topic[TOPIC_LEN];
    char data_type[DATA_TYPE_LEN];
    char content[CONTENT_LEN];
};


struct client_data {
    char id[ID_LEN];
    bool connected;
    int fd;
    vector<string> subscriptions;
};

struct client_request {
    char type;
    char topic[TOPIC_LEN];
};


#endif  // types_h