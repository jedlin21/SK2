#include "lib_mq.h"
#include "mq_impl.h"


MessageBroker::MessageQueue::MessageQueue(){}

void MessageBroker::MessageQueue::queue_server(int port){
    int argc = 2;
    char *argv;
    sprintf(argv, "%d", port);
    server(argc, argv);
}

int MessageBroker::MessageQueue::get_producent_queue_fd(char * ip, char * port, std::string queueName){
    std::string role = "producent";
    return client(ip, port, role, queueName);
}

int MessageBroker::MessageQueue::get_consumer_queue_fd(char * ip, char * port, std::string queueName){
    std::string role = "consumer";
    return client(ip, port, role, queueName);
}

void MessageBroker::MessageQueue::send_message(std::string message, int sock){
    client_send(message, sock);
}

std::string MessageBroker::MessageQueue::receive_message(int sock){
    std::string message;
    message = client_receive(sock);
    return message;
}

int main(){
	return 0;
}