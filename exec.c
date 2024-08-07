#include "exec.h"
#include "config.h"
#include "command.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/wait.h> 

// Creating a new process for command execution
int new_process(char **args, int (*func)(char **)) {
    pid_t pid = fork(); // Create child process
    int status;

    if (pid == 0) {
        // Child process
        int result = func(args);
        
        if (result == ERR_STATUS) {
            fprintf(stderr, 
                    RED "Error executing command : %s\n" reset, strerror(errno));
        }
        
        _exit(result); // Exit child process with the result of the command
    } else if (pid < 0) {
        // Fork failed
        fprintf(stderr,
                RED "Error in creating child process : %s\n" reset, strerror(errno));
        return ERR_STATUS;
    } else {
        // Parent process: wait for the child process to finish
        do {
            waitpid(pid, &status, WUNTRACED);
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
    }

    return WEXITSTATUS(status); // Return the exit status of the child process
}

// Supported commands and corresponding pointers to the functions
int exec_cmd(char **args) {
    static char *builtin_func_list[] = {
        "ls",
        "cd",
        "echo",
        "rm",
        "mv",
        "open",
        "touch",
        "cmake",
        "run",
        "mkdir",
        "clang",
        "comake",
        "build",
        "pwd",
        "head"
    };
    
    static int (*builtin_func[])(char **) = {
        &ls,
        &cd, 
        &echo, 
        &rm, 
        &mv,
        &shopen,
        &touch,
        &cmake,
        &run, 
        &createDirectory, 
        &clang,
        &comake,
        &build,
        &pwd,
        &head
    };

    if (!args[0]) {
        fprintf(stderr, 
                RED "Invalid command\n" reset);
        return NOTSUP_STATUS;
    } 
    else if (strcmp(args[0], "exit") == 0) {
        exit(EXIT_SUCCESS);
    }

    // Find and execute the command
    for (int i = 0, n = sizeof(builtin_func_list) / sizeof(char *); i < n; i++) {
        if (strcmp(args[0], builtin_func_list[i]) == 0) {
            return new_process(args, builtin_func[i]);
        }
    }

    // Command not found
    fprintf(stderr, 
            RED "%s : Unsupported command\n" reset, args[0]);
    return NOTSUP_STATUS;
}
