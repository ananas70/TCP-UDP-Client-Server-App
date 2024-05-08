#include "subscription.h"

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
