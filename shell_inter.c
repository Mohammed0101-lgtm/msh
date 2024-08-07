#include "shell_inter.h"
#include "input.h"
#include "config.h"
#include "exec.h"

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <stdbool.h>
#include <errno.h>
#include <pwd.h>


int shell_inter() {
    char *line  = NULL;
    char **args = NULL;
    int status  = 0;

    do {
        char username[_SC_LOGIN_NAME_MAX];
        char hostname[_SC_HOST_NAME_MAX];
        char cwd[PATH_MAX];

        if (getlogin_r(username, sizeof(username)) != 0) {
            fprintf(stderr, 
                    RED "Error getting username : %s\n" reset, strerror(errno));
            exit(EXIT_FAILURE);
        }

        if (gethostname(hostname, sizeof(hostname)) != 0) {
            fprintf(stderr, 
                    RED "Error getting hostname : %s\n" reset, strerror(errno));
            exit(EXIT_FAILURE);
        }

        if (!getcwd(cwd, sizeof(cwd))) {
            fprintf(stderr, 
                    RED "Error getting current working directory : %s\n" reset, strerror(errno));
            exit(EXIT_FAILURE);
        }

        printf(CYN "%s@%s@%s$ " reset, username, hostname, cwd);

        line   = read_line();
        args   = tok_line(line);
        status = exec_cmd(args);
        
        free(line);
        free(args);

    } while (status != ERR_STATUS);

    return EXIT_SUCCESS;
}

void shell_non_inter() {
    char *line = NULL;
    
    while ((line = read_line()) != NULL) {
        char **args = tok_line(line);
        int status  = exec_cmd(args);

        free(line);
        free(args);

        if (status == EXIT_FAILURE) {
            fprintf(stderr, 
                    RED "Error: Unsupported command\n" reset);
        }
    }
}
