#include "exec.h"
#include "config.h"
#include "command.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

int new_process(char **args, int command) {
    pid_t pid = fork(); 
    int status;

    if (pid == 0) {
        int result = command;
        if (result == ERR_STATUS) {
            fprintf(stderr, "Error forking process : %s\n", strerror(errno));
            _exit(EXIT_FAILURE);
        }
        _exit(EXIT_SUCCESS);
    } else if (pid < 0) {
        fprintf(stderr, "Error in creating child process : %s\n", strerror(errno));
        return ERR_STATUS;
    } else {
        do {
            waitpid(pid, &status, WUNTRACED);
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
    }

    return NOTSUP_STATUS;
}

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

    if (args[0] == NULL) {
        fprintf(stderr, "Invalid command\n");
        return NOTSUP_STATUS;
    } else if (strcmp(args[0], "exit") == 0) {
        exit(EXIT_SUCCESS); 
    }

    for (int i = 0, n = sizeof(builtin_func_list) / sizeof(char *); i < n; i++) {
        if (strcmp(args[0], builtin_func_list[i]) == 0) 
            return new_process(args, ((*builtin_func[i])(args)));
    }

    fprintf(stderr, "%s : Unsupported command\n", args[0]);
    return NOTSUP_STATUS;
}

