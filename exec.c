#include "exec.h"
#include "config.h"
#include "command.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

// creating new process for the command execution
int new_process(char **args, int command) {
    pid_t pid = fork(); // create child process
    int status;

    if (pid == 0) {
        // execute through a function callback
        int result = command;
        
        if (result == ERR_STATUS) {
            fprintf(stderr, 
                    "Error forking process : %s\n", strerror(errno));
            
            _exit(EXIT_FAILURE);
        }
        
        _exit(EXIT_SUCCESS); // exit child process
    } else if (pid < 0) {
        fprintf(stderr, 
                "Error in creating child process : %s\n", strerror(errno));
        
        return ERR_STATUS;
    } else {
        // wait for the child process to execute
        do {
            waitpid(pid, &status, WUNTRACED);
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
    }

    return NOTSUP_STATUS;
}

// supported commands and corresponding pointers to the function
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
                "Invalid command\n");
        
        return NOTSUP_STATUS;
    } 
    else if (strcmp(args[0], "exit")) 
        exit(EXIT_SUCCESS); 

    // find the command to execute
    for (int i = 0, n = sizeof(builtin_func_list) / sizeof(char *); i < n; i++) 
        if (strcmp(args[0], builtin_func_list[i]) == 0) 
            return new_process(args, ((*builtin_func[i])(args)));

    // if the command is not found
    fprintf(stderr, 
            "%s : Unsupported command\n", args[0]);
    
    return NOTSUP_STATUS;
}
