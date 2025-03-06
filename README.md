# POSIX-Semaphores
## Introduction
The purpose of this project is to develop a program in the C programming language that utilizes shared memory and semaphores for managing and synchronizing multiple processes. In this implementation, efficient cooperation between a parent process and multiple child processes is achieved using inter-process communication (IPC) mechanisms that ensure correct execution.

## Program Structure
The program consists of the main function and three helper functions:
### Helper Functions
1. read_data: Responsible for reading the configuration file and storing its data in dynamically allocated memory through the `Data` structure. This structure includes three fields:
```c
typedef struct {
    int number;                                       
    int process;                                       
    char type;                                      
} Data;
```
2. max_i_processes: Calculates the maximum number of active child processes based on the data in the configuration file.
3. read_random_line: Selects and returns a random line from a given text file.

### Main Function
The program starts by verifying the command-line arguments. The required format is:
```c
./erg <configuration_file> <M> 
```
The first argument specifies the configuration file, while the second indicates the maximum number of child processes.

## Semaphores & Shared Memory
The program then initializes shared memory, which includes space for storing the random line from the text file, M+1 semaphores for inter-process communication, and an integer that indicates which process should terminate. The program attaches to the shared memory, grants read/write access to the parent process, and sets the appropriate pointers for shared memory and semaphores.

M+1 POSIX semaphores are initialized in a blocked state, and the parent process releases its semaphore to begin execution. The PID of the parent process is initialized using getpid(), and child processes are initialized to 0, indicating they are not active.

## Execution Loop
The program handles inter-process communication within a for-loop, where each iteration represents a timestamp. Commands are executed only when the current timestamp matches the one in the configuration file. The available commands are `T` (terminate) and `S` (spawn a new child process). In both commands, the parent process selects a random active child process to send the random line. This is done using a helper function `read_random_line`, and the line is transmitted via shared memory. The parent activates the child's semaphore to print the line and waits for the completion of this action before continuing to the next command.

## Resource Cleanup & Memory Deallocation
At the end of the program, all resources used by the processes are freed. This includes deleting the M+1 semaphores, detaching the shared memory, and marking it for deletion after execution. Additionally, dynamically allocated variables such as random_line and data_array are freed to prevent memory leaks.

The program was thoroughly tested using Valgrind to ensure there are no memory leaks across various configurations.

## Makefile
### Compile
```c
make erg
```

### Run
```c
./erg <configuration_file> <M> 
```

