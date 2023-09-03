#include <iostream>
#include <signal.h>
#include <stdlib.h>
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
// user info
map<string, string>password;
map<string, string>email;
map<int, string>usr;
map<string, bool>status;
map<int, string>people_gameroom;
map<string, vector<int>>room_people;
// game related
map<string, string>target;
map<string, int>times;
map<string, vector<int>>wordbank;
map<string, bool>room_startornot;
map<string, int>player_seq, max_player_index, cur_player_id;
//game room related
map<int, string>manager_room;
map<string, int>id_permission; //(0 for public, 1 for private)
map<string, int>room_code;
map<int, vector<int>>invitee_inviter;
int maxclient = 30;
int opt = true; 


void my_handler(int s){
    printf("Caught signal %d\n",s);
    int status1, status2, status3;
    ifstream readfile("/home/ec2-user/efs/server_status");
    readfile >> status1 >> status2 >> status3;
    readfile.close();
    status1 = 0;
    string writestatus = to_string(status1) + " " + to_string(status2) + " "+to_string(status3);
    ofstream wfile("/home/ec2-user/efs/server_status");
    wfile << writestatus;
    wfile.close();
    exit(1); 
}
int main(int argc, char *argv[]){
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
    hint.sin_port = htons(8888);
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
    signal (SIGINT,my_handler);
    int status1, status2, status3;
    ifstream readfile("/home/ec2-user/efs/server_status");
    readfile >> status1 >> status2 >> status3;
    readfile.close();
    if(status1 == 0 || status2 == 0 || status3 == 0){
        ofstream writefile("/home/ec2-user/efs/record");
	string resetrecord = "0 0 0";
	writefile << resetrecord;
	writefile.close();
    }
    status1 = 1;
    string writetostatus = to_string(status1) + " " + to_string(status2) + " "+to_string(status3);
    ofstream writefile("/home/ec2-user/efs/server_status");
    writefile << writetostatus;
    writefile.close();

    // While loop: accept and echo message back to client
    char buf[4096];
    int addrlen = sizeof(hint);
    char buffer[1025];
    while(true){
        // cout << "alwayshere~"<<endl;
        // set listening and udpfd in readset
        FD_SET(listening, &readfds);
        FD_SET(udpfd, &readfds);
        //add master socket to set
        int max_sd = max(listening, udpfd);
        //add child sockets to set 

        for(int i = 0 ; i < maxclient ; i++){
            // cout << maxclient << " "; 
            //socket descriptor 
            int sd = client_socket[i];  
            //if valid socket descriptor then add to read list 
            if(sd > 0)  FD_SET( sd , &readfds);     
            //highest file descriptor number, need it for the select function 
            if(sd > max_sd)  max_sd = sd;  
        }  
        //wait for an activity on one of the sockets , timeout is NULL , 
        //so wait indefinitely 
        // cout << "before select";
        int activity = select( max_sd+1 , &readfds , NULL , NULL , NULL);  
        // cout << "after select! "<<activity << endl;
        if((activity < 0) && (errno!=EINTR)){  
            cout << activity << endl;
            printf("select error ");  
            cout << errno << endl;

        }  
        // cout << "selected!"<<endl;
        //If something happened on the master socket , 
        //then its an incoming connection 
        if(FD_ISSET(listening, &readfds)){  
            // cout << "listene<d..."<<endl;
            sockaddr_in client;
            socklen_t clientSize = sizeof(client);
            int clientSocket = accept(listening, (sockaddr*)&client, &clientSize);
            printf("New connection.\n");  
            for(int i = 0; i < maxclient; i++){  
                //if position is empty 
                if(client_socket[i] == 0){  
                    client_socket[i] = clientSocket;
                    usr[i] = "";  
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
                    client_socket[i] = 0;
                    if(status[usr[i]] == true){
                        int server1, server2, server3;
                        ifstream readfile("/home/ec2-user/efs/record");
                        readfile >> server1 >> server2 >> server3;
                        readfile.close();
			server1 -= 1;
                        string writetorecord = to_string(server1) + " " + to_string(server2) + " "+to_string(server3);
                        ofstream writefile("/home/ec2-user/efs/record");
                        writefile << writetorecord;
                        writefile.close();
                    }
                    status[usr[i]] = false;
                    usr[i] = "";
                    string roomid = people_gameroom[i];   
                    people_gameroom.erase(i);
                    vector<vector<int>::iterator>remove_it;
                    for(auto it = room_people[roomid].begin(); it != room_people[roomid].end(); ++it){
                        if(*it == i){
                            remove_it.push_back(it);
                            //break;
                        }
                    }
                    for(auto it : remove_it){
                        room_people[roomid].erase(it);
                    }
                    // game is going on -> stop game
                    if(room_startornot[roomid]){
                        //game related
                        target.erase(roomid);
                        times.erase(roomid);
                        wordbank.erase(roomid);
                        room_startornot[roomid] = false;
                        player_seq.erase(roomid);
                        max_player_index.erase(roomid);
                        cur_player_id.erase(roomid);
                    }
                    // you are room manager -> destory room
                    if(manager_room[i] != ""){
                        if(id_permission[roomid] == 1){ 
                            room_code.erase(roomid); 
                        }
                        id_permission.erase(roomid);
                        manager_room.erase(i);
                        room_startornot.erase(roomid);
                        //! not sure it is valid or not
                        vector<pair<int, int>>remove;
                        for(auto it = invitee_inviter.begin(); it != invitee_inviter.end(); ++it){
                            int invitee_id = it->first;
                            for(int j = 0; j < it->second.size(); ++j){
                                bool same = (it->second[j] == i);
                                if(same){
                                    // it->second.erase(it2);
                                    remove.push_back(make_pair(invitee_id, j));
                                }
                            }
                        }
                        for(int j = 0; j < remove.size(); ++j){
                            invitee_inviter[remove[j].first].erase(invitee_inviter[remove[j].first].begin() + remove[j].second);
                        }

                        room_people.erase(roomid);
                        vector<map<int, string>::iterator>remove_it2;
                        for(auto it = people_gameroom.begin(); it != people_gameroom.end(); ++it){
                            bool same =(it->second == roomid);
                            if(same){
                                remove_it2.push_back(it);
                            }
                        }
                        for(auto it : remove_it2){
                            people_gameroom.erase(it);
                        }
                    }
                    FD_ZERO(&readfds);
                    // break;
                }
                if(bytesReceived > 0){
                    string received(buf, 0, bytesReceived);
                    stringstream rec(received);
                    string instr;
                    rec >> instr;
                    
                    if(instr == "status"){
                        string returnmsg;
                        int server1, server2, server3;
                        ifstream readfile("/home/ec2-user/efs/record");
                        readfile >> server1 >> server2 >> server3;
                        returnmsg = "Server1: " + to_string(server1) + "\nServer2: "+ to_string(server2) + "\nServer3: "+ to_string(server3) + "\n";
                        readfile.close();
                    	const char *STRING_mod = returnmsg.c_str();
                        send(client_socket[i], STRING_mod, strlen(STRING_mod), 0);
		    }
                    // login <username> <password>
                    if(instr == "login"){ 
                        string user = "", pwd = "", returnmsg = "";
                        rec >> user >> pwd;
                        if(user == "" || pwd == ""){
                            returnmsg = "Usage: login <username> <password>\n";
                        }
                        // Fail(1)
                        else if(password.find(user) == password.end()){ 
                            returnmsg =  "Username does not exist\n";
                        }

                        // Fail(2)
                        else if(usr[i] != ""){ 
                            returnmsg = "You already logged in as " + usr[i] + "\n";
                        }
                        // Fail(3)
                        else if(status[user]){ 
                            returnmsg = "Someone already logged in as " + user + "\n";
                        }
                        // Fail(4)
                        else if(password[user] != pwd){ 
                            returnmsg = "Wrong password\n";
                        }
                        // Success
                        else{
                            returnmsg = "Welcome, " + user + "\n";
                            usr[i] = user;
                            status[user] = true;
                            int server1, server2, server3;
                            ifstream readfile("/home/ec2-user/efs/record");
                            readfile >> server1 >> server2 >> server3;
                            readfile.close();
			    server1 += 1;
                            string writetorecord = to_string(server1) + " " + to_string(server2) + " "+to_string(server3);
                            ofstream writefile("/home/ec2-user/efs/record");
                            writefile << writetorecord;
                            writefile.close();
                            
                        }

                        const char *STRING_mod = returnmsg.c_str();
                        send(client_socket[i], STRING_mod, strlen(STRING_mod), 0);
                    }
                    // logout
                    if(instr == "logout"){
                        string returnmsg;
                        // Fail(1)
                        if(!status[usr[i]]){
                            returnmsg = "You are not logged in\n";
                        }
                        // Fail(2)
                        else if(people_gameroom[i] != ""){
                            returnmsg = "You are already in game room " + people_gameroom[i] + ", please leave game room\n";
                        }
                        // Success
                        else{
                            returnmsg = "Goodbye, " +usr[i]+"\n";
                            status[usr[i]] = false;
                            usr[i] = "";
                            int server1, server2, server3;
                            ifstream readfile("/home/ec2-user/efs/record");
                            readfile >> server1 >> server2 >> server3;
                            readfile.close();
			    server1 -= 1;
                            string writetorecord = to_string(server1) + " " + to_string(server2) + " "+to_string(server3);
                            ofstream writefile("/home/ec2-user/efs/record");
                            writefile << writetorecord;
                            writefile.close();
                        }
                        const char *STRING_mod = returnmsg.c_str();
                        send(client_socket[i], STRING_mod, strlen(STRING_mod), 0);
                    }
                    // create public room <game room id>
                    // create private room <game_room_id> <invitation code>
                    if(instr == "create"){
                        string permission, useless, returnmsg;
                        string roomid = "";
                        rec >> permission >> useless >> roomid;
                        if(roomid == ""){
                            returnmsg = "Usage: create public room <game room id>\n";
                        }
                        // Fail(1)
                        else if(!status[usr[i]]){ 
                            returnmsg = "You are not logged in\n";
                        }
                        // Fail(2)
                        else if(people_gameroom[i] != ""){
                            returnmsg = "You are already in game room " + people_gameroom[i] + ", please leave game room\n";
                        }
                        // Fail(3)
                        else if(id_permission.find(roomid) != id_permission.end()){ // Fail(3)
                            returnmsg = "Game room ID is used, choose another one\n";
                        }
                        else{
                            // create public room
                            if(permission == "public"){
                                people_gameroom[i] = roomid;
                                manager_room[i] = roomid;
                                id_permission[roomid] = 0; //public
                                room_startornot[roomid] = false;
                                room_people[roomid].push_back(i);
                                returnmsg = "You create public game room " + roomid + "\n";
                            }
                            // create private room
                            else if(permission == "private"){
                                int code;
                                rec >> code;
                                people_gameroom[i] = roomid;
                                manager_room[i] = roomid;
                                id_permission[roomid] = 1; //private
                                room_code[roomid] = code;
                                room_startornot[roomid] = false;
                                room_people[roomid].push_back(i);
                                returnmsg = "You create private game room " + roomid + "\n";
                            }
                        }
                        const char *STRING_mod = returnmsg.c_str();
                        send(client_socket[i], STRING_mod, strlen(STRING_mod), 0);
                    }
                    // join room <game room id>
                    if(instr == "join"){
                        string useless, roomid="", returnmsg, broadcastmsg;
                        rec >> useless >> roomid;
                        if(roomid == ""){
                            returnmsg = "Usage: join room <game room id>\n";
                        }
                        // Fail(1)
                        else if(!status[usr[i]]){
                            returnmsg = "You are not logged in\n";
                        }
                        // Fail(2)
                        else if(people_gameroom[i] != ""){
                            returnmsg = "You are already in game room " + people_gameroom[i] + ", please leave game room\n";
                        }
                        // Fail(3)
                        else if(id_permission.find(roomid) == id_permission.end()){
                            returnmsg = "Game room " + roomid + " is not exist\n";
                        }
                        // Fail(4)
                        else if(id_permission[roomid] == 1){
                            returnmsg = "Game room is private, please join game by invitation code\n";
                        }
                        // Fail(5)
                        else if(room_startornot[roomid]){
                            returnmsg = "Game has started, you can't join now\n";
                        }
                        // Success
                        else{
                            people_gameroom[i] = roomid;
                            room_people[roomid].push_back(i);
                            returnmsg = "You join game room " + people_gameroom[i] + "\n";
                            broadcastmsg = "Welcome, "+ usr[i] + " to game!\n";
                            for(int j = 0; j < room_people[roomid].size(); ++j){
                                if(room_people[roomid][j] != i){
                                    const char *STRING_mod2 = broadcastmsg.c_str();
                                    send(client_socket[room_people[roomid][j]], STRING_mod2, strlen(STRING_mod2), 0);
                                }
                            }
                        }
                        const char *STRING_mod = returnmsg.c_str();
                        send(client_socket[i], STRING_mod, strlen(STRING_mod), 0);
                    }
                    // invite <invitee email>
                    if(instr == "invite"){
                        string mail = "", returnmsg = "", broadcastmsg = "";
                        rec >> mail;
                        if(mail == ""){
                            returnmsg = "Usage: invite <invitee email>\n";
                        }
                        // Fail(1)
                        else if(!status[usr[i]]){
                            returnmsg = "You are not logged in\n";
                        }
                        // Fail(2)
                        else if(people_gameroom[i] == ""){
                            returnmsg = "You did not join any game room\n";
                        }
                        // Fail(3)
                        else if(manager_room[i] == ""){
                            returnmsg = "You are not private game room manager\n";
                        }
                        else{
                            string invitee;
                            for(auto it = email.begin(); it != email.end(); ++it){
                                if(it->second == mail){
                                    invitee = it->first;
                                }
                            }
                            // Fail(4)
                            if(!status[invitee]){
                                returnmsg = "Invitee not logged in\n";
                            }
                            // Success
                            else{
                                int inviteeid;
                                for(auto it = usr.begin(); it != usr.end(); ++it){
                                    if(it->second == invitee){
                                        inviteeid = it->first;
                                    }
                                }
                                invitee_inviter[inviteeid].push_back(i);
                                returnmsg = "You send invitation to "+ invitee + "<"+email[invitee] + ">\n";
                                broadcastmsg = "You receive invitation from " + usr[i] + "<"+email[usr[i]] + ">\n";
                                const char *STRING_mod2 = broadcastmsg.c_str();
                                send(client_socket[inviteeid], STRING_mod2, strlen(STRING_mod2), 0);
                            }
                        }
                        const char *STRING_mod = returnmsg.c_str();
                        send(client_socket[i], STRING_mod, strlen(STRING_mod), 0);
                    }
                    // list invitations
                    if(instr == "list"){
                        string returnmsg;
                        // Fail(1)
                        if(!status[usr[i]]){
                            returnmsg = "You are not logged in\n";
                        }
                        else{
                            returnmsg = "List invitations\n";
                            // No invitations
                            if(invitee_inviter[i].size() == 0){
                                returnmsg += "No Invitations\n";
                            }
                            // at least one invitation
                            else{
                                map<string, string>temp_map;
                                for(int j = 0; j < invitee_inviter[i].size(); ++j){
                                    string invitername = usr[invitee_inviter[i][j]];
                                    string gameroomid = manager_room[invitee_inviter[i][j]];
                                    //! maybe change accept, leave, and exit to deleate invalid invitations?
                                    // if(gameroomid == "") continue; 
                                    temp_map[gameroomid] = invitername;
                                    
                                }
                                int seq = 1;
                                for(auto it = temp_map.begin(); it!=temp_map.end(); ++it){
                                    string roomid = it->first;
                                    string invitername = it->second;
                                    string inviteremail = email[invitername];
                                    int invitationcode = room_code[roomid];
                                    returnmsg += (to_string(seq) + ". " + invitername + "<" + inviteremail + "> invite you to join game room " + roomid + ", invitation code is " + to_string(invitationcode) + "\n");
                                    seq++;
                                }
                            }
                        }
                        
                        const char *STRING_mod = returnmsg.c_str();
                        send(client_socket[i], STRING_mod, strlen(STRING_mod), 0);
                    }
                    // accept <inviter email> <invitation code>
                    if(instr == "accept"){
                        string inviteremail, returnmsg, broadcastmsg;
                        int invitationcode = -1;
                        rec >> inviteremail >> invitationcode;
                        if(inviteremail == "" || invitationcode == -1){
                            returnmsg = "Usage: accept <invitee email> <invitation code>\n";
                        } 
                        // Fail(1)
                        else if(!status[usr[i]]){
                            returnmsg = "You are not logged in\n";
                        }
                        // Fail(2)
                        else if(people_gameroom[i] != ""){
                            returnmsg = "You are already in game room " + people_gameroom[i] + ", please leave game room\n";
                        }
                        else{
                            int inviter_id = -1;
                            string roomid = "";
                            for(int j = 0; j < invitee_inviter[i].size(); ++j){
                                if(email[usr[invitee_inviter[i][j]]] == inviteremail){
                                    inviter_id = invitee_inviter[i][j];
                                    roomid = manager_room[inviter_id];
                                }
                            }
                            // Fail(3)
                            //! 有invitee is invited but the inviter leave the game room, so the invitation is expired 的問題
                            if(inviter_id == -1){
                                returnmsg = "Invitation not exist\n";
                            }
                            else if(people_gameroom[inviter_id] == ""){
                                returnmsg = "Invitation not exist\n";
                            }
                            // Fail(4)
                            else if(room_code[roomid] != invitationcode){
                                returnmsg = "Your invitation code is incorrect\n";
                            }
                            // Fail(5)
                            else if(room_startornot[roomid]){
                                returnmsg = "Game has started, you can't join now\n";
                            }
                            // Success
                            else{
                                people_gameroom[i] = roomid;
                                room_people[roomid].push_back(i);
                                returnmsg = "You join game room " + people_gameroom[i] + "\n";
                                broadcastmsg = "Welcome, "+ usr[i] + " to game!\n";
                                for(int j = 0; j < room_people[roomid].size(); ++j){
                                    if(room_people[roomid][j] != i){
                                        const char *STRING_mod2 = broadcastmsg.c_str();
                                        send(client_socket[room_people[roomid][j]], STRING_mod2, strlen(STRING_mod2), 0);
                                    }
                                }
                                
                            }
                        }
                        const char *STRING_mod = returnmsg.c_str();
                        send(client_socket[i], STRING_mod, strlen(STRING_mod), 0);
                    }
                    // leave room
                    if(instr == "leave"){
                        string returnmsg = "", broadcastmsg = "";
                        string roomid = people_gameroom[i];
                        // cout << target[roomid] << endl;
                        // Fail(1)
                        if(!status[usr[i]]){
                            returnmsg = "You are not logged in\n";
                        }
                        // Fail(2)
                        else if(people_gameroom[i] == ""){
                            returnmsg = "You did not join any game room\n";
                        }
                        // Success
                        else{
                            // people_gameroom.erase(i);
                            returnmsg = "You leave game room " + roomid + "\n";
                            // Success(1) room manager leave -> destroy room
                            if(manager_room[i] == roomid){ 
                                broadcastmsg = "Game room manager leave game room " + roomid + ", you are forced to leave too\n";
                                people_gameroom.erase(i);
                                //game related
                                target.erase(people_gameroom[i]);
                                times.erase(people_gameroom[i]);
                                wordbank.erase(people_gameroom[i]);
                                room_startornot.erase(roomid);
                                player_seq.erase(people_gameroom[i]);
                                max_player_index.erase(people_gameroom[i]);
                                cur_player_id.erase(people_gameroom[i]);
                                //game room related
                                if(id_permission[roomid] == 1){ 
                                    room_code.erase(roomid); 
                                }
                                id_permission.erase(roomid);
                                manager_room.erase(i);
                                room_startornot.erase(roomid);
                                //! not sure it is valid or not
                                vector<pair<int, int>>remove;
                                for(auto it = invitee_inviter.begin(); it != invitee_inviter.end(); ++it){
                                    int invitee_id = it->first;
                                    for(int j = 0; j < it->second.size(); ++j){
                                        bool same = (it->second[j] == i);
                                        if(same){
                                            // it->second.erase(it2);
                                            remove.push_back(make_pair(invitee_id, j));
                                        }
                                    }
                                }
                                for(int j = 0; j < remove.size(); ++j){
                                    invitee_inviter[remove[j].first].erase(invitee_inviter[remove[j].first].begin() + remove[j].second);
                                }
                                
                                room_people.erase(roomid);
                                vector<map<int, string>::iterator>remove_it;
                                for(auto it = people_gameroom.begin(); it != people_gameroom.end(); ++it){
                                    bool same =(it->second == roomid);
                                    if(same){
                                        int id = it->first;
                                        const char *STRING_mod2 = broadcastmsg.c_str();
                                        send(client_socket[id], STRING_mod2, strlen(STRING_mod2), 0);
                                        remove_it.push_back(it);
                                    }
                                }
                                for(auto it : remove_it){
                                    people_gameroom.erase(it);
                                }
                            }
                            // Success(2) game is going -> stop game and remain room
                            else if(room_startornot[roomid]){
                                returnmsg.pop_back();
                                returnmsg += ", game ends\n"; 
                                broadcastmsg = usr[i] + " leave game room " + roomid + ", game ends\n";
                                //game related
                                target.erase(people_gameroom[i]);
                                times.erase(people_gameroom[i]);
                                wordbank.erase(people_gameroom[i]);
                                room_startornot[people_gameroom[i]] = false;
                                player_seq.erase(people_gameroom[i]);
                                max_player_index.erase(people_gameroom[i]);
                                cur_player_id.erase(people_gameroom[i]);
                                //game room related
                                people_gameroom.erase(i);
                                vector<vector<int>::iterator>remove_it;
                                for(auto it = room_people[roomid].begin(); it != room_people[roomid].end(); ++it){
                                    if(*it == i){
                                        remove_it.push_back(it);
                                        //break;
                                    }
                                }
                                for(auto it : remove_it){
                                    room_people[roomid].erase(it);
                                }
                                for(auto it = people_gameroom.begin(); it != people_gameroom.end(); ++it){
                                    bool same =(it->second == roomid);
                                    if(same){
                                        int id = it->first;
                                        const char *STRING_mod2 = broadcastmsg.c_str();
                                        send(client_socket[id], STRING_mod2, strlen(STRING_mod2), 0);
                                    }
                                }
                            }
                            // Success(3) remain room
                            else{ //
                                broadcastmsg = usr[i] + " leave game room " + roomid + "\n";
                                people_gameroom.erase(i);
                                vector<vector<int>::iterator>remove_it;
                                for(auto it = room_people[roomid].begin(); it != room_people[roomid].end(); ++it){
                                    if(*it == i){
                                        remove_it.push_back(it);
                                        //break;
                                    }
                                }
                                for(auto it : remove_it){
                                    room_people[roomid].erase(it);
                                }
                                for(auto it = people_gameroom.begin(); it != people_gameroom.end(); ++it){
                                    bool same =(it->second == roomid);
                                    if(same){
                                        int id = it->first;
                                        const char *STRING_mod2 = broadcastmsg.c_str();
                                        send(client_socket[id], STRING_mod2, strlen(STRING_mod2), 0);
                                    }
                                }
                            }
                            // cout << "after leave:\n";
                            // for(auto it = people_gameroom.begin(); it!=people_gameroom.end(); ++it){
                                // cout << it->first << " " << it->second << endl;
                            // }                            
                        }
                        const char *STRING_mod = returnmsg.c_str();
                        send(client_socket[i], STRING_mod, strlen(STRING_mod), 0);
                    }
                    // start game <number of rounds> <guess number>
                    if(instr == "start"){
                        string useless, guessnum, returnmsg, broadcastmsg;
                        int num_rounds = -1;
                        rec >> useless >> num_rounds >> guessnum;
                        if(num_rounds == -1){
                            returnmsg = "Usage: start game <number of rounds> <guess number>";
                        }
                        // Fail(1)
                        else if(!status[usr[i]]){
                            returnmsg = "You are not logged in\n";
                        }
                        // Fail(2)
                        else if(people_gameroom[i] == ""){
                            returnmsg = "You did not join any game room\n";
                        }
                        // Fail(3)
                        else if(manager_room[i] == ""){
                            returnmsg = "You are not game room manager, you can't start game\n";
                        }
                        // Fail(4)
                        else if(room_startornot[people_gameroom[i]]){
                            returnmsg = "Game has started, you can't start again\n";
                        }
                        else{
                            if(guessnum != ""){
                                bool qualify = true;
                                // Fail(5)
                                if(guessnum.size() != 4){
                                    returnmsg = "Please enter 4 digit number with leading zero\n";
                                    qualify = false;
                                }
                                else{
                                    for(int j = 0; j < guessnum.size(); ++j){
                                        // Fail(5)
                                        if(!isdigit(guessnum[j])){
                                            returnmsg = "Please enter 4 digit number with leading zero\n"; 
                                            qualify = false;
                                            break;
                                        }
                                    }
                                }
                                // Success
                                if(qualify) {
                                    // cout << "in!\n";
                                    room_startornot[people_gameroom[i]] = true;
                                    player_seq[people_gameroom[i]]= 0;
                                    max_player_index[people_gameroom[i]]= room_people[people_gameroom[i]].size()-1;
                                    cur_player_id[people_gameroom[i]] = room_people[people_gameroom[i]][player_seq[people_gameroom[i]]];
                                    returnmsg = "Game start! Current player is " + usr[cur_player_id[people_gameroom[i]]] + "\n";
                                    target[people_gameroom[i]] = guessnum;
                                    times[people_gameroom[i]] = num_rounds * (room_people[people_gameroom[i]].size());
                                    for(int j = 0; j < target[people_gameroom[i]].size(); ++j){
                                        wordbank[people_gameroom[i]].push_back(target[people_gameroom[i]][j]);
                                    }
                                    broadcastmsg = returnmsg;
                                    for(int j = 0; j < room_people[people_gameroom[i]].size(); ++j){
                                        if(room_people[people_gameroom[i]][j] != i){
                                            const char *STRING_mod2 = broadcastmsg.c_str();
                                            send(client_socket[room_people[people_gameroom[i]][j]], STRING_mod2, strlen(STRING_mod2), 0);
                                        }
                                    }
                                }
                            }
                            // Success
                            else if(guessnum == ""){
                                stringstream inttostr;
                                string temp;
                                int number = rand() % 9000 + 1000;
                                inttostr << number;
                                inttostr >> target[people_gameroom[i]];
                                room_startornot[people_gameroom[i]] = true;
                                player_seq[people_gameroom[i]]= 0;
                                max_player_index[people_gameroom[i]]= room_people[people_gameroom[i]].size()-1;
                                cur_player_id[people_gameroom[i]] = room_people[people_gameroom[i]][player_seq[people_gameroom[i]]];
                                returnmsg = "Game start! Current player is " + usr[cur_player_id[people_gameroom[i]]] + "\n";
                                times[people_gameroom[i]] = num_rounds * (room_people[people_gameroom[i]].size());
                                for(int j = 0; j < target[people_gameroom[i]].size(); ++j){
                                    wordbank[people_gameroom[i]].push_back(target[people_gameroom[i]][j]);
                                }
                                broadcastmsg = returnmsg;
                                for(int j = 0; j < room_people[people_gameroom[i]].size(); ++j){
                                    if(room_people[people_gameroom[i]][j] != i){
                                        const char *STRING_mod2 = broadcastmsg.c_str();
                                        send(client_socket[room_people[people_gameroom[i]][j]], STRING_mod2, strlen(STRING_mod2), 0);
                                    }
                                }
                            }   
                        }
                        const char *game = returnmsg.c_str();
                        send(client_socket[i], game, strlen(game), 0);
                        // cout << "start end" << endl;
                    }
                    // guess <guess number>
                    if(instr == "guess"){
                        string guessnum, returnmsg, broadcastmsg;
                        rec >> guessnum;
                        if(guessnum == ""){
                            returnmsg = "Usage: guess <guess number>\n";
                        }
                        // Fail(1)
                        else if(!status[usr[i]]){
                            returnmsg = "You are not logged in\n";
                        }
                        // Fail(2)
                        else if(people_gameroom[i] == ""){
                            returnmsg = "You did not join any game room\n";
                        }
                        // Fail(3)
                        else if(!room_startornot[people_gameroom[i]]){
                            if(manager_room[i] == people_gameroom[i]){
                                returnmsg = "You are game room manager, please start game first\n";
                            }
                            else{
                                returnmsg = "Game has not started yet\n";
                            }
                        }
                        // Fail(4)
                        else if(cur_player_id[people_gameroom[i]] != i){
                            returnmsg = "Please wait..., current player is " + usr[cur_player_id[people_gameroom[i]]] + "\n";
                        }
                        // Success(Bingo) -> game end, room remain
                        else if(guessnum == target[people_gameroom[i]]){
                            returnmsg = usr[i] + " guess '"+ guessnum+ "' and got Bingo!!! " + usr[i] + " wins the game, game ends\n";
                            broadcastmsg = returnmsg;
                            //game related
                            target.erase(people_gameroom[i]);
                            times.erase(people_gameroom[i]);
                            wordbank.erase(people_gameroom[i]);
                            room_startornot[people_gameroom[i]] = false;
                            player_seq.erase(people_gameroom[i]);
                            max_player_index.erase(people_gameroom[i]);
                            cur_player_id.erase(people_gameroom[i]);
                            for(int j = 0; j < room_people[people_gameroom[i]].size(); ++j){
                                if(room_people[people_gameroom[i]][j] != i){
                                    const char *STRING_mod2 = broadcastmsg.c_str();
                                    send(client_socket[room_people[people_gameroom[i]][j]], STRING_mod2, strlen(STRING_mod2), 0);
                                }
                            }
                        }
                        else{ //not finished or lose
                            bool valid = true;
                            string process = guessnum;
                            if(guessnum.size()!= 4){
                                valid = false;
                            }
                            else{
                                for(long long j = 0; j < guessnum.size(); ++j){
                                    if(!isdigit(guessnum[j])){
                                        valid = false;
                                    }
                                }
                            }
                            // Fail(5)
                            if(!valid){
                                returnmsg = "Please enter 4 digit number with leading zero\n";
                            }
                            // Success
                            else{
                                vector<bool>targetvalid(4, false);
                                times[people_gameroom[i]]--;
                                player_seq[people_gameroom[i]]++;
                                if(player_seq[people_gameroom[i]] > max_player_index[people_gameroom[i]]){
                                    player_seq[people_gameroom[i]] = 0;
                                }
                                cur_player_id[people_gameroom[i]] = room_people[people_gameroom[i]][player_seq[people_gameroom[i]]];
                                int a = 0, b = 0;
                                for(int j = 0; j < process.size(); ++j){
                                    if(process[j] == target[people_gameroom[i]][j]){
                                        a++;
                                        process[j] = ' ';
                                        targetvalid[j] = true;
                                    }
                                }   
                                for(int j = 0; j < process.size(); ++j){
                                    for(int k = 0; k < target[people_gameroom[i]].size(); ++k){
                                        if(j == k || targetvalid[k]) continue;
                                        else if(process[j] == target[people_gameroom[i]][k]){
                                            b++;
                                            targetvalid[k] = true;
                                            break;
                                        }
                                    }
                                }
                                stringstream chartostr;
                                string result;
                                chartostr << a << "A" << b << "B";
                                chartostr >> result;
                                returnmsg = usr[i] + " guess '" + guessnum + "' and got '" + result + "'\n";
                                // No chances
                                if(times[people_gameroom[i]] == 0){
                                    //game related
                                    target.erase(people_gameroom[i]);
                                    times.erase(people_gameroom[i]);
                                    wordbank.erase(people_gameroom[i]);
                                    room_startornot[people_gameroom[i]] = false;
                                    player_seq.erase(people_gameroom[i]);
                                    max_player_index.erase(people_gameroom[i]);
                                    cur_player_id.erase(people_gameroom[i]);
                                    returnmsg += "Game ends, no one wins\n";
                                }
                                broadcastmsg = returnmsg;
                                for(int j = 0; j < room_people[people_gameroom[i]].size(); ++j){
                                    if(room_people[people_gameroom[i]][j] != i){
                                        const char *STRING_mod2 = broadcastmsg.c_str();
                                        send(client_socket[room_people[people_gameroom[i]][j]], STRING_mod2, strlen(STRING_mod2), 0);
                                    }
                                }
                            }
                        }
                        const char *STRING_mod = returnmsg.c_str();
                        send(client_socket[i], STRING_mod, strlen(STRING_mod), 0);
                    }
                    // exit
                    if (instr == "exit"){
                        cout << "tcp get msg: exit" << endl;
                        close( sd );  
                        client_socket[i] = 0;
                        if(status[usr[i]] == true){
                            int server1, server2, server3;
                            ifstream readfile("/home/ec2-user/efs/record");
                            readfile >> server1 >> server2 >> server3;
                            readfile.close();
			    server1 -= 1;
                            string writetorecord = to_string(server1) + " " + to_string(server2) + " "+to_string(server3);
                            ofstream writefile("/home/ec2-user/efs/record");
                            writefile << writetorecord;
                            writefile.close();
                        }
                        status[usr[i]] = false;
                        usr[i] = "";
                        string roomid = people_gameroom[i];   
                        people_gameroom.erase(i);
                        vector<vector<int>::iterator>remove_it;
                        for(auto it = room_people[roomid].begin(); it != room_people[roomid].end(); ++it){
                            if(*it == i){
                                remove_it.push_back(it);
                                //break;
                            }
                        }
                        for(auto it : remove_it){
                            room_people[roomid].erase(it);
                        }
                        // game is going on -> stop game
                        if(room_startornot[roomid]){
                            //game related
                            target.erase(roomid);
                            times.erase(roomid);
                            wordbank.erase(roomid);
                            room_startornot[roomid] = false;
                            player_seq.erase(roomid);
                            max_player_index.erase(roomid);
                            cur_player_id.erase(roomid);
                        }
                        // you are room manager -> destory room
                        if(manager_room[i] != ""){
                            if(id_permission[roomid] == 1){ 
                                room_code.erase(roomid); 
                            }
                            id_permission.erase(roomid);
                            manager_room.erase(i);
                            room_startornot.erase(roomid);
                            //! not sure it is valid or not
                            vector<pair<int, int>>remove;
                            for(auto it = invitee_inviter.begin(); it != invitee_inviter.end(); ++it){
                                int invitee_id = it->first;
                                for(int j = 0; j < it->second.size(); ++j){
                                    bool same = (it->second[j] == i);
                                    if(same){
                                        // it->second.erase(it2);
                                        remove.push_back(make_pair(invitee_id, j));
                                    }
                                }
                            }
                            for(int j = 0; j < remove.size(); ++j){
                                invitee_inviter[remove[j].first].erase(invitee_inviter[remove[j].first].begin() + remove[j].second);
                            }
                            
                            room_people.erase(roomid);
                            vector<map<int, string>::iterator>remove_it2;
                            for(auto it = people_gameroom.begin(); it != people_gameroom.end(); ++it){
                                bool same =(it->second == roomid);
                                if(same){
                                    remove_it2.push_back(it);
                                }
                            }
                            for(auto it : remove_it2){
                                people_gameroom.erase(it);
                            }
                        }
                        
                        FD_ZERO(&readfds);
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
            // register
            if(instr == "register"){
                bool sameemail = false;
                string username = "", mail = "", pwd = "";
                rec >> username >> mail >> pwd;
                if(username == "" || mail == ""||pwd == ""){
                    returnmsg = "Usage: register <username> <email> <password>\n";
                }
                // Fail(1)
                else if(password.find(username) != password.end()){
                    returnmsg = "Username or Email is already used\n";
                }
                else{
                    for(auto it = email.begin(); it != email.end(); ++it){
                        if(it->second == mail){
                            sameemail = true;
                            break;
                        }
                    }
                    // Fail(1)
                    if(sameemail){
                        returnmsg = "Username or Email is already used\n";
                    }
                    // Success
                    else{
                        returnmsg = "Register Successfully\n";
                        password[username] = pwd;
                        email[username] = (mail);
                        status[username] = false;
                    }
                }
                
            }
            // list rooms
            // list users
            if (instr == "list"){
                string what;
                rec >> what;
                // list rooms
                if(what == "rooms"){
                    returnmsg = "List Game Rooms\n";
                    // no game room
                    if(id_permission.size() == 0){
                        returnmsg += "No Rooms\n";
                    }
                    //at least one game room
                    else{
                        int seq = 1;
                        for(auto it = id_permission.begin(); it != id_permission.end(); ++it){
                            string roomid = it->first;
                            int permission = it->second;
                            bool startornot = room_startornot[roomid];
                            // public room
                            if(permission == 0){
                                returnmsg += (to_string(seq) + ". (Public) Game Room " + roomid);
                            } 
                            // private room
                            else if(permission == 1){
                                returnmsg += (to_string(seq) + ".(Private) Game Room " + roomid);
                            }
                            // has started playing
                            if(startornot){
                                returnmsg += " has started playing\n";
                            }
                            // is open for players
                            else if(!startornot){
                                returnmsg += " is open for players\n";
                            }
                            seq++;
                        }
                    }
                }
                else if(what == "users"){
                    returnmsg = "List Users\n";
                    // No user registered
                    if(email.size() == 0){
                        returnmsg += "No Users\n";
                    }
                    // at least one user registered
                    else{
                        int seq = 1;
                        for(auto it = email.begin(); it != email.end(); ++it){
                            returnmsg += (to_string(seq) + ". " + it->first + "<" + it->second + ">");
                            // online
                            if(status[it->first]){
                                returnmsg += " Online\n";
                            }
                            // offline
                            else if(!status[it->first]){
                                returnmsg += " Offline\n";
                            }
                            seq++;
                        }
                    }
                }
            } 
            const char *re = returnmsg.c_str();
            sendto(udpfd, re, strlen(re), 0, (sockaddr*)&udpclient, sizeof(udpclient));
        }
        // cout << "here!"<<endl;
    }
    
    ifstream rfile("/home/ec2-user/efs/server_status");
    rfile >> status1 >> status2 >> status3;
    rfile.close();
    status1 = 0;
    string writestatus = to_string(status1) + " " + to_string(status2) + " "+to_string(status3);
    ofstream wfile("/home/ec2-user/efs/server_status");
    wfile << writestatus;
    wfile.close();

    return 0;
}
