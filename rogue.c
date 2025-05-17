#include <stdio.h>
#include <signal.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <semaphore.h>
#include "dungeon_info.h"


struct Dungeon* sharemem;
sem_t* sem1;	// Init the sem1 so it knows when barb opens it
sem_t* sem2;	// Init the sem2 so it knows when wiz opens it

void rog_pick(struct Dungeon* sharemem)
{
    int min = 0;	// Set the min and max number for the guess range
    int max = MAX_PICK_ANGLE;

    while (min <= max)	// Binary search
    {
	    int guess = (min + max) / 2;
	    float rog_guess = (float)guess;	// typecast to float variable for the guess
	    sharemem->rogue.pick = rog_guess;	
	    usleep(TIME_BETWEEN_ROGUE_TICKS);


	    if (sharemem->trap.direction == 'u')	// The number must be higher
	    {
		    min = guess + 1;    // exclude the guess and raise it by 1
	    }
	    else if (sharemem->trap.direction == 'd')	// The number must be lower
	    {
		    max = guess - 1;    // exclude the guess and reduce it by 1
	    }
	    else
	    {
		    break;  // the direction is '-'
	    }
    }
}

void rog_handler(int sig)
{
    rog_pick(sharemem); // call the function above when it gets the signal
}

void rog_get_treasure(struct Dungeon* sharemem)
{
    int treasure_index = 0; // Keep track of the treasure's index when it's ready at the moment
    int spoils_index = 0;   // Keep track of the spoil's index to contain the treasure and null terminate the final index

    while (treasure_index < 4) // Reading through the treasure's contents
    {
        if (sharemem->treasure[treasure_index] != '\0') // tests if the treasure is not that initialized null terminating char
        {
            sharemem->spoils[spoils_index] = sharemem->treasure[treasure_index];
            spoils_index++;
            treasure_index++;
            usleep(100000); // wait for the next char
        } 
        else 
        {
            usleep(100000); // wait for the next char if it is null
        }
    }
    sharemem->spoils[spoils_index] = '\0';	// sets the last index of the spoils to the null terminating char so it can be read at the end (and not seg fault)
}

void rog_sem_handler(int sig)	// For some reason this was executing prematurely so I made these conditions so that wouldn't happen
{
    int val1, val2;
    
    if (sem_getvalue(sem1, &val1) == -1 || sem_getvalue(sem2, &val2) == -1)  // sem_getvalue() takes in the semaphore pointer in the first argument, and the integer pointer in the second argument.
									     // It returns 0 if its successful and the second argument holds that semaphore's current value, and -1 if it fails.
									     // So after this, val1 and val2 become 0 in the next if block, if this got executed first, and 1 when the semaphores are 
									     // defined properly in int main() 
    {
        perror("sem_getvalue");
        return;
    }

    if (val1 > 0 && val2 > 0)   // This ensures both semaphores are initialized and have nonzero values
    {
        sem_wait(sem1);
        sem_wait(sem2);
        rog_get_treasure(sharemem);
    }
}

int main()
{
    struct sigaction sa;
    sa.sa_handler = rog_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(DUNGEON_SIGNAL, &sa, NULL) < 0)   // Checking the execution of sigaction onto the signal
    {
        perror("sigaction");
        return -1;
    }

    sem1 = sem_open(dungeon_lever_one, O_CREAT, 0644, 0);	// Sets it to lever 1
    sem2 = sem_open(dungeon_lever_two, O_CREAT, 0644, 0);	// Sets it to lever 2
    if (sem1 == SEM_FAILED || sem2 == SEM_FAILED)
    {
        perror("sem_open");
        return -1;
    }

// This block here prevents semaphore to be executed while rogue is computing the binary search section (this is why rogue was
// getting stuck once in a while after I implemented semaphore :p) 
// (Update 2: It still does it once in a while might be because of the time between rogue ticks or something else in a different file throwing it off >_<)
{
    struct sigaction sa2;
    sa2.sa_handler = rog_sem_handler;
    sigemptyset(&sa2.sa_mask);
    sa2.sa_flags = SA_RESTART;
    if (sigaction(SEMAPHORE_SIGNAL, &sa2, NULL) < 0)   // Checking the execution of sigaction onto the signal
    {
        perror("sigaction");
        return -1;
    }
}

    int shm_file_desc = shm_open(dungeon_shm_name, O_RDWR, 0);
    if (shm_file_desc == -1)
    {
        perror("shm_open");
        return -1;
    }
    sharemem = mmap(NULL, sizeof(struct Dungeon), PROT_READ| PROT_WRITE, MAP_SHARED, shm_file_desc, 0);
    if (sharemem == MAP_FAILED)
    {
        perror("mmap");
        return -1;
    }

    while (true)
    {
        pause();
        if (sharemem->spoils[0] != '\0' && sharemem->spoils[1] != '\0' && sharemem->spoils[2] != '\0')	 // Check if we have filled the spoils with at least 3 characters
        {
            // We've collected enough treasure, now we release the semaphores and shared memory
            sem_close(sem1);
            sem_close(sem2);
            if (munmap(sharemem, sizeof(struct Dungeon)) == -1) // Helps prevent shared mem from being read into the next run
            {
                perror("munmap");
                exit(EXIT_FAILURE);
            }
            exit(EXIT_SUCCESS);
            break;
        }
    }
    return 0;
}
