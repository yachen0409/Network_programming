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
#include <filesystem>
namespace fs = std::filesystem;
using namespace std;
int main(int argc, char *argv[]){
    /* create UDP socket */
    int udpfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (udpfd == -1){
        cerr << "Can't create UDP a socket! Quitting" << endl;
        return -1;
    }
    // Bind the ip address and port to a socket
    sockaddr_in hint;
    hint.sin_family = AF_INET;
    hint.sin_port = htons(atoi(argv[1]));
    hint.sin_addr.s_addr = htonl(INADDR_ANY);

    bind(udpfd, (sockaddr*)&hint, sizeof(hint));
    cout << "UDP server is running\n";
    // While loop: accept and echo message back to client
    char buf[4096];
    while(true){
        sockaddr_in udpclient;
        socklen_t udpclientSize = sizeof(udpclient);
        int n = recvfrom(udpfd, buf, sizeof(buf), 0, (sockaddr*)&udpclient, &udpclientSize);
        stringstream ss(buf);
        string instr;
        ss >> instr;

        string returnmsg;
        // cout <<"Client: " << buf << endl;
        if(instr == "get-file-list"){
            returnmsg = "File:";
            string path = fs::current_path();
            for (const auto & entry : fs::directory_iterator(path)){
                returnmsg += " " + string(entry.path().filename());
            }
            // cout << returnmsg << endl;
        }
        else if(instr == "get-file"){
            string cur_file;
            returnmsg += "newfilestart "; 
            while(ss >> cur_file){
                string line;
                ifstream myfile (cur_file);
                if (myfile.is_open()){   
                    returnmsg += (cur_file + "\n");
                    while (getline(myfile,line)){
                        returnmsg += line + "\n";
                    }
                    returnmsg += "newfileend\n";
                    myfile.close();
                }
                else{
                    returnmsg = "Invalid filename input!";
                    break;
                }
            }
            
            
        }
        else if(instr == "exit"){
            returnmsg = "exit";
        }
        else{
            returnmsg = "COMMAND NOT AVAILABLE!";
        }
        // cout << "here" << endl;
        // cout << returnmsg << endl;
        const char *re = returnmsg.c_str();
        // cout << re << endl;
        sendto(udpfd, re, strlen(re), 0, (sockaddr*)&udpclient, udpclientSize);
        // cout << "sent" << endl;
    }
    return 0;
}