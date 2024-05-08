    README - TEMA 2 - Aplicație client-server TCP și UDP pentru gestionarea mesajelor
    Curs : Protocoale de Comunicație
    Studentă: Stoica Ana-Florina, grupa 325 CB

    Structura fișierelor:

    Pentru a rezolva această temă, am folosit următoare structură de fișiere:

    - server.cpp și subscriber.cpp : implementează server-ul și clientul TCP.
    - general_utils.h - conține structurile ce vor încadra mesajele transmise între server și clienți, precum și
    diverse funcții utile pentru comunicarea cu aceștia.
    - subscription.h - conține funcții ajutătoare pentru realizarea operațiilor ce țin de abonare/dezabonare la topic-uri.

    Descrierea funcționalității:
    
    SERVER:

    - Pentru realizarea comunicării cu clienții UDP, am folosit structura udp_pckt pentru pachetele ce urmează sa fie 
    recepționate de server. Am implementat comunicarea propriu-zisă folosind UDP sockets API - ul prezentat în laboratorul
    5, iar recepționarea și transmiterea mai departe a notificărilor către clienții potriviți (cei abonați la topic - urile
    potrivite) este suportată de funcțiile UDP_connection (inițiază procesul de comunicare cu clientul UDP), build_notification
    (construiește pachetul client_notification ce urmează a fi trimis către clienții abonați la topic) și send_notification_to_client
    (verifică subscripțiile clientului - incluzând și cazurile cu wildcards - și trimite efectiv pachetul dacă clientul este 
    un destinatar potrivit - are abonament la topic - ul respectiv).
     - Pentru inițierea comunicării cu clienții TCP, am pornit de la implementarea laboratorului 7. Funcția TCP_connection inițiază
    comunicarea cu clienții TCP (subscriber-ii), modificând corespunzător structurile de date menite să stocheze clienții
    (vector <struct client_data> clients) și descriptorii de fișier (vector <struct pollfd> pfds). Structurile sunt alocate dinamic 
    folosind STL pentru eficiență. 
    - Pentru parsarea comenzilor venite dinspre clienții TCP, am implementat funcția SUBSCRIBER_connection, care tratează toate cele
    trei comenzi posibile ce pot veni din partea unui client TCP. Pentru comenzile de subscribe/unsubscribe, folosim funcția
    handle_client_request.
    - Pentru comenzile venite de la intrarea standard (STDIN), există funcția STDIN_message. În cazul comenzii de exit, funcția
    returnează true și se iese corespunzător din bucla infinită din main pentru a se închide server-ul.

    CLIENT:

    - Clientul recepționează pachete de la server prin intermediul funcției parse_server_msg. El primește pachete de tipul client_notification
    și le afișează corespunzător.
    - Clientul interpretează comenzile venite de la intrarea standard cu ajutorul funcției STDIN_message. Pentru comenzile de subscribe/unsubscribe
    care necesită mai mult calcul, este implementară funcția submit_request ce construiește prin build_request pachetul de tip client_request ce urmează
    a fi trimis către server.
