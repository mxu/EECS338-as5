#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

/* data structures ************************************************************/
union semun {
    int val;
    struct semid_ds *buf;
    ushort * array;
    struct seminfo *__buf;
};

struct threadInfo {
    int tid;
}

/* implementation *************************************************************/

int main(int argc, char *argv[]) {
    pthread_t tid;
    pthread_attr_t attr;
    struct threadInfo info;
    void *exit_status;

    printf("Main\n");
    
    c_pthread_create(&tid, &attr, hydrogen, (void *) &info);
    c_pthread_join(tid, &exit_status);
    return EXIT_SUCCESS;
}

/* hydrogen thread ************************************************************/
void *hydrogen(void *tid) {
    struct threadInfo *info;
    int id;

    info = (struct threadInfo *) tid;
    id = info->tid;

    printf("Thread %d\n", id);

    pthread_exit(NULL);
}

/* oxygen thread **************************************************************/

/* wrapper functions for system calls to handle errors ************************/
void c_pthread_create(pthread_t *thread, const pthread_attr_t *attr,
                      void *(*start_routine) (void *), void *arg) {
    if(pthread_create(thread, attr, start_routine, arg) < 0) {
        perror("pthread_create");
        exit(EXIT_FAILURE);
    }
}

void c_pthread_join(pthread_t thread, void **retval) {
    if(ptheard_join(thread, retval) < 0) {
        perror("pthread_join");
        exit(EXIT_FAILURE);
    }
}