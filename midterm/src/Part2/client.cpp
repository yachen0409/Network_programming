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

int main(int argc, char *argv[]){
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

    //	While loop:
    char buf[4096];
    string userInput;
    int welcomebytesReceived = recv(sock, buf, 4096, 0);
    string welcomereceived(buf, 0, welcomebytesReceived);
    cout << welcomereceived;
    do{
        cout << "% ";
        getline(cin, userInput);
        int sendRes = send(sock, userInput.c_str(), userInput.size() + 1, 0);
        // cout << userInput.c_str() << endl;
        if (sendRes == -1){
            cout << "TCP Could not send to server! Whoops!\r\n";
            continue;
        }
        //Wait for response
        memset(buf, 0, 4096);
        int bytesReceived = recv(sock, buf, 4096, 0);
        //Dealing server response 
        if (bytesReceived == -1){
            cout << "There was an error getting response from server\r\n";
        }
        else{
            stringstream ss(buf);
            string instr;
            ss >> instr;
            cout << buf;
            if(instr == "Bye"){
                break;
            }
        }
    }while(true);

    //	Close the socket
    close(sock);

    return 0;
}