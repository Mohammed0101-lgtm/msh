#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <errno.h>
#include <pwd.h>

#include "config.h"
#include "input.h"
#include "exec.h"
#include "command.h"
#include "shell_inter.h"


int main(void) {
    if (isatty(STDIN_FILENO)) {
        if (shell_inter() != 0) {
            fprintf(stderr, "Shell interrupted\n");
            exit(EXIT_FAILURE);
        }
    } else {
        shell_non_inter();
    }
    
    return 0;
}

