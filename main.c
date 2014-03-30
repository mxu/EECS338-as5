#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

union semun {
    int val;
    struct semid_ds *buf;
    ushort * array;
    struct seminfo *__buf;
};

int main(int argc, char *argv[]) {
    thread_t tid;
    
    printf("Main\n");
    return EXIT_SUCCESS;
}
