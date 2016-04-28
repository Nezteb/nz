/*

nzServ.cpp
Noah Betzen

A simple web server in C++.

Compile with:
g++ nzServ.cpp -o nzServ.out

Run with:
./nzServ.out PORT DIR

NOTE: Do not use relative directories. Just cd to the HTML location and run the exectuable from there.

*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <iostream>
using std::cout;
using std::endl;
#include <fstream>
using std::ofstream;
#include <sstream>
using std::stringstream;

#define BUFFERSIZE 8096

struct
{
	char *extension;
	char *fileType;
} mimeTypes [] = {
	{"html","text/html"},
	{"js","text/js"},
	{"css","test/css"},
	{"png", "image/png"},
	{"jpg", "image/jpeg"},
	{0,0}
};

void logIt(stringstream &logStream)
{
	ofstream logFile("nzServ.log", std::ios_base::out | std::ios_base::app);
    logFile << logStream.rdbuf();
	logStream.str("");
}
stringstream logStream;

void web(int fd, int hit)
{
	int j;
	int fileFd;
	int bufferLength;
	int len;
	long i;
	long ret;
	char *fileStr;
	static char buffer[BUFFERSIZE+1];

	ret = read(fd, buffer, BUFFERSIZE);

	if(ret == 0 || ret == -1)
	{
		logStream << "ERROR: Failed to read browser request!" << endl;
		logIt(logStream);
		exit(1);
	}
	if(ret > 0 && ret < BUFFERSIZE)
	{
		buffer[ret] = 0;
	}
	else
	{
		buffer[0] = 0;
	}

	for(i=0;i<ret;i++)
	{
		if(buffer[i] == '\r' || buffer[i] == '\n')
		{
			buffer[i]='*';
		}
	}

	logStream << "LOG: Request: " << buffer << " " << hit << endl;
	logIt(logStream);

	if(strncmp(buffer,"GET ",4) && strncmp(buffer,"get ",4))
	{
		logStream << "ERROR: Can only handle GET requests!" << endl;
		logIt(logStream);
		exit(1);
	}

	for(i = 4; i < BUFFERSIZE; i++)
	{
		if(buffer[i] == ' ')
		{
			buffer[i] = 0;
			break;
		}
	}

	for(j = 0; j < i - 1; j++)
	{
		if(buffer[j] == '.' && buffer[j+1] == '.')
		{
			logStream << "ERROR: . and .. paths not supported." << endl;
			logIt(logStream);
			exit(1);
		}
	}

	if(!strncmp(&buffer[0], "GET /\0", 6) || !strncmp(&buffer[0], "get /\0", 6))
	{
		strcpy(buffer, "GET /index.html");
	}

	bufferLength = strlen(buffer);
	fileStr = (char *)0;

	for(i = 0; mimeTypes[i].extension != 0; i++)
	{
		len = strlen(mimeTypes[i].extension);

		if(!strncmp(&buffer[bufferLength-len], mimeTypes[i].extension, len))
		{
			fileStr = mimeTypes[i].fileType;
			break;
		}
	}
	if(fileStr == 0)
	{
		logStream << "ERROR: File extension not supported!" << endl;
		logIt(logStream);
		exit(1);
	}

	if((fileFd = open(&buffer[5], O_RDONLY)) == -1)
	{
		logStream << "ERROR: Failed to open file! " << buffer << endl;
		logIt(logStream);
		exit(1);
	}

	logStream << "LOG: Send: " << &buffer[5] << " " << hit << endl;
	logIt(logStream);

	sprintf(buffer,"HTTP/1.0 200 OK\r\nContent-Type: %s\r\n\r\n", fileStr);
	write(fd, buffer, strlen(buffer));

	while ((ret = read(fileFd, buffer, BUFFERSIZE)) > 0)
	{
		write(fd,buffer,ret);
	}

	sleep(1);
	exit(1);
}

int main(int argc, char *argv[])
{
	int i;
	int port;
	int pid;
	int listenFd;
	int socketFd;
	int hit;
	socklen_t length;
	static struct sockaddr_in cli_addr;
	static struct sockaddr_in serv_addr;

	if( argc < 3  || argc > 3 || !strcmp(argv[1], "-?") )
	{
		cout << "Usage: server [port] [server directory] [&]" << endl;
		exit(0);
	}

	if(fork() != 0)
	{
		return 0;
	}

	signal(SIGCLD, SIG_IGN);
	signal(SIGHUP, SIG_IGN);

	for(i = 0; i < 32; i++)
	{
		close(i);
	}

	setpgrp();

	logStream << "LOG: Web server starting: " << argv[1] << " " << getpid() << endl;
	logIt(logStream);

	if((listenFd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		logStream << "ERROR: Cannot create socket!" << endl;
		logIt(logStream);
		exit(1);
	}

	int enable = 1;
	if(setsockopt(listenFd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
	{
		logStream << "ERROR: Cannot set socket options!" << endl;
		logIt(logStream);
		exit(1);
	}

	port = atoi(argv[1]);
    logStream << "LOG: Port: " << port << endl;
    logIt(logStream);

	if(port < 0 || port > 65535)
	{
		logStream << "ERROR: Invalid port number, try 1 through 65535! Port: " << argv[1] << endl;
		logIt(logStream);
		exit(1);
	}

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(port);
	if(bind(listenFd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
	{
		logStream << "ERROR: Cannot bind to socket!" << endl;
		logIt(logStream);
		exit(1);
	}

	if(listen(listenFd, 64) < 0)
	{
		logStream << "ERROR: Cannot listen on socket!" << endl;
		logIt(logStream);
		exit(1);
	}

	for(hit=1; ;hit++)
	{
		length = sizeof(cli_addr);
		if((socketFd = accept(listenFd, (struct sockaddr *)&cli_addr, &length)) < 0)
		{
			logStream << "ERROR: Cannot accept socket!" << endl;
			logIt(logStream);
			exit(1);
		}

		if((pid = fork()) < 0)
		{
			logStream << "ERROR: Cannot fork!" << endl;
			logIt(logStream);
			exit(1);
		}
		else
		{
			if(pid == 0)
			{
				close(listenFd);
				web(socketFd, hit);
			}
			else
			{
				close(socketFd);
			}
		}
	}
}
