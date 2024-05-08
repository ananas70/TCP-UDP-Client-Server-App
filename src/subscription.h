#ifndef SUBSCRIPTION_H
#define SUBSCRIPTION_H

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

#include "general_utils.h"

using namespace std;

bool match_topic(string pattern, string topic);
void subscribe(struct client_data* client, string wanted_topic, char type);

void unsubscribe_wildcard(struct client_data* client, string wanted_topic);





#endif  // SUBSCRIPTION_H