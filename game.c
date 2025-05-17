#include <stdio.h>
#include <signal.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <fcntl.h>
#include <ctype.h>
#include "dungeon_info.h"


int main()
{
    shm_unlink(dungeon_shm_name);   // Unlinking the process to make sure it's not still running when the program runs again (otherwise you'll get an error at shm_file_desc == -1)
    struct Dungeon* dptr;   // Making the pointer to the struct Dungeon in dungeon_info.h
    // Looping it in a pid_t array didn't work so create each process individually (UPDATE 4/30/25: there is in fact a way to do it but leave it for now)
    pid_t wizard, rogue, barbarian; // Set the process IDs for the 3 new processes


    int shm_file_desc = shm_open(dungeon_shm_name, O_CREAT | O_EXCL | O_RDWR, 0666);
    if (shm_file_desc == -1)
    {
        perror("shm_open");
        return -1;
    }

    if (ftruncate(shm_file_desc, sizeof(struct Dungeon)) == -1)
    {
        perror("ftruncate");
        return -1;
    }

    struct Dungeon* sharemem = mmap(NULL, sizeof(*dptr), PROT_READ| PROT_WRITE, MAP_SHARED, shm_file_desc, 0);
    if (sharemem == MAP_FAILED)
    {
        perror("mmap");
        return -1;
    }

    // Forks and execs barbarian process
    barbarian = fork();
    if (barbarian == 0)
    {
        execl("./barbarian", "./barbarian", NULL);
        perror("Failed to execute barbarian");
        exit(EXIT_FAILURE);
    }
    sleep(1);
    kill(barbarian, DUNGEON_SIGNAL);

    // Forks and execs wizard process
    wizard = fork();
    if (wizard == 0)
    { 
        execl("./wizard", "./wizard", NULL);
        perror("Failed to execute wizard"); // If execl failed this prints
        exit(EXIT_FAILURE);
    }
    sleep(2);
    kill(wizard, DUNGEON_SIGNAL);

    // Forks and execs rogue process
    rogue = fork();
    if (rogue == 0)
    {
        execl("./rogue", "./rogue", NULL);
        perror("Failed to execute rogue");
        exit(EXIT_FAILURE);
    }
    sleep(1);
    kill(rogue, DUNGEON_SIGNAL);

    kill(barbarian, SEMAPHORE_SIGNAL);
    kill(wizard, SEMAPHORE_SIGNAL);
    kill(rogue, SEMAPHORE_SIGNAL);
    
    RunDungeon(wizard, rogue, barbarian);   // Calls the dungeon function in dungeon_info.h to run
    shm_unlink(dungeon_shm_name);

    return 0;
}
