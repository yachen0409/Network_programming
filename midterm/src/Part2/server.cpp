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
#include <string>
using namespace std;

vector<int>user(0, 0);
map<int, bool>user_status;
map<int, int>client_to_usernum;
map<int, string> ipportinfo;


int main(int argc, char *argv[]){
    int maxclient = 50;
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
            
            string welcome = "Welcome, you are user";
            // getsockname (clientSocket,  (struct sockaddr *) &client, &clientSize );
            cout << "New connection from "<< inet_ntoa(client.sin_addr) << ":"<<ntohs(client.sin_port);
            string ip_port = string(inet_ntoa(client.sin_addr))+":" + to_string(ntohs(client.sin_port));
            for (int i = 0; i < maxclient; i++){  
                //if position is empty 
                if( client_socket[i] == 0 ){  //init
                    client_socket[i] = clientSocket;
                    // user.push_back(user.size());
                    user_status[i+1] = true;
                    ipportinfo[i+1] = ip_port;
                    // client_to_usernum[i] = user.size();
                    cout << " user"<< i+1 << endl;
                    welcome = welcome + to_string(i+1) + "\n"; 
                    const char *STRING_mod = welcome.c_str();
                    send(clientSocket, STRING_mod, strlen(STRING_mod), 0);
                    break;  
                }  
            }  
        }  
        //else its some IO operation on some other socket
        for (int i = 0; i < maxclient; i++){  
            int sd = client_socket[i];  
            if (FD_ISSET( sd , &readfds)){ 
                // cout << "here!!!!" << endl; 
                int sd = client_socket[i];
                FD_SET(sd, &readfds);
                
                // Wait for client to send data
                memset(buf, 0, 4096);
                int bytesReceived = recv(client_socket[i], buf, 4096, 0);
                if (bytesReceived == -1){
                    cerr << "Error in recv(). Quitting" << endl;
                    break;
                }
                if (bytesReceived == 0){
                    cout << "user"<<i+1<<" "<<ipportinfo[i+1] << " disconnected" << endl;
                    close( sd );  
                    client_socket[i] = -1;
                    user_status[i+1] = false;
                    // break;
                }
                else{
                    string returnmsg ;
                    // cout << "Client " << i+1 <<": " << buf << endl;
                    if(string(buf) == "list-users"){
                        for(auto it = user_status.begin(); it != user_status.end(); ++it){
                            if(it->second == true){
                                if(returnmsg == ""){
                                    returnmsg += "user" + to_string(it->first);
                                }
                                else{
                                    returnmsg += "\nuser" + to_string(it->first);
                                }
                                
                            }
                        }
                        returnmsg += "\n";
                    }
                    else if(string(buf) == "get-ip"){
                        returnmsg = "IP: " + ipportinfo[i+1] + "\n";
                    }
                    else if(string(buf) == "exit"){
                        returnmsg = "Bye user" + to_string(i+1) + ".\n";
                    }
                    else{
                        returnmsg = "COMMAND NOT AVAILABLE!\n";
                    }
                    // cout << "returnmsg = " << returnmsg.c_str() << endl;
                    const char *STRING_mod = returnmsg.c_str();
                    send(client_socket[i], STRING_mod, strlen(STRING_mod), 0);
                }
                
            }  
        }
    }
    return 0;
}