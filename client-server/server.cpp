//
//  server.cpp
//  Lab07
//
//

//#define DEBUG

#include <iostream>
#include <errno.h>
#include <string>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

const char HOST_IP[] = "127.0. 0.1"; /// localhost
const int BUF_LEN = 4096;            /// buffer length
const int MSG_LEN = 256;             /// message length
const long TIMEOUT = 5;              /// will shutdown after a timeout
const int MAX_CLIENT = 5;


enum status { err, report };
enum exit_code { ERR_ARG = 1, ERR_CONNECT };

void handleIssue(status stat = err, const char* msg = nullptr, int serverFd = -1);


int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Port number is not provided." << std::endl;
        exit(ERR_ARG);
    }
    
    struct sockaddr_in serverAddr;
    bzero(&serverAddr, sizeof(serverAddr));
    char buf[BUF_LEN], msg[MSG_LEN];
    bzero(buf, BUF_LEN);
    bzero(msg, MSG_LEN);
    int masterFd, newSocketFd, clientFDs[MAX_CLIENT], maxFd;
    int numClients = 0;
    int portNum = atoi(argv[1]);    /// assumption: port number is valid
    fd_set activeFDs, readFDs;
    FD_ZERO(&activeFDs);
    FD_ZERO(&readFDs);
    size_t returnVal;
    timeval timeout;                /// terminating program after TIMEOUT seconds
    bool isRunning = true;
    bool willAccept = true;         /// server will accept up  to MAX_CLIENT
    
    
    // create socket
    masterFd = socket(AF_INET, SOCK_STREAM, 0);
    if ( masterFd == -1) handleIssue(err,"error on creating socket");
    
    // setup sockaddr_in structs
    serverAddr.sin_family = AF_INET;
    returnVal = inet_pton(AF_INET, HOST_IP, &serverAddr.sin_addr);
    if (returnVal < 0) handleIssue(err, "no such host IP address", masterFd);
    /**
        if (returnVal == 0) then according to `man7.org` port  does not contain a character string representing a valid network address in the specified address family.
        But it could succeed in connecting on virtual Ubuntu on Docker.
     */
    serverAddr.sin_port = htons(portNum);
    
    // bind the socket to the local socket file
    returnVal = bind(masterFd, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
    if (returnVal == -1) handleIssue(err,"error on binding socket", masterFd);
    #ifdef DEBUG
    std::cout << "server fd " << masterFd << " bound to address ";
    std::cout << inet_ntoa(serverAddr.sin_addr) << ":" << portNum << std::endl;
    #endif
    
    // listen for clients to connect
    std::cout << "Waiting clients..." << std::endl;
    returnVal = listen(masterFd, MAX_CLIENT);
    if (returnVal == -1) handleIssue(err,"error on start listening", masterFd);
    
    // adding masterFd to the set fd_set
    FD_SET(masterFd, &activeFDs);
    maxFd = masterFd;
    
    // setting timeout
    timeout.tv_sec = TIMEOUT;
    timeout.tv_usec = 100; /// tolerence
    
    
    while(isRunning) {
        readFDs = activeFDs;
        // select from up to maxFd + 1 sockets
        returnVal = select(maxFd + 1, &readFDs, NULL, NULL, &timeout);
        if (returnVal == -1) handleIssue(report, "couldn't select client");
        
        if (FD_ISSET(masterFd, &readFDs) && willAccept) { /// this is a new connection request
            newSocketFd = accept(masterFd, NULL, NULL);
            if (newSocketFd == -1) handleIssue(report, "couldn't accept client");
            else { // add the client to the active list and communicate
                std::cout << "server: incomming connection " << newSocketFd << std::endl;
                clientFDs[numClients] = newSocketFd;
                FD_SET(clientFDs[numClients], &activeFDs);
                
                // start the conversation
                returnVal = write(clientFDs[numClients], "Send Text", 10);
                if (returnVal == -1) {
                    sprintf(msg, "couldn't start talking with client %d", clientFDs[numClients]);
                    handleIssue(report, msg);
                }
                
                // setup boundaries
                if (maxFd < clientFDs[numClients]) maxFd = clientFDs[numClients];
                ++numClients;
                if (numClients == MAX_CLIENT) willAccept = false;
            }
        }
        else { // this is a former client
            // find the client and communicate
            for (int i = 0; i < numClients; i++) {
                if(FD_ISSET(clientFDs[i], &readFDs)) { /// found
                    returnVal = read(clientFDs[i], buf, BUF_LEN);
                    if (returnVal == -1) {
                        sprintf(msg, "couldn't read from client %d", clientFDs[i]);
                        handleIssue(report, msg);
                    }
                    else std::cout << buf << std::endl;
                }
            }
        }
        
        // check the timeout
        if (timeout.tv_sec == 0 && timeout.tv_usec < 100) isRunning = false;
    }
    
    // Request each client to quit
    for(int i=0; i< numClients; ++i) {
        returnVal = write(clientFDs[i], "Quit", 5);
        if (returnVal == -1) {
            sprintf(msg, "couldn't quit client %d", clientFDs[i]);
            handleIssue(report, msg);
        }
        FD_CLR(clientFDs[i], &activeFDs);
        close(clientFDs[i]);
    }
    
    FD_CLR(masterFd, &activeFDs);
    close(masterFd);
    
    return 0;
}

void handleIssue(status stat, const char* msg, int serverFd) {
    if (msg) std::cout << "server: " << msg;
    if (stat == err || stat == report) std::cout << " -> " << strerror(errno);
    std::cout << std::endl;
    if (serverFd != -1 && stat != report) {
        close(serverFd);
    }
    if (stat == err) exit(ERR_CONNECT);
}
