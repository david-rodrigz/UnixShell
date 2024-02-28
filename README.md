# Homework #3: Execution Integrity

* Author: David Rodriguez
* Class: CS 452 Section 1
* Semester: Spring 2024

## Overview

This project is a Shell implementation that allows users to run bash terminal commands. Users can also utilize various operators such as for background processes `&`, sequential processes `;`, pipelines `|`, and I/O redirection `<` `>`.

## Reflection

The hardest part about this project was implementing the pipelines and background processes. Namely, child processes were difficult to work with as I could not find an effective way to debug them (at least not with the `gdb` debugger). This forced me to learn how the `fork`ing process works in bash. The most important lesson I learned from this is that all child processes carry a PID of 0. Knowing this, I knew what PIDs to `wait` for and how to exclude code just for the child processes and not for the parent process.

I look forward to learning more about how `jobs` play an important part in execution integrity in future projects. Even though I was able to create a data structure for these processes, I did not find myself using them much in this project. I understand that the `jobs` command in Shell allows users to monitor and manipulate tasks or processes started in the current shell. Perhaps my program could have used this resource more effectively. 

## Compiling and Using

To compile and run the `Shell.c` program on onyx, `cd` into the main directory if not done already. Make sure that the corresponding parent `GNUmakefile` is located in the parent directory (you can get it from `~jbuffenb/classes/452/pub/GNUmakefile`). Then, run the following commands:

```bash
make try
```
Then 
```bash
try
```

The program will prompt you to run a Bash command after each execution. To exit, use the `exit` command.

To run the regression Tester, run the following command (also from the main directory):

```bash
Test/run
```