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
string welcome = "*****Welcome to Game 1A2B*****\n";
string gamerule = "1. Each question is a 4-digit secret number.\n2. After each guess, you will get a hint with the following information:\n2.1 The number of \"A\", which are digits in the guess that are in the correct position.\n2.2 The number of \"B\", which are digits in the guess that are in the answer but are in the wrong position.\nThe hint will be formatted as \"xAyB\".\n3. 5 chances for each question.\n";
map<string, string>password;
vector<string>email;
map<int, string>usr;
map<int, string>target;
map<int, int>times;
map<int, bool>win;
map<int, vector<int>>wordbank;
map<string, bool>status;

int main(int argc, char *argv[])
{
    int maxclient = 30;
    int opt = true; 
    vector<int>client_socket(maxclient, 0);
    // Create a socket
    int listening = socket(AF_INET, SOCK_STREAM, 0);
    if (listening == -1){
        cerr << "Can't create a TCP socket! Quitting" << endl;
        return -1;
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
    
    /* create UDP socket */
    int udpfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (udpfd == -1){
        cerr << "Can't create UDP a socket! Quitting" << endl;
        return -1;
    }
    bind(udpfd, (sockaddr*)&hint, sizeof(hint));
    cout << "UDP server is running\n";
    cout << "TCP server is running\n";
 
    //set of socket descriptors 
    fd_set readfds;  
    FD_ZERO(&readfds);

    // While loop: accept and echo message back to client
    char buf[4096];
    password["test"] = "0000";
    email.push_back("test@test.nycu.edu.tw");
    status["test"] = false;
    password["test2"] = "0000";
    email.push_back("test2@test.nycu.edu.tw");
    status["test2"] = false;
    int addrlen = sizeof(hint); 
    char buffer[1025]; 
    while(true){
        // set listening and udpfd in readset
        FD_SET(listening, &readfds);
        FD_SET(udpfd, &readfds);
        //add master socket to set
        int max_sd = max(listening, udpfd);
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
            const char *STRING_mod = welcome.c_str();
            send(clientSocket, STRING_mod, strlen(STRING_mod), 0);
            printf("New connection.\n");  
            for (int i = 0; i < maxclient; i++){  
                //if position is empty 
                if( client_socket[i] == 0 ){  
                    client_socket[i] = clientSocket;
                    usr[i] = "";  
                    break;  
                }  
            }  
        }  
        //else its some IO operation on some other socket
        for (int i = 0; i < maxclient; i++){  
            int sd = client_socket[i];  
            if (FD_ISSET( sd , &readfds)){  
                int sd = client_socket[i];
                FD_SET(sd, &readfds);
                // Wait for client to send data
                int bytesReceived = recv(client_socket[i], buf, 4096, 0);
                if (bytesReceived == -1){
                    cerr << "Error in recv(). Quitting" << endl;
                    break;
                }
                if (bytesReceived == 0){
                    cout << "Client Interrupted." << endl;
                    close( sd );  
                    client_socket[i] = 0;
                    status[usr[i]] = false;
                    // break;
                }
                if(bytesReceived > 0){
                    string received(buf, 0, bytesReceived);
                    stringstream rec(received);
                    string instr;
                    rec >> instr;
                    if( instr == "login"){
                        string user = "", pwd = "";
                        string returnmsg;
                        rec >> user >> pwd;
                        if(user == "" || pwd == ""){
                            returnmsg = "Usage: login <username> <password>\n";
                        }
                        else if(password.find(user) == password.end()){
                            returnmsg =  "Username not found.\n";
                        }
                        else{
                            if(password[user] != pwd){
                                returnmsg = "Password not correct.\n";
                            }
                            else{
                                if(status[user]){
                                    returnmsg = "Please logout first.\n";
                                }
                                else{
                                    returnmsg = "Welcome, " + user + ".\n";
                                    usr[i] = user;
                                    status[user] = true;
                                }
                            }
                        }
                        const char *STRING_mod = returnmsg.c_str();
                        send(client_socket[i], STRING_mod, strlen(STRING_mod), 0);
                    }
                    if(instr == "logout"){
                        string returnmsg;
                        if(!status[usr[i]]){
                            returnmsg = "Please login first.\n";
                        }
                        else{
                            returnmsg = "Bye, " +usr[i]+".\n";
                            status[usr[i]] = false;
                            usr[i] = "";
                        }
                        const char *STRING_mod = returnmsg.c_str();
                        send(client_socket[i], STRING_mod, strlen(STRING_mod), 0);
                    }
                    if(instr == "gaming"){
                        string guess;
                        rec >> guess;
                        string gamemsg = "";
                        if(guess == target[i]){
                            gamemsg = "win You got the answer!\n";
                            win[i] = true;
                            times[i] = 0;
                            target[i] = "";
                        }
                        else{
                            bool valid = true;
                            if(guess.size()!= 4){
                                valid = false;
                            }
                            else{
                                for(long long i = 0; i < guess.size(); ++i){
                                    if(!isdigit(guess[i])){
                                        valid = false;
                                    }
                                }
                            }
                            //invalid input
                            if(!valid){
                                gamemsg = "gaming Your guess should be a 4-digit number.\n";
                                // times[i]--;
                            }
                            else{
                                vector<bool>targetvalid(4, false);
                                // cout <<"before times: " <<times[i] << endl;
                                times[i]++;
                                // cout <<"times: " <<times[i] << endl;
                                int a = 0, b = 0;
                                for(int j = 0; j < guess.size(); ++j){
                                    if(guess[j] == target[i][j]){
                                        a++;
                                        guess[j] = ' ';
                                        targetvalid[j] = true;
                                        // guessa.push_back(guess[j]);

                                    }
                                }   
                                
                                for(int j = 0; j < guess.size(); ++j){
                                    for(int k = 0; k < target[i].size(); ++k){
                                        if(j == k || targetvalid[k]) continue;
                                        else if(guess[j] == target[i][k]){
                                            b++;
                                            targetvalid[k] = true;
                                            break;
                                        }
                                    }
                                }
                                
                                
                                stringstream chartostr;
                                string result;
                                chartostr << a << "a" << b << "b";
                                chartostr >> result;
                                gamemsg = "gaming lose " + result;
                                if(!win[i] && times[i]==5){
                                    // cout << "in lose" << endl;
                                    times[i] = 0;
                                    target[i] = "";
                                    gamemsg = gamemsg + " You lose the game!\n";
                                    // cout << gamemsg << endl;
                                }
                            }
                        }
                        const char *STRING_mod = gamemsg.c_str();
                        send(client_socket[i], STRING_mod, strlen(STRING_mod), 0);
                        // if(win[i]){
                        //     times[i] = 0;
                        //     target[i] = "";
                        // }
                    }
                    if(instr == "start-game"){
                        string startmsg;
                        bool qualify = true;
                        string num = "";
                        rec >> num;
                        if(!status[usr[i]]){
                            startmsg = "Please login first.\n";
                            qualify = false;
                        }
                        else if(num != ""){
                            if(num.size() != 4){
                                startmsg = "Usage: start-game <4-digit number>\n";
                                qualify = false;
                            }
                            else{
                                for(int j = 0; j < num.size(); ++j){
                                    if(!isdigit(num[j])){
                                        startmsg = "Usage: start-game <4-digit number>\n"; 
                                        qualify = false;
                                        break;
                                    }
                                }
                                
                            }
                            if(qualify) {
                                startmsg = "start Please typing a 4-digit number:";
                                target[i] = num;
                                times[i] = 0;
                                win[i] = false;
                            }
                        }
                        else if(num == ""){
                            stringstream inttostr;
                            string temp;
                            int number = rand() % 9000 + 1000;
                            inttostr << number;
                            inttostr >> target[i];
                            // cout << target[i] << endl;
                            startmsg = "start Please typing a 4-digit number:";
                            times[i] = 0;
                            win[i] = false;
                        }
                        for(int j = 0; j < target[i].size(); ++j){
                            wordbank[i].push_back(target[i][j]);
                        }
                        const char *game = startmsg.c_str();
                        send(client_socket[i], game, strlen(game), 0);
                    }
                    if (instr == "exit"){
                        cout << "tcp get msg: exit" << endl;
                        close( sd );  
                        client_socket[i] = 0;
                        status[usr[i]] = false;
                        // break;
                    }
                }
                memset(buf, 0, 4096);
            }  
            
        }  
        if(FD_ISSET(udpfd, &readfds)){ //udp
            sockaddr_in udpclient;
            socklen_t udpclientSize = sizeof(udpclient);
            int n = recvfrom(udpfd, buf, sizeof(buf), 0, (sockaddr*)&udpclient, &udpclientSize);
            string received(buf, 0, n);
            stringstream rec(received);
            string instr;
            rec >> instr;
            string returnmsg = "";
            if(instr == "register"){
                string username = "", mail = "", pwd = "";
                rec >> username >> mail >> pwd;
                if(username == "" || mail == ""||pwd == ""){
                    returnmsg = "Usage: register <username> <email> <password>\n";
                }
                else if(password.find(username) != password.end()){
                    returnmsg = "Username is already used.\n";
                }
                else if(find(email.begin(), email.end(), mail) != email.end()){
                    returnmsg = "Email is already used.\n";
                }
                else{
                    returnmsg = "Register successfully.\n";
                    password[username] = pwd;
                    email.push_back(mail);
                    status[username] = false;
                }
            }
            if (instr == "game-rule"){
                returnmsg = gamerule;
            } 
            const char * re = returnmsg.c_str();
            sendto(udpfd, re, strlen(re), 0, (sockaddr*)&udpclient, sizeof(udpclient));
        }
    }
    return 0;
}