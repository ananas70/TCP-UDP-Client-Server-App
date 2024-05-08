#include <iostream>
#include <vector>
#include <string>
#include <sstream>

using namespace std;

bool match(const string& pattern, const string& topic) {
    stringstream pattern_stream(pattern);
    stringstream topic_stream(topic);

    string pattern_word, topic_word;
    while (getline(pattern_stream, pattern_word, '/')) {
        if (!getline(topic_stream, topic_word, '/')) {
            // If the topic has fewer parts than the pattern, it doesn't match
            return false;
        }
        if (pattern_word != "*" && pattern_word != "+" && pattern_word != topic_word) {
            // If parts don't match and the pattern part is not a wildcard, return false
            return false;
        }

        if (pattern_word == "+") {
            // If pattern part is '+', skip to the next part in both pattern and topic
            continue;
        }

        if (pattern_word == "*") {
            // If pattern part is '*', special algorithm
            if(!getline(pattern_stream, pattern_word, '/'))
                //daca steluta e ultimul cuvant din pattern
                return true;

            //acum, pattern_word are stocat in el primul cuvant de dupa steluta
            bool found_next = false;
            //iteram in topic pana dam de pattern_word
            while (getline(topic_stream, topic_word, '/')) {
                if(topic_word == pattern_word) {
                    //reset
                    found_next = true;
                    break;                
                }
            }

            if(!found_next)
                return false;
        }
    }

        if(getline(topic_stream, topic_word, '/'))
        return false;
    return true;
}

vector<string> filterTopics(const vector<string>& allTopics, const string& pattern) {
    vector<string> filteredTopics;
    
    for (const string& topic : allTopics) {
        if (match(pattern, topic)) {
            filteredTopics.push_back(topic);
        }
    }
    
    return filteredTopics;
}

int main() {
    vector<string> allTopics = {"upb/precis/100/schedule/monday/8","upb/ec/100/pressure", "upb/precis/100/humidity","upb/ec/100/schedule/tuesday/12" };
    string pattern = "upb/+/100/+";
    
    vector<string> filteredTopics = filterTopics(allTopics, pattern);
    
    cout << "Filtered topics:" << endl;
    for (const string& topic : filteredTopics) {
        cout << topic << endl;
    }
    
    return 0;
}
