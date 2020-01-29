/*
    MQ library
    Maciej Leszczyk 136759
    Patryk Jedlikowski 136723
    Informatyka, I2, semestr 5
*/

#ifndef _mq_H_
#define _mq_H_

#include <cstdlib>
#include <cstdio>
#include <cstring>
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

#include <fstream>
#include <iostream>
#include <algorithm>

// Server
struct Config {
	// number of seconds after messages are terminated
	int holdingTime;
	// length of message which producent is sending through queue
	int producentMessageLength;
};

void readConfig(std::string filePath);

struct message {
    std::string mess;
    time_t time;
};

// handles SIGINT
void ctrl_c(int);

// sends data to clientFds excluding fd
void sendToAllBut(int fd, std::string message, int count, std::string queueName);

// converts cstring to port
uint16_t readPort(char * txt);

// sets SO_REUSEADDR
void setReuseAddr(int sock);

void monitorMessageQueue();

int server(int argc, char ** argv);


// Client
ssize_t readData(int fd, char * buffer, ssize_t buffsize);

void writeData(int fd, char * buffer, ssize_t count);

void sendMessage(int sock, std::string message);

int connect(char * ip, char * port, std::string role, std::string queue);

int client(char * ip, char * port, std::string role, std::string queue);

void client_send(std::string message, int sock);

std::string client_receive(int sock);

#endif