/*

nzServ.cpp
Noah Betzen

A simple web server in C++.

Compile with:
g++ nzServ.cpp -o nzServ.out

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

void web(int fd, int hit)
{
	int j;
	int file_fd;
	int buflen;
	int len;
	long i;
	long ret;
	char *fstr;
	static char buffer[BUFFERSIZE+1];

	ret =read(fd, buffer, BUFFERSIZE);

	if(ret == 0 || ret == -1)
	{
		cout << "ERROR: Failed to read browser request!" << endl;
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

	cout << "LOG: Request: " << buffer << " " << hit << endl;

	if(strncmp(buffer,"GET ",4) && strncmp(buffer,"get ",4))
	{
		cout << "ERROR: Can only handle GET requests!" << endl;
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
			cout << "ERROR: . and .. paths not supported." << endl;
			exit(1);
		}
	}

	if(!strncmp(&buffer[0], "GET /\0", 6) || !strncmp(&buffer[0], "get /\0", 6))
	{
		strcpy(buffer,"GET /index.html");
	}

	buflen = strlen(buffer);
	fstr = (char *)0;

	for(i = 0; mimeTypes[i].extension != 0; i++)
	{
		len = strlen(mimeTypes[i].extension);

		if(!strncmp(&buffer[buflen-len], mimeTypes[i].extension, len))
		{
			fstr = mimeTypes[i].fileType;
			break;
		}
	}
	if(fstr == 0)
	{
		cout << "ERROR: File extension not supported!" << endl;
		exit(1);
	}

	if((file_fd = open(&buffer[5],O_RDONLY)) == -1)
	{
		cout << "ERROR: Failed to open file!" << endl;
		exit(1);
	}

	cout << "LOG: Send: " << &buffer[5] << " " << hit << endl;

	sprintf(buffer,"HTTP/1.0 200 OK\r\nContent-Type: %s\r\n\r\n", fstr);
	write(fd,buffer,strlen(buffer));

	while ((ret = read(file_fd, buffer, BUFFERSIZE)) > 0)
	{
		write(fd,buffer,ret);
	}

	sleep(1);
	exit(1);
}

int main(int argc, char *argv[])
{
	int i, port, pid, listenfd, socketfd, hit;
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

	cout << "LOG: Web server starting: " << argv[1] << " " << getpid() << endl;

	if((listenfd = socket(AF_INET, SOCK_STREAM,0)) < 0)
	{
		cout << "ERROR: Cannot create socket!" << endl;
		exit(1);
	}

	port = atoi(argv[1]);

	if(port < 0 || port > 65535)
	{
		cout << "ERROR: Invalid port number, try 1 through 65535! Port: " << argv[1] << endl;
		exit(1);
	}

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(port);
	if(bind(listenfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
	{
		cout << "ERROR: Cannot bind to socket!" << endl;
		exit(1);
	}

	if(listen(listenfd, 64) < 0)
	{
		cout << "ERROR: Cannot listen on socket!" << endl;
		exit(1);
	}

	for(hit=1; ;hit++)
	{
		length = sizeof(cli_addr);
		if((socketfd = accept(listenfd, (struct sockaddr *)&cli_addr, &length)) < 0)
		{
			cout << "ERROR: Cannot accept socket!" << endl;
			exit(1);
		}

		if((pid = fork()) < 0)
		{
			cout << "ERROR: Cannot fork!" << endl;
			exit(1);
		}
		else
		{
			if(pid == 0)
			{
				close(listenfd);
				web(socketfd, hit);
			}
			else
			{
				close(socketfd);
			}
		}
	}
}
