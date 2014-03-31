/* Sequential H2O Production Problem
 * Mike Xu
 * EECS 338
 * 3-30-2014
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <time.h>

#define SEM_MUTEX       0           /* mutex to lock shared variables */
#define SEM_H           1           /* hydrogen lock */
#define SEM_O           2           /* oxygen lock */
#define SEM_B           3           /* barrier lock */
#define SEM_SIZE        4           /* number of semaphores */
#define NUM_H           40          /* total number of hydrogen threads to spawn */
#define NUM_O           20          /* total number of oxygen threads to spawn */

/* data structures ************************************************************/
union semun {
    int                 val;        /* value for SETVAL */
    struct semid_ds     *buf;       /* buffer for IPC_STAT and IPC_SET */
    ushort              *array;     /* array for GETALL, SETALL */
    struct seminfo      *__buf;     /* Linux buffer for IPC_INFO */
};

/* function declarations ******************************************************/
void *hydrogen(void *id);
void *oxygen(void *id);
void c_semget(int *semid);
void semWait(int semid, int sem);
void semSignal(int semid, int sem);
void c_pthread_create(pthread_t *thread, const pthread_attr_t *attr,
                      void *(*start_routine) (void *), void *arg);
void c_pthread_join(pthread_t thread, void **retval);
time_t c_time(time_t *t);
void printHeader();
void printInfo(char *pname, int tid, char *msg);

/* implementation *************************************************************/
int semid,                          /* semaphore block id */
    hCount,                         /* number of hydrogens at barrier */
    oCount,                         /* number of oxygens at barrier */
    bCount;                         /* number of atoms crossing */

int main(int argc, char *argv[]) {
    pthread_t       h[NUM_H];       /* array of hydrogen threads */
    pthread_t       o[NUM_O];       /* array of oxygen threads */
    pthread_attr_t  attr;           /* thread attribute data */
    union semun     arg;            /* semaphore union */
    ushort          sem[SEM_SIZE];  /* array for storing semaphore values */
    int             hSpawned,       /* number of hydrogen threads spawned */
                    oSpawned;       /* number of oxygen threads spawned */
    int             *targ;          /* thread id argument */
    char            buf[64];        /* buffer for writing strings */
    
    printHeader();

    /* initialize semaphore */
    printInfo("Main", 0, "Initializing semaphore");
    c_semget(&semid);
    sem[SEM_MUTEX]  = 1;
    sem[SEM_H]      = 0;
    sem[SEM_O]      = 0;
    sem[SEM_B]      = 1;
    arg.array = sem;
    if(semctl(semid, 0, SETALL, arg) < 0) {
        perror("semctl(SETALL)");
        exit(EXIT_FAILURE);
    }

    /* initialize variables */
    printInfo("Main", 0, "Initializing variables");
    hCount = 0;
    oCount = 0;
    bCount = 0;
    hSpawned = 0;
    oSpawned = 0;

    /* initialize default thread attributes */
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    /* spawn hydrogen threads */
    while(hSpawned < NUM_H) {
        sprintf(buf, "Spawning hydrogen %d", hSpawned);
        printInfo("Main", 0, buf);
        targ = malloc(sizeof(*targ));
        if(targ == NULL) {
            perror("malloc");
            exit(EXIT_FAILURE);
        }
        *targ = hSpawned;
        c_pthread_create(&h[hSpawned], &attr, hydrogen, targ);
        hSpawned++;
    }

    /* spawn oxygen thread */
    while(oSpawned < NUM_O) {
        sprintf(buf, "Spawning oxygen %d", oSpawned);
        printInfo("Main", 0, buf);
        targ = malloc(sizeof(*targ));
        if(targ == NULL) {
            perror("malloc");
            exit(EXIT_FAILURE);
        }
        *targ = oSpawned;
        c_pthread_create(&o[oSpawned], &attr, oxygen, targ);
        oSpawned++;
    }

    printInfo("Main", 0, "Done spawning");
    
    /* terminate threads */
    pthread_attr_destroy(&attr);
    while(--hSpawned >= 0) c_pthread_join(h[hSpawned], NULL);
    while(--oSpawned >= 0) c_pthread_join(o[oSpawned], NULL);

    /* release semaphore */
    if(semctl(semid, 0, IPC_RMID) == -1) { perror("semctl"); }
    printInfo("Main", 0, "Terminating");
    return EXIT_SUCCESS;
}

/* hydrogen thread ************************************************************/
void *hydrogen(void *id) {
    int tid = *((int *) id);
    free(id);
    semWait(semid, SEM_MUTEX);
    if(oCount >= 1 && hCount >= 1) {    
        /* if at least 1 H and 1 O are available, invoke bond */
        bCount = 2;
        printInfo("Hydrogen", tid, "Invoking bond");
        semSignal(semid, SEM_O);
        semSignal(semid, SEM_H);
        printInfo("Hydrogen", tid, "Destroyed");
        pthread_exit(NULL);
    } else {
        /* otherwise, release mutex, and wait for bond invocation */
        hCount++;
        semSignal(semid, SEM_MUTEX);
        printInfo("Hydrogen", tid, "Wait for bond");
        semWait(semid, SEM_H);
        semWait(semid, SEM_B);
        /* barrier passage synchronization */
        printInfo("Hydrogen", tid, "Bond invoked");
        hCount--;
        bCount--;
        if(bCount != 0) {
            semSignal(semid, SEM_B);
            printInfo("Hydrogen", tid, "Destroyed");
            pthread_exit(NULL);
        } else {
            /* crossed the barrier */
            semSignal(semid, SEM_B);
            semSignal(semid, SEM_MUTEX);
            printInfo("Hydrogen", tid, "Crossed the barrier");
        }
    }
    printInfo("Hydrogen", tid, "has become H2O");
    pthread_exit(NULL);
}

/* oxygen thread **************************************************************/
void *oxygen(void *id) {
    int tid = *((int *) id);
    free(id);
    semWait(semid, SEM_MUTEX);
    if(hCount >= 2) {
        /* if at least 2 H are available, invoke bond */
        bCount = 2;
        printInfo("Oxygen", tid, "Invoking bond");
        semSignal(semid, SEM_H);
        semSignal(semid, SEM_H);
        printInfo("Oxygen", tid, "Destroyed");
        pthread_exit(NULL);
    } else {
        /* otherwise, release mutex, and wait for bond invocation */
        oCount++;
        semSignal(semid, SEM_MUTEX);
        printInfo("Oxygen", tid, "Wait for bond");
        semWait(semid, SEM_O);
        semWait(semid, SEM_B);
        /* barrier passage synchronization */
        printInfo("Oxygen", tid, "Bond invoked");
        oCount--;
        bCount--;
        if(bCount != 0) {
            semSignal(semid, SEM_B);
            printInfo("Oxygen", tid, "-X");
            pthread_exit(NULL);
        } else {
            /* crossed the barrier */
            semSignal(semid, SEM_B);
            semSignal(semid, SEM_MUTEX);
            printInfo("Oxygen", tid, "Crossed the barrier");
        }
        
    }
    printInfo("Oxygen", tid, "has become H2O");
    pthread_exit(NULL);
}

/* wrapper functions for system calls to handle errors ************************/
void c_semget(int *semid) {
    key_t key;
    /* generate unique key from filepath */
    if((key = ftok(".", 0)) == (key_t) -1) {
        perror("ftok");
        exit(EXIT_FAILURE);
    }
    /* get semaphore block */
    *semid = semget(key, SEM_SIZE, IPC_CREAT | IPC_EXCL | 0666);
    if(*semid < 0) {
        perror("semget");
        exit(EXIT_FAILURE);
    }
}

void semWait(int semid, int sem) {
    struct sembuf buf;
    buf.sem_op = -1;
    buf.sem_flg = 0;
    buf.sem_num = sem;
    semop(semid, &buf, 1);
}

void semSignal(int semid, int sem) {
    struct sembuf buf;
    buf.sem_op = 1;
    buf.sem_flg = 0;
    buf.sem_num = sem;
    semop(semid, &buf, 1);
}

void c_pthread_create(pthread_t *thread, const pthread_attr_t *attr,
                      void *(*start_routine) (void *), void *arg) {
    if(pthread_create(thread, attr, start_routine, arg) == -1) {
        perror("pthread_create");
        exit(EXIT_FAILURE);
    }
}

void c_pthread_join(pthread_t thread, void **retval) {
    if(pthread_join(thread, retval) == -1) {
        perror("pthread_join");
        exit(EXIT_FAILURE);
    }
}

time_t c_time(time_t *t) {
    time_t time_val = time(t);
    if(time_val == ((time_t) -1)) {
        perror("time");
        exit(EXIT_FAILURE);
    }
    return time_val;
}

void printHeader() {
    printf(" %-10s | %-8s | %-4s | %s\n", "TIME", "THREAD", "ID", "STATUS");
    printf("------------+----------+------+------------\n");
    fflush(stdin);
}

void printInfo(char *pname, int tid, char *msg) {
    printf(" %-10d | %-8s | %-4d | %s\n", (int) c_time(NULL), pname, tid, msg);
    fflush(stdin);
}