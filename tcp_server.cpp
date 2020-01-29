#include "lib_mq.h"
#include <iostream>

int main(int argc, char ** argv)
{
    std::cout << "Server wake up!\n";

    MessageBroker::MessageQueue mq = MessageBroker::MessageQueue();
    // FOR EXAMPLE PORT 8888
    int port = 8888;
    mq.queue_server(port);

    std::cout << "Server take off..\n";
    return 0;
}