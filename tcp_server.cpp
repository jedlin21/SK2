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
#include <mutex>
#include <unordered_set>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <map>
#include <vector> 
#include <ctime>

int HOLDING_TIME = 5;

typedef std::map < std::string, std::unordered_set<int> > MapQueues;
struct message {
    std::string mess;
    time_t time;
};
typedef std::map < std::string, std::vector<message> > MapMessages;

// mutex
std::mutex mutex;
// time
time_t currentTime;

// server socket
int servFd;

// undelivered messages
MapMessages messagesQueues;

// client sockets
MapQueues queues;

// handles SIGINT
void ctrl_c(int);

// sends data to clientFds excluding fd
void sendToAllBut(int fd, char * buffer, int count, std::string queueName);

// converts cstring to port
uint16_t readPort(char * txt);

// sets SO_REUSEADDR
void setReuseAddr(int sock);

void monitorMessageQueue();

int main(int argc, char ** argv){
	// get and validate port number
	if(argc != 2) error(1, 0, "Need 1 arg (port)");
	auto port = readPort(argv[1]);
	std::thread (monitorMessageQueue).detach();
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
			mutex.lock();
			MapQueues::iterator it = queues.find(queueName);
			if(it == queues.end()){
				std::unordered_set<int> clientSet;
				queues.insert( MapQueues::value_type(queueName, clientSet) );
			}
			queues[queueName].insert(clientFd);
			mutex.unlock();

			mutex.lock();
			if (strcmp(role.c_str(), "consumer") == 0){
				printf("Send overdue messages");
				for(message m : messagesQueues[queueName]){
					printf("%s %d %d\n", m.mess.c_str(), (int)m.time, int(time(& currentTime) - m.time));
					write(clientFd, m.mess.c_str(), m.mess.size());
				}
			}
			mutex.unlock();

			while (true)
			{		
				if (strcmp(role.c_str(), "producent") == 0){
					// read a message
					char buffer[255];
					int count = read(clientFd, buffer, 255);
					if(count < 1) {
						printf("removing %d\n", clientFd);
						//clientFds.erase(clientFd);
						close(clientFd);
						break;
					} else {
						// broadcast the message
						mutex.lock();
						messagesQueues[queueName].push_back({std::string (buffer, count), time( & currentTime)});
						mutex.unlock();
						sendToAllBut(clientFd, buffer, count, queueName);
					}
				}
				else if (strcmp(role.c_str(), "consumer") == 0){
					// read a message
					char buffer[255];
					int count = read(clientFd, buffer, 255);
					
					if(count < 1) {
						printf("removing %d\n", clientFd);
						mutex.lock();
						queues[queueName].erase(clientFd);
						mutex.unlock();
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

void monitorMessageQueue(){
	int position = 0;
	while (true)
	{
	mutex.lock();
	for(const auto& kv : messagesQueues){
		//printf("queue: %s \n", kv.first.c_str());
		position = 0;
		for(message m : kv.second){
			//printf("%s %d %d\n", m.mess.c_str(), (int)m.time, int(time(& currentTime) - m.time));
			if( time(& currentTime) - m.time > HOLDING_TIME ){
				position++;
			}
		}
		if( position != 0){
			messagesQueues[kv.first].erase(messagesQueues[kv.first].begin(), messagesQueues[kv.first].begin() + position );
			//printf("position: %d\n", position);
		}
	}
	mutex.unlock();
	sleep(1);
	}	
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
	mutex.lock();
	for(const auto& kv : queues){
		for(int clientFd : kv.second){
			close(clientFd);
			printf("%d closed \n", clientFd);
		}
	}
	mutex.unlock();
	close(servFd);
	printf("Closing server\n");
	exit(0);
}

void sendToAllBut(int fd, char * buffer, int count, std::string queueName){
	int res;
	std::unordered_set<int> bad;

	mutex.lock();
	for(int clientFd : queues[queueName]){
		if(clientFd == fd) continue;
		res = write(clientFd, buffer, count);
		if(res!=count)
			bad.insert(clientFd);
	}
	for(int clientFd : bad){
		printf("removing %d\n", clientFd);
		queues[queueName].erase(clientFd);
		close(clientFd);
	}
	mutex.unlock();
}