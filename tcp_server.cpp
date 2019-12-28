#include <cstdlib>
#include <cstdio>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <error.h>
#include <netdb.h>
#include <sys/epoll.h>
#include <poll.h> 
#include <thread>
#include <unordered_set>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <map>

typedef std::map < std::string, std::unordered_set<int> > Map;

// server socket
int servFd;

// client sockets
std::unordered_set<int> clientFds;

Map queues;

// handles SIGINT
void ctrl_c(int);

// sends data to clientFds excluding fd
void sendToAllBut(int fd, char * buffer, int count, std::string queueName);

// converts cstring to port
uint16_t readPort(char * txt);

// sets SO_REUSEADDR
void setReuseAddr(int sock);

int main(int argc, char ** argv){
	// get and validate port number
	if(argc != 2) error(1, 0, "Need 1 arg (port)");
	auto port = readPort(argv[1]);
	
	// create socket
	servFd = socket(AF_INET, SOCK_STREAM, 0);
	if(servFd == -1) error(1, errno, "socket failed");
	
	// graceful ctrl+c exit
	signal(SIGINT, ctrl_c);
	// prevent dead sockets from throwing pipe errors on write
	signal(SIGPIPE, SIG_IGN);
	
	setReuseAddr(servFd);
	
	// bind to any address and port provided in arguments
	sockaddr_in serverAddr{.sin_family=AF_INET, .sin_port=htons((short)port), .sin_addr={INADDR_ANY}};
	int res = bind(servFd, (sockaddr*) &serverAddr, sizeof(serverAddr));
	if(res) error(1, errno, "bind failed");
	
	// enter listening mode
	res = listen(servFd, 1);
	if(res) error(1, errno, "listen failed");
	
/****************************/
	
	while(true){
		// prepare placeholders for client address
		sockaddr_in clientAddr{0};
		socklen_t clientAddrSize = sizeof(clientAddr);
		
		// accept new connection
		auto clientFd = accept(servFd, (sockaddr*) &clientAddr, &clientAddrSize);
		if(clientFd == -1) error(1, errno, "accept failed");
		
		// tell who has connected
		printf("new connection from: %s:%hu (fd: %d)\n", inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port), clientFd);
		
/****************************/
		printf("Start new thread \n");
		std::thread ( [clientFd] {
			std::string role, queueName;
			char buffer[255];
			int count = read(clientFd, buffer, 255);
			// Check for correctness
			if (count > 0){
				std::string registerData(buffer, count);
				printf("%s \n", registerData.c_str());

				std::size_t pos = registerData.find(";");
				role = registerData.substr(0, pos);
				queueName = registerData.substr(pos+1);
				printf("role: %s   queue: %s", role.c_str(), queueName.c_str());
				//Send response to client to confirm connection
				write(clientFd, "OK", 3);
			}
			else{
				// if message is empty, then sth go wrong
				printf("An error ocured  \n");
				// TODO: exit from thread
			}

			// add client to clients set
			// if queue does not exist create it
			Map::iterator it = queues.find(queueName);
			if(it == queues.end()){
				std::unordered_set<int> clientSet;
				queues.insert( Map::value_type(queueName, clientSet) );
			}
			queues[queueName].insert(clientFd);
		
			while (true)
			{		
				if (strcmp(role.c_str(), "producent") == 0){
					// read a message
					char buffer[255];
					int count = read(clientFd, buffer, 255);
					if(count < 1) {
						printf("removing %d\n", clientFd);
						clientFds.erase(clientFd);
						close(clientFd);
						break;
					} else {
						// broadcast the message
						sendToAllBut(clientFd, buffer, count, queueName);
					}
				}
				else if (strcmp(role.c_str(), "consumer") == 0){
					// read a message
					char buffer[255];
					int count = read(clientFd, buffer, 255);
					
					if(count < 1) {
						printf("removing %d\n", clientFd);
						clientFds.erase(clientFd);
						close(clientFd);
						break;
					} 
				}
				else{
					printf("unknow role: %s\n exit \n", role.c_str());
					break;
				}
			}
		}).detach();
	}
/****************************/
}

uint16_t readPort(char * txt){
	char * ptr;
	auto port = strtol(txt, &ptr, 10);
	if(*ptr!=0 || port<1 || (port>((1<<16)-1))) error(1,0,"illegal argument %s", txt);
	return port;
}

void setReuseAddr(int sock){
	const int one = 1;
	int res = setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
	if(res) error(1,errno, "setsockopt failed");
}

void ctrl_c(int){
	for(int clientFd : clientFds)
		close(clientFd);
	close(servFd);
	printf("Closing server\n");
	exit(0);
}

void sendToAllBut(int fd, char * buffer, int count, std::string queueName){
	int res;
	std::unordered_set<int> bad;
	for(int clientFd : queues[queueName]){
		if(clientFd == fd) continue;
		res = write(clientFd, buffer, count);
		if(res!=count)
			bad.insert(clientFd);
	}
	for(int clientFd : bad){
		printf("removing %d\n", clientFd);
		clientFds.erase(clientFd);
		close(clientFd);
	}
}