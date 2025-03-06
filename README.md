# POSIX-Semaphores
POSIX Semaphores in Operating Systems Exercise

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
./erg config <M> <configuration_file>
```
The first argument specifies the configuration file, while the second indicates the maximum number of child processes.
