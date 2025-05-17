#include <stdio.h>
#include <signal.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h> // for strlen
#include <semaphore.h>
#include "dungeon_info.h"

struct Dungeon* sharemem;
sem_t* sem2;    // Sets up its semaphore to be used when it is time

void wiz_spell(struct Dungeon* sharemem)
{
    int key = sharemem->barrier.spell[0];   // First character is the key
    int shift = key % 26;   // Have the key number mod 26 to get the shift by amount
    char* text = sharemem->barrier.spell;
    for (int i = 1; text[i] != '\0'; i++)   // Only looks at capital letters and lowercase letters, ignores the rest
    {
        if (text[i] >= 'A' && text[i] <= 'Z')
        {
            text[i] = ((text[i] - 'A' - shift + 26) % 26) + 'A';
        }
        else if (text[i] >= 'a' && text[i] <= 'z')
        {
            text[i] = ((text[i] - 'a' - shift + 26) % 26) + 'a';
        }
        sharemem->wizard.spell[i - 1] = text[i];    // So it doesn't write the first character key into the wizard spell
    }
    sharemem->wizard.spell[strlen(text) - 1] = '\0'; // Null terminates the wizard's spell
    sharemem->barrier.spell[0] = '\0'; // Clears the barrier spell so the text doesn't get overwritten in the next round (this was frustrating to figure out why it was happening!!!)
}

void wiz_handler(int sig)
{
    wiz_spell(sharemem);	// When it gets the signal, execute function above
}

void wiz_sem()
{
    sem_post(sem2);	// Open the lever 2
}

void wiz_sem_unlock_handler(int sig)
{
    wiz_sem();
    sem_close(sem2);
}

int main()
{
    struct sigaction sa;
    sa.sa_handler = wiz_handler;    // Map the sigaction handler to the wizard handler so it can decipher
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(DUNGEON_SIGNAL, &sa, NULL) < 0)   // Checking the execution of sigaction onto the signal
    {
        perror("sigaction");
        return -1;
    }

    sem2 = sem_open(dungeon_lever_two, O_CREAT, 0644, 0);   // Opens the second lever semaphore object
    struct sigaction sa2;
    sa2.sa_handler = wiz_sem_unlock_handler;    // Map the sigaction handler to the wizard's unlock semaphore handler function
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
