#ifndef _lib_mq_H_
#define _lib_mq_H_

#include <string>

namespace MessageBroker{
    class MessageQueue{
    public:
        MessageQueue();
        // Create a queue server with given port as it's argument
        void queue_server(int port);

        // Return a file descriptor to socket that provide's queue funcionality for producent
        int get_producent_queue_fd(char * ip, char * port, std::string queueName);

        // Return a file descriptor to socket that provide's queue funcionality for consumer
        int get_consumer_queue_fd(char * ip, char * port, std::string queueName);

        // Send given message through socket for producent's queue
        void send_message(std::string message, int sock);

        // Receive message from given socket for consumer queue
        std::string receive_message(int sock);
    };
}

#endif