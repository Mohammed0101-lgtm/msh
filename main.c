#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "config.h"
#include "shell_inter.h"


int main(void) {
    if (isatty(STDIN_FILENO)) {
        if (shell_inter() != 0) {
            fprintf(stderr, RED "Shell interrupted\n" reset);
            exit(EXIT_FAILURE);
        }
    } else {
        shell_non_inter();
    }

    return 0;
}
