#include "mq.h"
#include <unistd.h>
#include <stdio.h>


int main(int argc, char ** argv)
{
    int queue;
    std::string receivedMessage;

    // if(argc!=5 && argc!=6) error(1,0,"Need 4 or 5 args: ip port producent/consumer queue [sec]");
    printf("Starting client instance\n");
    char * ip = argv[1];
    char * port = argv[2];
    std::string role =  argv[3];
    std::string queueName = argv[4];
    // char * ip = (char *)"127.0.0.1";
    // char * port = (char *)"8765";
    // std::string role =  "consumer";
    // std::string queueName = "Zebra";

    queue = client(ip, port, role, queueName);
    std::string message = "message";

    while(1){
        receivedMessage = client_receive(queue);
       
        if((int)(receivedMessage.length() > 0)){
            sendMessage(1, receivedMessage);
            printf("Received message. Length = %i\n", (int)(receivedMessage.length()));
	    }
    }
    printf("Closing..\n");
    return 0;
}