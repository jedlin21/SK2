#include "mq.h"

int main(int argc, char ** argv)
{
    printf("Starting client instance\n");
    client(argc, argv);
    printf("Closing..\n");
    return 0;
}