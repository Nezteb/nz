/*

nzProx.cpp
Noah Betzen

A simple reverse proxy and load balancer in C++.

Compile with:
g++ nzProx.cpp -o nzProx.out

Run with:
./nzProx.out -l PORT -h HOST0 -j HOST1 -p PORT [-f]

*/

#include <arpa/inet.h>
#include <libgen.h>
#include <netdb.h>
#include <resolv.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <wait.h>
#include <iostream>
using std::cout;
using std::endl;
#include <fstream>
using std::ofstream;
#include <sstream>
using std::stringstream;

#define BUFFERSIZE 1024

#define PIPEREAD  0
#define PIPEWRITE 1

// forward declarations
void forwardData(int sourceSock, int destinationSock);
void sigchldHandler(int signal);
void sigtermHandler(int signal);
void handleClient(int clientSock, struct sockaddr_in clientAddr, bool sendToFirst);
int createSocket(int port);
int createConnection(int num);
int parseArguments(int argc, char *argv[]);
void serverLoop();

int serverSock;
int clientSock;
int remoteSock0;
int remoteSock1;
int remotePort = 0;

char *remoteHost0;
char *remoteHost1;
bool foreground = false;

void logIt(stringstream &logStream)
{
	ofstream logFile("nzProx.log", std::ios_base::out | std::ios_base::app);
    logFile << logStream.rdbuf();
    logStream.str("");
}
stringstream logStream;

int main(int argc, char *argv[])
{
    int localPort;
    pid_t pid;

    localPort = parseArguments(argc, argv);

    if (localPort < 0)
    {
        logStream << "Usage: " << argv[0] << " -l localPort -h remoteHost0 -j remoteHost1 -p remotePort [-f (stay in foreground)]" << endl;
        logIt(logStream);
        exit(1);
    }

    // start server
    if ((serverSock = createSocket(localPort)) < 0)
    {
        logStream << "ERROR: Cannot run server!" << endl;
        logIt(logStream);
        exit(1);
    }

    signal(SIGCHLD, sigchldHandler); // prevent ended children from becoming zombies
    signal(SIGTERM, sigtermHandler); // handle KILL signal

    if (foreground)
    {
        serverLoop();
    }
    else // run in background
    {
        switch(pid = fork())
        {
            case 0: // child
                serverLoop();
                break;
            case -1: // error
                logStream << "ERROR: Cannot spawn child!" << endl;
                logIt(logStream);
                exit(1);
            default: // parent
                close(serverSock);
        }
    }

    return 0;
}

int parseArguments(int argc, char *argv[])
{
    int argument;
    int localPort = 0;

    while ((argument = getopt(argc, argv, "l:h:j:p:f")) != -1)
    {
        switch(argument)
        {
            case 'l':
                localPort = atoi(optarg);
                break;
            case 'h':
                remoteHost0 = optarg;
                break;
            case 'j':
                remoteHost1 = optarg;
                break;
            case 'p':
                remotePort = atoi(optarg);
                break;
            case 'f':
                foreground = true;
        }
    }

    if (localPort && remoteHost0 && remoteHost1 && remotePort)
    {
        return localPort;
    }
    else
    {
        logStream << "ERROR: Bad syntax!" << endl;
        logIt(logStream);
        return -1;
    }
}

int createSocket(int port)
{
    int serverSock;
    int optval;
    struct sockaddr_in serverAddr;

    if ((serverSock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        logStream << "ERROR: Bad server socket!" << endl;
        logIt(logStream);
        return -1;
    }

    if (setsockopt(serverSock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0)
    {
        logStream << "ERROR: Cannot set server socket options!" << endl;
        logIt(logStream);
        return -1;
    }

    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(serverSock, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) != 0)
    {
        logStream << "ERROR: Bad server bind!" << endl;
        logIt(logStream);
        return -1;
    }

    if (listen(serverSock, 20) < 0)
    {
        logStream << "ERROR: Server cannot listen!" << endl;
        logIt(logStream);
        return -1;
    }

    return serverSock;
}

void sigchldHandler(int signal)
{
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

void sigtermHandler(int signal)
{
    close(clientSock);
    close(serverSock);
    exit(0);
}

void serverLoop()
{
    struct sockaddr_in clientAddr;
    socklen_t addrLength = sizeof(clientAddr);
    bool sendToFirst = true;

    while (true)
    {
        clientSock = accept(serverSock, (struct sockaddr*)&clientAddr, &addrLength);

        logStream << "LOOP CHOICE: " << (sendToFirst ? "first" : "second") << endl;
        logIt(logStream);

        // handle client connection in a separate process
        if (fork() == 0)
        {
            close(serverSock);

            logStream << "FORK CHOICE: " << getpid() << " " << (sendToFirst ? "first" : "second") << endl;
            logIt(logStream);

            handleClient(clientSock, clientAddr, sendToFirst);

            exit(0);
        }

        sendToFirst = !sendToFirst;

        close(clientSock);
    }

}

void handleClient(int clientSock, struct sockaddr_in clientAddr, bool sendToFirst)
{
    if ((remoteSock0 = createConnection(0)) < 0)
    {
        logStream << "ERROR: Cannot connect to host!" << endl;
        logIt(logStream);
        return;
    }
    if ((remoteSock1 = createConnection(1)) < 0)
    {
        logStream << "ERROR: Cannot connect to host!" << endl;
        logIt(logStream);
        return;
    }

    // a process forwarding data from client to remote socket
   if (fork() == 0)
   {
       logStream << "FORK2 CHOICE: " << getpid() << " " << (sendToFirst ? "first" : "second") << endl;
       logIt(logStream);

       if(sendToFirst)
       {
           forwardData(clientSock, remoteSock0);
           exit(0);
       }
       else
       {
           forwardData(clientSock, remoteSock1);
           exit(0);
       }
   }

   // a process forwarding data from remote socket to client
   if (fork() == 0)
   {
       logStream << "FORK2 CHOICE: " << getpid() << " " << (sendToFirst ? "first" : "second") << endl;
       logIt(logStream);

       if(sendToFirst)
       {
           forwardData(remoteSock0, clientSock);
           exit(0);
       }
       else
       {
           forwardData(remoteSock1, clientSock);
           exit(0);
       }
   }

   close(remoteSock0);
   close(remoteSock1);
   close(clientSock);
}

void forwardData(int sourceSock, int destinationSock)
{
    char buffer[BUFFERSIZE];
    int retVal;

    // read data from input socket
    while ((retVal = recv(sourceSock, buffer, BUFFERSIZE, 0)) > 0)
    {
        send(destinationSock, buffer, retVal, 0); // send data to output socket
    }

    shutdown(destinationSock, SHUT_RDWR); // stop other processes from using socket
    close(destinationSock);

    shutdown(sourceSock, SHUT_RDWR); // stop other processes from using socket
    close(sourceSock);
}

int createConnection(int num)
{
    struct sockaddr_in serverAddr0;
    struct sockaddr_in serverAddr1;
    struct hostent *server0;
    struct hostent *server1;
    bool first = true;
    int sock;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        logStream << "ERROR: Bad client socket!" << endl;
        logIt(logStream);
        return -1;
    }

    if(num == 0)
    {
        if ((server0 = gethostbyname(remoteHost0)) == NULL)
        {
            logStream << "ERROR: Bad client resolve!" << endl;
            logIt(logStream);
            return -1;
        }
        memset(&serverAddr0, 0, sizeof(serverAddr0));
        serverAddr0.sin_family = AF_INET;
        memcpy(&serverAddr0.sin_addr.s_addr, server0->h_addr, server0->h_length);
        serverAddr0.sin_port = htons(remotePort);
        if (connect(sock, (struct sockaddr *) &serverAddr0, sizeof(serverAddr0)) < 0)
        {
            logStream << "ERROR: Bad client connection!" << endl;
            logIt(logStream);
            return -1;
        }
    }
    else
    {
        if ((server1 = gethostbyname(remoteHost1)) == NULL)
        {
            logStream << "ERROR: Bad client resolve!" << endl;
            logIt(logStream);
            return -1;
        }
        memset(&serverAddr1, 0, sizeof(serverAddr1));
        serverAddr1.sin_family = AF_INET;
        memcpy(&serverAddr1.sin_addr.s_addr, server1->h_addr, server1->h_length);
        serverAddr1.sin_port = htons(remotePort);
        if (connect(sock, (struct sockaddr *) &serverAddr1, sizeof(serverAddr1)) < 0)
        {
            logStream << "ERROR: Bad client connection!" << endl;
            logIt(logStream);
            return -1;
        }
    }

    return sock;
}
