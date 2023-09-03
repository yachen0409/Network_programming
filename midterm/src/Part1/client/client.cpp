#include <iostream>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <string>
#include <fstream>
#include <sstream>
using namespace std;
int main(int argc, char *argv[]){
    //create an UDP socket
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd == -1){
        cout << "UDP socket failed\n";
        return 1;
    }

    sockaddr_in hint;
    hint.sin_family = AF_INET;
    hint.sin_port = htons(atoi(argv[2]));
    inet_pton(AF_INET, argv[1], &hint.sin_addr);
    //	Connect to the server on the socket
    int udpconnectRes = connect(sockfd, (sockaddr*)&hint, sizeof(hint));
    if (udpconnectRes == -1){
        return 1;
    }
    //	While loop:
    char buf[4096];
    string userInput;

    do{
        cout << "% ";
        getline(cin, userInput);
        int sendRes = sendto(sockfd, userInput.c_str(), userInput.size() + 1, 0, (sockaddr*)&hint, sizeof(hint));
        if (sendRes == -1){
            cout << "UDP Could not send to server! Whoops!\r\n";
            continue;
        }
        memset(buf, 0, 4096);
        socklen_t len = sizeof(hint);
        // cout << "waiting....."<<endl;
        int bytesReceived = recvfrom(sockfd, buf, 4096, 0, (sockaddr*)&hint, &len);
        // cout << "hi" << endl;
        // cout << bytesReceived << endl;
        stringstream ss(buf);
        string instr;
        ss >> instr;
        // 
        if(instr == "newfilestart"){
            string filename;
            while(ss >> filename){
                ofstream outfile (filename);
                string line;
                ss.ignore();
                bool first = true;
                while( getline(ss, line) && line != "newfileend"){
                    // cout << "line = " << line << endl;
                    if(first){
                        outfile << line;
                        first = false;
                    }
                    else{
                        outfile << endl << line ;
                    }
                    
                }
                outfile.close();
            }
        }
        else if(instr == "exit"){
            break;
        }
        else{
            cout << buf << endl;
        }
    }while(true);
    close(sockfd);
    return 0;
}