#include "mq_impl.h"

//Server

//global structure to hold config
struct Config config;

static void readConfig(std::string filePath = "config.ini") {
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
			if (key == "holdingTime")
				config.holdingTime = std::stoi(value);
		}
	}
	else
	{
		printf("An error occured while reading config file!");
		exit(1);
	}
	configFile.close();
}


typedef std::map < std::string, std::unordered_set<int> > MapQueues;
typedef std::map < std::string, std::vector<message> > MapMessages;

// mutex
static std::mutex mutex;
// time
static time_t currentTime;

// server socket
static int servFd;

// undelivered messages
static MapMessages messagesQueues;

// client sockets
static MapQueues queues;


int server(int argc, char * argv){
	// get and validate port number
	if(argc != 2) error(1, 0, "Need 1 arg (port)");
	auto port = readPort(argv);
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

	// read configuration from config.ini file
	readConfig("config.ini");
	
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

			if (role == "consumer"){
				printf("Send overdue messages");
				for(message m : messagesQueues[queueName]){
					printf("%s %d %d\n", m.mess.c_str(), (int)m.time, int(time(& currentTime) - m.time));
					write(clientFd, m.mess.c_str(), m.mess.size());
				}
			}
			mutex.unlock();

			while (true)
			{		
				if (role == "producent"){
					// read a message
					char buffer[255];
					std::string receivedMessage;
					int count = read(clientFd, buffer, 255);
					// check if there are more bytes for full message
					if ( count > 0 ){
						receivedMessage += std::string(buffer, strlen(buffer));
						while ((count = recv(clientFd, buffer, 255, MSG_DONTWAIT)) > 0) {
							receivedMessage += std::string(buffer, count);
						}
					}
					if( receivedMessage.length() == 0 ) {
						printf("removing %d\n", clientFd);
						//clientFds.erase(clientFd);
						mutex.lock();
						queues[queueName].erase(clientFd);
						mutex.unlock();
						close(clientFd);
						break;
					} else {
						// broadcast the message
						mutex.lock();
						messagesQueues[queueName].push_back({receivedMessage, time( & currentTime)});
						mutex.unlock();
						sendToAllBut(clientFd, receivedMessage, receivedMessage.length(), queueName);
					}
				}
				else if (role == "consumer"){
					// read a message ( wait for EOF )
					char buffer[255];
					std::string receivedMessage;
					int count = read(clientFd, buffer, 255);
					// check if there are more bytes for full message
					if ( count > 0 ){
						receivedMessage += std::string(buffer, count);
						while ((count = recv(clientFd, buffer, 255, MSG_DONTWAIT)) > 0) {
							receivedMessage += std::string(buffer, count);
						}
					}
					if(receivedMessage.length() == 0) {
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
					close(clientFd);
					break;
				}
			}
		}).detach();
	}
/****************************/
}

static void monitorMessageQueue(){
	int position = 0;
	while (true)
	{
	mutex.lock();
	for(const auto& kv : messagesQueues){
		//printf("queue: %s \n", kv.first.c_str());
		bool empty = true;
		position = 0;
		for(message m : kv.second){
			empty = false;
			//printf("%s %d %d\n", m.mess.c_str(), (int)m.time, int(time(& currentTime) - m.time));
			if( time(& currentTime) - m.time > config.holdingTime ){
				position++;
			}
		}
		if( position != 0){
			messagesQueues[kv.first].erase(messagesQueues[kv.first].begin(), messagesQueues[kv.first].begin() + position );
			//printf("position: %d\n", position);
		}
		if( empty ) {
			if (queues[kv.first].empty()) {
				//drop a queue without messages and clients
				printf("Dropping queue %s\n", kv.first.c_str());
				queues.erase(queues.find(kv.first));
				messagesQueues.erase(messagesQueues.find(kv.first));
			}
		}

	}
	mutex.unlock();
	sleep(1);
	}	
}

static uint16_t readPort(char * txt){
	char * ptr;
	auto port = strtol(txt, &ptr, 10);
	if(*ptr!=0 || port<1 || (port>((1<<16)-1))) error(1,0,"illegal argument %s", txt);
	return port;
}

static void setReuseAddr(int sock){
	const int one = 1;
	int res = setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
	if(res) error(1,errno, "setsockopt failed");
}

static void ctrl_c(int){
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

static void sendToAllBut(int fd, std::string message, int count, std::string queueName){
	int res;
	std::unordered_set<int> bad;

	mutex.lock();
	for(int clientFd : queues[queueName]){
		if(clientFd == fd) continue;
		char cstr[count];
		message.copy(cstr, count);
		// char cstr[count+1];
		// message.copy(cstr, count + 1);
		// cstr[count] = '\n';
		//std::cout << std::endl << "Message: " << message.c_str() << " " << " count: " << count << std::endl;
		res = send(clientFd, cstr, count, MSG_DONTWAIT);
		if (res == -1) error(1, errno, "write failed on descriptor %d", fd);
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

// CLIENT

static ssize_t readData(int fd, char * buffer, ssize_t buffsize){
	auto ret = read(fd, buffer, buffsize);
	if(ret==-1) error(1,errno, "read failed on descriptor %d", fd);
	return ret;
}

static void writeData(int fd, char * buffer, ssize_t count){
	auto ret = write(fd, buffer, count);
	if(ret==-1) error(1, errno, "write failed on descriptor %d", fd);
	if(ret!=count) error(0, errno, "wrote less than requested to descriptor %d (%ld/%ld)", fd, count, ret);
}

static void sendMessage(int sock, std::string message){
	//writeData(sock, buffer2, received2);
	char cstr[message.size()];
	message.copy(cstr, message.size());
	// age.size()+1];
	// message.copy(cstr, message.size() + 1);
	// cstr[message.size()] = '\n';
	// writeData(sock, cstr, message.size() + 1);
	writeData(sock, cstr, message.size());
}

static int connect(char * ip, char * port, std::string role, std::string queue){
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

int client(char * ip, char * port, std::string role, std::string queue){
	// queue's name can't be longer than 240 characters
	if ( queue.length() > 240 ) error(1,0,"Queue's name can not be longer than 240 characters!");

	readConfig("config.ini");

	int sock = connect(ip, port, role, queue); 

	return sock;
}
	// //	Prepare ultra long message
	// std::string ultraLongMessage;
	// while (ultraLongMessage.length() < config.producentMessageLength)
	// 	ultraLongMessage += 'a';
	
	// int i = 0;
void client_send(std::string message, int sock){

	// write to socket
	// message = "Message! " + queue + " " + std::to_string(i) + " ";

	// message += ultraLongMessage.substr(0, (config.producentMessageLength) - message.length());
	sendMessage(sock, message);
	int msgLength = message.length();
	printf("Message sent, message length = %i\n", msgLength);
}

std::string client_receive(int sock){
	// read from socket, write to stdout
	std::string receivedMessage;
	char buffer1[255];

	int count = read(sock, buffer1, 255);
	if(count == -1) error(1,errno, "read failed on descriptor %d", sock);
	if ( count > 0 ){
		receivedMessage += std::string(buffer1, count);
		while ((count = recv(sock, buffer1, 255, MSG_DONTWAIT)) > 0) {
			receivedMessage += std::string(buffer1, count);
		}
	}
	
	
	return receivedMessage;
}

