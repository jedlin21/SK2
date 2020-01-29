#include "mq.h"

int main(int argc, char ** argv)
{
    printf("Server wake up!\n");
    server(argc, argv);
    printf("Server take off..\n");
    return 0;
}