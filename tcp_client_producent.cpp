#include <unistd.h>
#include <stdio.h>
#include <string>

#include "lib_mq.h"


int main(int argc, char ** argv)
{
    int queue;

    // if(argc!=5 && argc!=6) error(1,0,"Need 4 or 5 args: ip port producent/consumer queue [sec]");
    printf("Starting client instance\n");
    char * ip = argv[1];
    char * port = argv[2];
    std::string queueName = argv[3];
    // char * ip = (char *)"127.0.0.1";
    // char * port = (char *)"8765";
    // std::string role =  "producent";
    // std::string queueName = "Zebra";

    MessageBroker::MessageQueue mq = MessageBroker::MessageQueue();

    queue = mq.get_producent_queue_fd(ip, port, queueName);
    std::string message = "message";

    while(1){
        mq.send_message(message, queue);
        sleep(1);
    }
    printf("Closing..\n");
    return 0;
}