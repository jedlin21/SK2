/*
    MQ library
    Maciej Leszczyk 136759
    Patryk Jedlikowski 136723
    Informatyka, I2, semestr 5
*/

#ifndef _mq_impl_H_
#define _mq_impl_H_


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
#include <string>
#include <algorithm>

// Server
struct Config {
	// number of seconds after messages are terminated
	int holdingTime;
	// length of message which producent is sending through queue
	int producentMessageLength;
};

static void readConfig(std::string filePath);

struct message {
    std::string mess;
    time_t time;
};

// handles SIGINT
static void ctrl_c(int);

// sends data to clientFds excluding fd
static void sendToAllBut(int fd, std::string message, int count, std::string queueName);

// converts cstring to port
static uint16_t readPort(char * txt);

// sets SO_REUSEADDR
static void setReuseAddr(int sock);

static void monitorMessageQueue();

int server(int argc, int argv);


// Client
static ssize_t readData(int fd, char * buffer, ssize_t buffsize);

static void writeData(int fd, char * buffer, ssize_t count);

static void sendMessage(int sock, std::string message);

static int connect(char * ip, char * port, std::string role, std::string queue);

int client(char * ip, char * port, std::string role, std::string queue);

void client_send(std::string message, int sock);

std::string client_receive(int sock);


#endif