#include <cstdlib>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <error.h>
#include <netdb.h>
#include <sys/epoll.h>
#include <poll.h> 
#include <thread>
#include <stdio.h>
#include <string.h>

ssize_t readData(int fd, char * buffer, ssize_t buffsize){
	auto ret = read(fd, buffer, buffsize);
	if(ret==-1) error(1,errno, "read failed on descriptor %d", fd);
	return ret;
}

void writeData(int fd, char * buffer, ssize_t count){
	auto ret = write(fd, buffer, count);
	if(ret==-1) error(1, errno, "write failed on descriptor %d", fd);
	if(ret!=count) error(0, errno, "wrote less than requested to descriptor %d (%ld/%ld)", fd, count, ret);
}

void sendMessage(int sock, std::string message){
	//writeData(sock, buffer2, received2);
	char cstr[message.size()+1];
	message.copy(cstr, message.size() + 1);
	cstr[message.size()] = '\n';
	writeData(sock, cstr, message.size() + 1);
}

int connect(char * ip, char * port, std::string role, std::string queue){
	// Resolve arguments to IPv4 address with a port number
	addrinfo *resolved, hints={.ai_flags=0, .ai_family=AF_INET, .ai_socktype=SOCK_STREAM};
	int res = getaddrinfo(ip, port, &hints, &resolved);
	if(res || !resolved) error(1, 0, "getaddrinfo: %s", gai_strerror(res));
	
	// create socket
	int sock = socket(resolved->ai_family, resolved->ai_socktype, 0);
	if(sock == -1) error(1, errno, "socket failed");
	
	// attept to connect
	res = connect(sock, resolved->ai_addr, resolved->ai_addrlen);
	if(res) error(1, errno, "connect failed");
	
	// free memory
	freeaddrinfo(resolved);

	// Notice server about client role and queue
	sendMessage(sock, role+";"+queue);

	//Check if server response "OK"
	char buffer1[255];
	ssize_t received = readData(sock, buffer1, 255);
	std::string response(buffer1, received);
	printf("Server response: %s\n", response.c_str());
	if (strcmp(response.c_str(), "OK") == 0){
		return sock;
	}
	else{
		printf("Connection error\n");
		exit(1);
	}
}

int main(int argc, char ** argv){
	if(argc!=5 && argc!=6) error(1,0,"Need 4 or 5 args: ip port producent/consumer queue [sec]");
	std::string role = argv[3];
	std::string queue = argv[4];

	int sock = connect(argv[1], argv[2], role, queue); 
	
	if (strcmp(role.c_str(), "producent") == 0 && argc !=6) error(1,0,"Need 5 args: ip port producent queue sec");
	
	int i = 0;
	std::string message;
	while (true){
		if (strcmp(role.c_str(), "producent") == 0){
			// write to socket
			message = "Message! " + queue + " " + std::to_string(i);
			i += 1;
			sendMessage(sock, message);
			printf("Message sent \n");
			sleep(atoi(argv[5]));
		}
		else if (strcmp(role.c_str(), "consumer") == 0){
			// read from socket, write to stdout
			ssize_t bufsize1 = 255, received1;
			char buffer1[bufsize1];
			received1 = readData(sock, buffer1, bufsize1);
			writeData(1, buffer1, received1);
		}
	}
/****************************/
	
	close(sock);
	
	return 0;
}
