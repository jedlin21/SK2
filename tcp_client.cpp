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

#include <fstream>
#include <iostream>

//global structure to hold config
struct Config {
	// number of seconds after messages are terminated
	int holdingTime;
	// length of message which producent is sending through queue
	int producentMessageLength;
} config;

void readConfig(std::string filePath = "config.ini") {
	std::ifstream configFile;
	configFile.open(filePath);
	if (configFile.is_open())
	{
		std::string line;
		std::string key;
		std::string value;
		while( std::getline(configFile, line))
		{
			size_t pos = 0;
			std::string token;
			while ((pos = line.find('=')) != std::string::npos) {
				token = line.substr(0, pos);
				key = token;
				line.erase(0, pos + 1);
			}
			value = line;
			if (key == "producentMessageLength")
				config.producentMessageLength = std::stoi(value);
		}
	}
	else
	{
		printf("An error occured while reading config file!");
		exit(1);
	}
	configFile.close();
}

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

	// queue's name can't be longer than 240 characters
	if ( queue.length() > 240 ) error(1,0,"Queue's name can not be longer than 240 characters!");

	if (strcmp(role.c_str(), "producent") == 0 && argc !=6) error(1,0,"Need 5 args: ip port producent queue sec");

	readConfig();

	int sock = connect(argv[1], argv[2], role, queue); 

	//	Prepare ultra long message
	std::string ultraLongMessage;
	while (ultraLongMessage.length() < config.producentMessageLength)
		ultraLongMessage += 'a';
	
	int i = 0;
	std::string message;
	while (true){
		if (strcmp(role.c_str(), "producent") == 0){
			
			// write to socket
			message = "Message! " + queue + " " + std::to_string(i) + " ";

			message += ultraLongMessage.substr(0, (config.producentMessageLength) - message.length());
			i += 1;
			sendMessage(sock, message);
			int msgLength = message.length();
			printf("Message sent, message length = %i\n", msgLength);
			sleep(atoi(argv[5]));
		}
		else if (strcmp(role.c_str(), "consumer") == 0){
			// read from socket, write to stdout
			std::string receivedMessage;
			char buffer1[255];

			int count = read(sock, buffer1, 255);
			if(count == -1) error(1,errno, "read failed on descriptor %d", sock);
			if ( count > 0 ){
				receivedMessage += std::string(buffer1, strlen(buffer1));
				while ((count = recv(sock, buffer1, 255, MSG_DONTWAIT)) > 0) {
					receivedMessage += std::string(buffer1, count);
				}
			}

			sendMessage(1, receivedMessage);
			printf("Received message. Length = %i\n", (int)(receivedMessage.length()));
		}
	}
/****************************/
	
	close(sock);
	
	return 0;
}
