#include <iostream>
#include <sys/types.h>
#include <algorithm>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
using namespace std;
map<int, bool>usr_online;
map<int, bool>usr_mute;
vector<string>ip_ports;
//----------------------------HW2------------------------------------
// user info
// map<string, string>password;
// map<string, string>email;
// map<int, string>usr;
// map<string, bool>status;
// map<int, string>people_gameroom;
// map<string, vector<int>>room_people;
// // game related
// map<string, string>target;
// map<string, int>times;
// map<string, vector<int>>wordbank;
// map<string, bool>room_startornot;
// map<string, int>player_seq, max_player_index, cur_player_id;
// //game room related
// map<int, string>manager_room;
// map<string, int>id_permission; //(0 for public, 1 for private)
// map<string, int>room_code;
// map<int, vector<int>>invitee_inviter;
// int maxclient = 30;
// int opt = true;
//----------------------------HW2------------------------------------


int main(int argc, char *argv[]){
    int maxclient = 30;
    int opt = true;
    int max_sd;
    vector<int>client_socket(maxclient, 0);
    // Create a socket
    int listening = socket(AF_INET, SOCK_STREAM, 0);
    if (listening == -1){
        cerr << "Can't create a TCP socket! Quitting" << endl;
        return -1;
    }

    //initialise all client_socket[] to 0 so not checked
    for (int i = 0; i < maxclient; i++){
        client_socket[i] = 0;
    }
    //set master socket to allow multiple connections ,
    //this is just a good habit, it will work without this
    if( setsockopt(listening, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0 ){
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    // Bind the ip address and port to a socket
    sockaddr_in hint;
    hint.sin_family = AF_INET;
    hint.sin_port = htons(atoi(argv[1]));
    hint.sin_addr.s_addr = htonl(INADDR_ANY);
    bind(listening, (sockaddr*)&hint, sizeof(hint));
    listen(listening, SOMAXCONN);
    ip_ports.resize(maxclient);
    //set of socket descriptors
    fd_set readfds;
    FD_ZERO(&readfds);

    char buf[4096];
    while(true){
        //clear the socket set
        FD_ZERO(&readfds);
        // set listening in readset
        FD_SET(listening, &readfds);
        max_sd = listening;
        //add child sockets to set
        for ( int i = 0 ; i < maxclient ; i++){
            //socket descriptor
            int sd = client_socket[i];
            //if valid socket descriptor then add to read list
            if(sd > 0)  FD_SET( sd , &readfds);
            //highest file descriptor number, need it for the select function
            if(sd > max_sd)  max_sd = sd;
        }
        //wait for an activity on one of the sockets , timeout is NULL ,
        //so wait indefinitely
        int activity = select( max_sd + 1 , &readfds , NULL , NULL , NULL);
        if ((activity < 0) && (errno!=EINTR)){
            printf("select error");
            cout << errno << endl;
        }
        //If something happened on the master socket ,
        //then its an incoming connection
        if (FD_ISSET(listening, &readfds)){
            sockaddr_in client;
            socklen_t clientSize = sizeof(client);
            int clientSocket = accept(listening, (sockaddr*)&client, &clientSize);

            string welcome = "**************************\n*Welcome to the BBS server.*\n**************************\nWelcome, user";
            // getsockname (clientSocket,  (struct sockaddr *) &client, &clientSize );
            cout << "New connection from "<< inet_ntoa(client.sin_addr) << ":"<<ntohs(client.sin_port);
            string ip_port = string(inet_ntoa(client.sin_addr))+":" + to_string(ntohs(client.sin_port));
            for (int i = 0; i < maxclient; i++){
                //if position is empty
                if( client_socket[i] == 0 ){  //init
                    client_socket[i] = clientSocket;
                    welcome += to_string(i)+".\n";
                    cout << " user"<< i << endl;
                    // Init
                    usr_mute[i] = false;
                    ip_ports[i] = ip_port;
                    usr_online[i] = true;
                    // welcome = welcome + to_string(i) + "\n";
                    const char *STRING_mod = welcome.c_str();
                    send(clientSocket, STRING_mod, strlen(STRING_mod), 0);
                    break;
                }
            }
        }
        //else its some IO operation on some other socket
        for(int i = 0; i < maxclient; i++){
            int sd = client_socket[i];
            if (FD_ISSET(sd , &readfds)){
                int sd = client_socket[i];
                FD_SET(sd, &readfds);
                // Wait for client to send data
                int bytesReceived = read(client_socket[i], buf, 4096);
                if(bytesReceived == -1){
                    cerr << "Error in recv(). Quitting" << endl;
                    break;
                }
                if(bytesReceived == 0){
                    cout << "Client Interrupted" << endl;
                    close( sd );
                    client_socket[i] = -1;
                    string temp = "user"+ to_string(i) + " " + ip_ports[i] + " disconnected";
                    cout << temp << endl ;
                    usr_online[i] = false;
                    usr_mute.erase(i);
                    FD_ZERO(&readfds);
                    // break;
                }
                if(bytesReceived > 0){
                    string received(buf, 0, bytesReceived);
                    stringstream rec(received);
                    string instr;
                    rec >> instr;
                    cout << instr << endl;
                    if(instr == "mute"){
                        string returnmsg;
                        if(usr_mute[i]){
                            returnmsg = "You are already in mute mode.";
                        }
                        else if(!usr_mute[i]){
                            usr_mute[i] = true;
                            returnmsg = "Mute mode.";
                        }
                        const char *STRING_mod = returnmsg.c_str();
                        send(client_socket[i], STRING_mod, strlen(STRING_mod), 0);
                    }
                    else if(instr == "unmute"){
                        string returnmsg;
                        if(!usr_mute[i]){
                            returnmsg = "You are already in unmute mode.";
                        }
                        else if(usr_mute[i]){
                            usr_mute[i] = false;
                            returnmsg = "Unmute mode.";
                        }
                        const char *STRING_mod = returnmsg.c_str();
                        send(client_socket[i], STRING_mod, strlen(STRING_mod), 0);
                    }
                    else if(instr == "yell"){
                        string temp, msg = "";
                        rec >> msg;
                        while(rec >> temp){
                            msg += (" " + temp);
                        }
                        string broadcastmsg = "user" + to_string(i) + ": " + msg;
                        cout << broadcastmsg << endl;
                        for(auto it = usr_mute.begin(); it != usr_mute.end(); ++it){
                            if(it->first != i && !it->second){
                                const char *STRING_mod2 = broadcastmsg.c_str();
                                send(client_socket[it->first], STRING_mod2, strlen(STRING_mod2), 0);
                            }
                        }
                    }
                    else if(instr == "tell"){
                        string cur_user = "user"+to_string(i);
                        int send_to_usr;
                        
                        string send_to, temp, msg = "", returnmsg;
                        rec >> send_to;
                        stringstream tempss(send_to.substr(4));
                        tempss >> send_to_usr;
                        rec >> msg;
                        while(rec >> temp){
                            msg += (" " + temp);
                        }
                        cout << send_to_usr << usr_online[send_to_usr] << endl;
                        if(!usr_mute[send_to_usr] && usr_online[send_to_usr]){
                            if(send_to_usr == i){
                                returnmsg = "You cannot send private msg to yourself!";
                            }
                            else{
                                // cout << "im here!\n";
                                returnmsg = cur_user + " told you: " + msg;
                            }
                            const char *STRING_mod = returnmsg.c_str();
                            send(client_socket[send_to_usr], STRING_mod, strlen(STRING_mod), 0);
                        }
                        else if(usr_mute[send_to_usr]){
                            //pass
                        }
                        else{
                            // cout << "so sad!\n";
                            returnmsg = send_to + " does not exist.";
                            const char *STRING_mod = returnmsg.c_str();
                            send(client_socket[i], STRING_mod, strlen(STRING_mod), 0);
                        }
                    }
                    else if(instr == "exit"){
                        close( sd );
                        client_socket[i] = -1;
                        string temp = "user"+ to_string(i) + " " + ip_ports[i] + " disconnected";
                        cout << temp << endl ;
                        usr_online[i] = false;
                        usr_mute.erase(i);
                        FD_ZERO(&readfds);
                    }
                    else{
                        string returnmsg;
                        returnmsg = "No such command!";
                        const char *STRING_mod = returnmsg.c_str();
                        send(client_socket[i], STRING_mod, strlen(STRING_mod), 0);
                    }
                    //-------------------------------HW2--------------------------------------------
                }
                //-------------------------------HW2--------------------------------------------
                memset(buf, 0, 4096);
            }
        }
    }
    return 0;
}