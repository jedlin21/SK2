#include <unistd.h>
#include <stdio.h>
#include <iostream>

#include "lib_mq.h"


int main(int argc, char ** argv)
{
    int queue;
    std::string receivedMessage;

    // if(argc!=5 && argc!=6) error(1,0,"Need 3 args: ip port queueName");
    printf("Starting client instance\n");
    char * ip = argv[1];
    char * port = argv[2];
    std::string queueName = argv[3];
    // char * ip = (char *)"127.0.0.1";
    // char * port = (char *)"8765";
    // std::string queueName = "Zebra";

    MessageBroker::MessageQueue mq = MessageBroker::MessageQueue();

    queue = mq.get_consumer_queue_fd(ip, port, queueName);

    while(1){
        receivedMessage = mq.receive_message(queue);
        std::cout << "TEST " << receivedMessage << " ||| Length = " << receivedMessage.length() << std::endl;
    }
    printf("Closing..\n");
    return 0;
}