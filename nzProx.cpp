/*

nzProx.cpp
Noah Betzen

A simple reverse proxy and load balancer in C++.

Compile with:
g++ nzProx.cpp -o nzProx.out

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

#define BUFFERSIZE 1024

#define PIPEREAD  0
#define PIPEWRITE 1

// forward declarations
void forwardDataExt(int sourceSock, int destinationSock, char *command);
void forwardData(int sourceSock, int destinationSock);
void sigchldHandler(int signal);
void sigtermHandler(int signal);
void handleClient(int clientSock, struct sockaddr_in clientAddr);
int createSocket(int port);
int createConnection();
int parseArguments(int argc, char *argv[]);
void serverLoop();

int serverSock;
int clientSock;
int remoteSock;
int remotePort = 0;
char *remoteHost;
char *commandIn;
char *commandOut;
bool foreground = false;

int main(int argc, char *argv[])
{
    int localPort;
    pid_t pid;

    localPort = parseArguments(argc, argv);

    if (localPort < 0)
    {
        cout << "Usage: " << argv[0] << " -l localPort -h remoteHost -p remotePort [-f (stay in foreground)]\n";
        exit(1);
    }

    // start server
    if ((serverSock = createSocket(localPort)) < 0)
    {
        cout << "ERROR: Cannot run server!" << endl;
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
                cout << "ERROR: Cannot spawn child!" << endl;
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

    while ((argument = getopt(argc, argv, "l:h:p:f")) != -1)
    {
        switch(argument)
        {
            case 'l':
                localPort = atoi(optarg);
                break;
            case 'h':
                remoteHost = optarg;
                break;
            case 'p':
                remotePort = atoi(optarg);
                break;
            case 'f':
                foreground = true;
        }
    }

    if (localPort && remoteHost && remotePort)
    {
        return localPort;
    }
    else
    {
        cout << "ERROR: Bad syntax!" << endl;
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
        cout << "ERROR: Bad server socket!" << endl;
        return -1;
    }

    if (setsockopt(serverSock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0)
    {
        cout << "ERROR: Bad server socket opt!" << endl;
        return -1;
    }

    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(serverSock, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) != 0)
    {
        cout << "ERROR: Bad server bind!" << endl;
        return -1;
    }

    if (listen(serverSock, 20) < 0)
    {
        cout << "ERROR: Server cannot listen!";
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
    socklen_t addrlen = sizeof(clientAddr);

    while (true)
    {
        clientSock = accept(serverSock, (struct sockaddr*)&clientAddr, &addrlen);

        // handle client connection in a separate process
        if (fork() == 0)
        {
            close(serverSock);
            handleClient(clientSock, clientAddr);
            exit(0);
        }

        close(clientSock);
    }

}

void handleClient(int clientSock, struct sockaddr_in clientAddr)
{
    if ((remoteSock = createConnection()) < 0)
    {
        cout << "ERROR: Cannot connect to host!";
        return;
    }

     // a process forwarding data from client to remote socket
    if (fork() == 0)
    {
        forwardData(clientSock, remoteSock);

        exit(0);
    }

    // a process forwarding data from remote socket to client
    if (fork() == 0)
    {
        forwardData(remoteSock, clientSock);

        exit(0);
    }

    close(remoteSock);
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

int createConnection()
{
    struct sockaddr_in serverAddr;
    struct hostent *server;
    int sock;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        cout << "ERROR: Bad client socket!" << endl;
        return -1;
    }

    if ((server = gethostbyname(remoteHost)) == NULL)
    {
        cout << "ERROR: Bad client resolve!" << endl;
        return -1;
    }

    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    memcpy(&serverAddr.sin_addr.s_addr, server->h_addr, server->h_length);
    serverAddr.sin_port = htons(remotePort);

    if (connect(sock, (struct sockaddr *) &serverAddr, sizeof(serverAddr)) < 0)
    {
        cout << "ERROR: Bad client connection!" << endl;
        return -1;
    }

    return sock;
}
