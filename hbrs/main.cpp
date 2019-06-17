#include <stdlib.h>
#include <stdio.h>
#include <thread>
#include <unistd.h>

int main(int argc,char **argv)
{
    printf("test\n");

    std::thread([](){
        printf("hello 112312\n");
    }).detach();

    sleep(1);
    return 0;    
}