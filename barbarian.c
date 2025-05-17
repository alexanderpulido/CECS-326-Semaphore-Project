#include <stdio.h>
#include <signal.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <semaphore.h>
#include "dungeon_info.h"

struct Dungeon* sharemem;   // Sets up the shared memory pointer to be used when the barbarian gets the signal in barb_handler()
sem_t* sem1;    // Sets up its semaphore to be used when it is time


void barb_attack(struct Dungeon* sharemem)
{
    sharemem->barbarian.attack = sharemem->enemy.health;  
}

void barb_handler(int sig)
{
    barb_attack(sharemem);  // The barbarian attack to enemy health function that gets executed
}

void barb_sem()
{
    sem_post(sem1);
}

void barb_sem_unlock_handler(int sig)
{
    barb_sem();
    sem_close(sem1);
}

int main()
{
    struct sigaction sa;    // Setting the sigaction struct
    sa.sa_handler = barb_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(DUNGEON_SIGNAL, &sa, NULL) < 0)   // Checking the execution of sigaction onto the signal
    {
        perror("sigaction");
        return -1;
    }

    sem1 = sem_open(dungeon_lever_one, O_CREAT, 0644, 0);   // Opens the first lever semaphore object
    struct sigaction sa2;
    sa2.sa_handler = barb_sem_unlock_handler;   // Map the sigaction handler to the barbarian's unlock semaphore handler function
    sigemptyset(&sa2.sa_mask);
    sa2.sa_flags = SA_RESTART;
    if (sigaction(SEMAPHORE_SIGNAL, &sa2, NULL) < 0)    // Execute the sigaction
    {
        perror("sigaction");
        return -1;
    }

    int shm_file_desc = shm_open(dungeon_shm_name, O_RDWR, 0);  // Opens the dungeon shared memory object
    if (shm_file_desc == -1)
    {
        perror("shm_open");
        return -1;
    }
    sharemem = mmap(NULL, sizeof(struct Dungeon), PROT_READ| PROT_WRITE, MAP_SHARED, shm_file_desc, 0); // Maps to the shared mem object that is the size of the dungeon
    if (sharemem == MAP_FAILED)
    {
        perror("mmap");
        return -1;
    }

    while (true)
    {
        pause();    // Stay on hold until a signal gets called
    }

    return 0;
}
