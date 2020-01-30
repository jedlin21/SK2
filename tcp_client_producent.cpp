#include <unistd.h>
#include <stdio.h>
#include <string>
#include <iostream>

#include "lib_mq.h"


int main(int argc, char ** argv)
{
    int queue;

    printf("Starting client instance\n");
    char * ip = argv[1];
    char * port = argv[2];
    std::string queueName = argv[3];
    // char * ip = (char *)"127.0.0.1";
    // char * port = (char *)"8765";
    // std::string queueName = "Zebra";

    MessageBroker::MessageQueue mq = MessageBroker::MessageQueue();

    queue = mq.get_producent_queue_fd(ip, port, queueName);
    std::string base = "message";

    int i = 0;
    std::string message;
    while(1){
        message = base + std::to_string(i) + "\n";
        std::cout << std::endl << message << std::endl;
        mq.send_message(message, queue);
        sleep(1);
        i += 1;
    }
    printf("Closing..\n");
    return 0;
}