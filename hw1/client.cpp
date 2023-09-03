#include <iostream>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <string>
#include <sstream>

using namespace std;

int main(int argc, char *argv[])
{
    //	Create a TCP socket
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1){
        cout << "TCP socket failed\n";
        return 1;
    }
    sockaddr_in hint;
    hint.sin_family = AF_INET;
    hint.sin_port = htons(atoi(argv[2]));
    inet_pton(AF_INET, argv[1], &hint.sin_addr);
    //	Connect to the server on the socket
    int connectRes = connect(sock, (sockaddr*)&hint, sizeof(hint));
    if (connectRes == -1){
        return 1;
    }
    //create an UDP socket
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd == -1){
        cout << "UDP socket failed\n";
        return 1;
    }
    //	Connect to the server on the socket
    int udpconnectRes = connect(sockfd, (sockaddr*)&hint, sizeof(hint));
    if (udpconnectRes == -1){
        return 1;
    }
    //	While loop:
    char buf[4096];
    bool in_game = false;
    int times = 0;
    bool win = false;
    string userInput;
    
    int welcomebytesReceived = recv(sock, buf, 4096, 0);
    string welcomereceived(buf, 0, welcomebytesReceived);
    cout << welcomereceived;
    do {
        //Enter lines of text
        // cout << "> ";
        //user input
        getline(cin, userInput);
        //seperate first userinput keyword
        stringstream linetoinstr(userInput);
        string kind;
        linetoinstr >> kind;
        int bytesReceived;
        if(kind == "register" || kind == "game-rule"){//udp
            int sendRes = sendto(sockfd, userInput.c_str(), userInput.size() + 1, 0, (sockaddr*)&hint, sizeof(hint));
            if (sendRes == -1){
                cout << "UDP Could not send to server! Whoops!\r\n";
                continue;
            }
            memset(buf, 0, 4096);
            socklen_t len = sizeof(hint);
            bytesReceived = recvfrom(sockfd, buf, 4096, 0, (sockaddr*)&hint, &len);
        }
        else if(kind == "login" || kind == "logout" || kind == "start-game" || kind == "exit" || in_game){//tcp
            if(in_game){
                userInput = "gaming "+userInput;
            }
            int sendRes = send(sock, userInput.c_str(), userInput.size() + 1, 0);
            if (sendRes == -1){
                cout << "TCP Could not send to server! Whoops!\r\n";
                continue;
            }
            if(kind == "exit") break;
            //Wait for response
            memset(buf, 0, 4096);
            bytesReceived = recv(sock, buf, 4096, 0);
        }
        //Dealing server response 
        if (bytesReceived == -1){
            cout << "There was an error getting response from server\r\n";
        }
        else{
            //Display response
            string received(buf, 0, bytesReceived);
            stringstream rec(received);
            string instr;
            rec >> instr;
            if(instr == "start"){
                string temp;
                while(rec >> temp){
                    cout << temp << " ";
                }
                memset(buf, 0, 4096);
                cout << endl;
                in_game = true;
            }
            else if(instr == "win"){
                string winmsg;
                while(rec >> winmsg){
                    cout << winmsg << " ";
                }
                cout << endl;
                times = 0;
                in_game = false;
            }
            else if(instr == "gaming"){
                string gameInput;
                string ins;
                rec >> ins;
                if(ins == "Your"){
                    cout << ins << " ";
                    string errormsg;
                    while(rec >> errormsg){
                        cout << errormsg << " ";
                    }
                    cout << endl;
                }
                
                else if(ins == "lose"){
                    times++;
                    string result;
                    rec >> result;
                    cout << result << endl;
                }
                if(times == 5) {
                    // cout << "in lose! " << received << endl;
                    times = 0;
                    in_game = false;
                    string end;
                    while(rec >> end){
                        cout << end << " ";
                    }
                    cout << endl;
                }
            }
            else{
                cout << string(buf, bytesReceived);
            }
        }
    } while(true);
    //	Close the socket
    close(sock);
    return 0;
}