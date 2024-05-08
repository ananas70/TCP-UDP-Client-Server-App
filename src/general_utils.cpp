#include "general_utils.h"


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
            sprintf(client_pckt->content, "%f", x / pow(10, exp));
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
            cerr << "Unknown data type = ["<< message->data_type<<"] sent by the UDP client\n";
            break;
        }
    }
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


void handle_client_request(struct client_request* request, struct client_data* client) {

    string wanted_topic = string(request->topic);

    // Vedem dacă are wildcard sau nu
    if(wanted_topic.find_first_of("*+") != string::npos && request->type == 'u')
        unsubscribe_wildcard(client, wanted_topic);
    else 
        subscribe(client, wanted_topic, request->type);

}