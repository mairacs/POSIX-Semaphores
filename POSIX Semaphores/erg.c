#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/ipc.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <semaphore.h>

#define RANDOMLINESIZE 1024

typedef struct {
    int number;                                         // xronoshmansh
    int process;                                        // For storing the code like "C3", "C1"
    char type;                                          // For storing 'S' or 'T'
} Data;


// a function which reads the data of the configuration file
Data *read_data(const char *filename, Data *data_array, int *num_lines) {
    FILE *file = fopen(filename, "r");                                          // open the configuration file
    if (file == NULL) {
        perror("Could not open file");
        exit(EXIT_FAILURE);
    }

    int lineCount = 0;                                                         // count how many lines-commands the configuration file has
    char temp_line[256];
    while (fgets(temp_line, sizeof(temp_line), file))                     
        lineCount++;
    *num_lines = lineCount;

    rewind(file);                                                           // go back to start of configuration file

    data_array = malloc(lineCount * sizeof(Data));                          // memory allocation for the array of data
    if (data_array == NULL) {
        perror("Failed to allocate memory for data_array");
        fclose(file);
        exit(EXIT_FAILURE);
    }

    int i = 0;
    char pr[5];
    while (!feof(file)) {
        if(fscanf(file, "%d %3s %c", &data_array[i].number, pr, &data_array[i].type) == 3) {            // read its line and store the data
            sscanf(pr, "C%d", &data_array[i].process);
            i++;                                                                                        // Increment the array index for the next line
        }
    }

    fclose(file);                                                                     // close the configuration file
    return data_array;                                                               // Return the array of data
}

// a function which counts how many child processes the configuration file contains
int max_i_processes(Data* data, int n){
    int max;

    if(!n){                                                                   // if the configuration file contains no commands                        
        perror("Empty dataset.\n");
        exit(EXIT_FAILURE);
    }
    if(n==1)                                                                 // if the configuration file contains only one command
        return data[0].process;

    max = data[0].process;                                                  // if the configuration file contains more than one command
    for(int i = 1; i < n-1; i++){
        if(data[i].process > max)
            max = data[i].process;                                          // find the max of child processes
    }
    return max;                                                             // and return it
}

// a function which chooses a random line from a text file
char *read_random_line(char *filename, char *line){
    FILE *file = fopen(filename, "r");                                      // open the text file
    if (file == NULL) {
        printf("Could not open file.\n");
        exit(EXIT_FAILURE);
    }

    int lineCount = 0;                                                       // count how many lines the text file has
    char temp_line[256];
    while (fgets(temp_line, sizeof(temp_line), file)) {
        lineCount++;
    }

    rewind(file);                                                           // go back to start of text file
    
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);                                    // random generator of nanoseconds
    srand((unsigned int)(ts.tv_nsec ^ ts.tv_sec));
    int randomLine;
    randomLine = rand() % lineCount;                                        // choose random line

    for (int i = 0; i <= randomLine; i++) {
        fgets(line, sizeof(temp_line), file);                               // go to this line and store it 
    }

    fclose(file);                                                            // close the text file
    return line;                                                            // return the random line
}


// Main function
int main(int argc, char *argv[]){
    if(argc < 3){                                                                  // test if the user puts the configuration file and M
        perror("Not enough arguments\n");                                       
        exit(EXIT_FAILURE);
    }

    int shm_id;                                                                     //Shared memory variables
    char *shm_text;
    void *shm_base;

    Data *data_array;                                                               //Variables for the data of configuration file and the configuration file
    char *filename = argv[1];

    char *random_line;                                                              // Variables for the random line that is sent to child processes
    random_line = malloc(256*sizeof(char));

    int num_commands;
    data_array = read_data(filename, data_array,&num_commands);                     // read the data of the configuration file and store them in an array
    int max = max_i_processes(data_array, num_commands);                            // count how many child processes the configuration file contains

    int M = atoi(argv[2]);                                                          // test if the user puts an M which can satisfuy the amount of children processes
    if(M < max){
        printf("Not enough semaphores\n");
        free(random_line);
        free(data_array);
        exit(EXIT_FAILURE);
    }

    shm_id = shmget(IPC_PRIVATE, RANDOMLINESIZE + (M+1)*sizeof(sem_t) + sizeof(int), IPC_CREAT | 0660);  // Create a new shared memory segment which will contain
    if (shm_id == -1) {                                                                               // the random line, M+1 semaphores and an int for the terminated process
        perror("Shared memory creation");
        exit(EXIT_FAILURE);
    }
	shm_base = shmat(shm_id, NULL, 0);                                                      // Attach the shared memory segment where there is enough space
	if ( shm_base == (void *) -1) { 
		perror("Shared memory attachment."); 
		exit(EXIT_FAILURE);
	}

    shm_text = (char *) shm_base;                                                           //pointer at shared memomry for the random line
    sem_t * sp = (sem_t *) (shm_text + RANDOMLINESIZE);                                     // pointer at shared memory for the semaphores
    int *terminated_process = (int *) (shm_text + RANDOMLINESIZE + (M+1)*sizeof(sem_t));    // pointer at shared memory for the terminated process
    *terminated_process = 0;

    for (int i = 0; i < M+1; i++){                          
        sem_init(&sp[i], 1, 0);                                                       // Initialization of the child semaphores to blocked condition
        if (&sp[i] == 0) {
           perror("Semaphore creation.");
            exit(EXIT_FAILURE);
        }
    }
    sem_post((&sp[0]));                                                              // Initialization of the parent process semaphore to 1

    int children_pid[M+1];                                      // Variables for pids
    children_pid[0] = getpid();                                 // pid of parent process
    for (int i=1; i<M+1; i++)
        children_pid[i] = 0;                                    // pid 0 for a child process shows that child is unalive
    int temp_pid;


    int j=0;
    for(int i = 0; i <= data_array[num_commands-1].number; i++){                            // loop for time
        if(i == data_array[j].number && i !=data_array[num_commands-1].number){             // condition for the time and it's not the last command 
            
            if(getpid() == children_pid[0]){                                                // if it's parent process wait the child process to print the random line
                sem_wait(&sp[0]);
            }
            
            printf("Command %d: %d %d %c\n", j+1, data_array[j].number, data_array[j].process, data_array[j].type);

            if(data_array[j].type == 'S'){                                                  // if command is 'S'
                // Child Process
                if((temp_pid = fork()) == 0){                                               // Spawn a child process
                    int child_id = data_array[j].process;
                    int messages_count = 0;                                                 // Initialization of the messages of child process
                    int start_time = data_array[j].number;                                  // Initialization of start time
                    while(1){
                        sem_wait((&sp[child_id]));                                          // P child semaphore in order to wait parent process to send random line

                        if (*terminated_process == child_id) {                              // if command of 'T' has been sent through shared memory
                           char time[4];
                           strcpy(time, shm_text);                                          // End time has also been sent through shared memory
                           int end_time = atoi(time);
                            printf("Child Process %d with PID: %d is terminating, has received %d messages and stayed alive for %d seconds\n", child_id,getpid(), messages_count, (end_time-start_time));
                            *terminated_process = 0;
                            free(random_line);                                              // Memory de-allocation of dynamic variables
                            free(data_array);
                            sem_post(&sp[0]);                                              // V parent semaphore in order to continue running the commands
                            exit(EXIT_SUCCESS);                                            // terminate child process
                        }
                        
                        printf("Child Process %d with PID %d received random line: %s\n", child_id,getpid(), shm_text);      // else print the random line that child process has received
                        messages_count++;
                        sem_post(&sp[0]);                                               // V parent semaphore in order to continue running the commands
                    }   
                }
                // Parent Process
                else                                                                                  
                    children_pid[data_array[j].process] = temp_pid;         // Initialization of the pid of the child which just created                
            }
            else if(data_array[j].type == 'T'){                                             // if command is 'T'
                int child_id = data_array[j].process;
                if(children_pid[child_id]!=0){                                               // test if the child process is alive
                   *terminated_process = child_id;                                          // Send the message of termination through shared memory
                    sprintf(shm_text, "%d", data_array[j].number);                           // Send the end time through shared memory
                    sem_post(&sp[child_id]);                                                // V child semaphore in order child process to continue and terminate
                    sem_wait(&sp[0]);                                                       // P parent semaphore in order to wait child process to terminate
                    int wstatus;
                    waitpid(children_pid[child_id], &wstatus, 0);                           // exit code of child process
                    if (WIFEXITED(wstatus)) {
                        printf("Child Process %d with PID:%d has been terminated with status %d\n", child_id,children_pid[child_id], WEXITSTATUS(wstatus));
                    }
                    children_pid[child_id] = 0;                                             // Mark the child as terminated
                }
                else
                    printf("The Child Process %d tried to be terminated without being spawned\n", child_id);
            }

            // Parent Process
            if(getpid() == children_pid[0] && data_array[j].number != data_array[num_commands-1].number){           
                int test=0;
                for(int i = 1; i<M+1; i++){
                    if(children_pid[i]!=0){                                        // Test if there are any alive child processes
                        test = 1;
                        break;
                    }                        
                }
                
                if(test){                                                          // If there is an alive child send the random line
                    int random_child = (rand() % M) + 1 ;
                    while(children_pid[random_child] <= 0){                        // test in order to send random line to an alive child
                        random_child = (rand() % M) + 1;
                    }
                    random_line = read_random_line("mobydick.txt", random_line);     // read a random line from text file
                    strcpy(shm_text, random_line);                                   // send it through shared memory
                    printf("Parent sent to Child Process %d with PID:%d random line: %s", random_child,children_pid[random_child], shm_text);
                    sem_post((&sp[random_child]));                              // V child semaphore in order child process to print the random line
                }
                else                                                             // If there is no alive child to send the random line
                    sem_post(&sp[0]);                                            // V parent semaphore in order parent process to continue

            j++;                                                                    // go to next command of configuration file
            }
        }

        else if(i == data_array[num_commands-1].number){                        // now we reach the last command EXIT
            sem_wait(&sp[0]);                                                   // P parent semaphore in order to wait child process to print random line
            printf("Command %d: %d EXIT\n", j+1, data_array[j].number);
            printf("Terminating the rest Child Processes\n");
            for(int z = 1; z<M+1; z++){
                if(children_pid[z]!=0){                                         // if the child process is alive
                    *terminated_process = z;                                    // Send the message of termination through shared memory
                    sprintf(shm_text, "%d", data_array[j].number);              // Send the end time through shared memory
                    sem_post(&sp[z]);                                           // V child semaphore in order child process to continue and terminate
                    sem_wait(&sp[0]);                                           // P parent semaphore in order to wait child process to terminate
                    int wstatus;
                    waitpid(children_pid[z], &wstatus, 0);                      // exit code of child process
                    if (WIFEXITED(wstatus)) {
                        printf("Child Process %d has been terminated with status %d\n", z, WEXITSTATUS(wstatus));
                    }
                    children_pid[z] = 0;                                        // Mark the child as terminated
                }
            }
        }
    }
    
    // Clean up and release resources before exiting the program
    for (int i = 0; i < M+1; i++) {                                              // Delete all the semaphores
        if(sem_destroy(&sp[i]) !=0)
            perror("Failed to destroy semaphore");
    }

    if (shmdt(shm_text) == -1)                                                // Detatch the shared memory segment
        perror("Failed to detach shared memory"); 
    
    if(shmctl(shm_id, IPC_RMID, 0) == -1)                                     // Mark the shared memory segment for deletion
	    perror("Failed to mark shared memory for removal");

    free(random_line);                                                      // De-allocation of dynamic variables
    free(data_array);

    return 0;
}