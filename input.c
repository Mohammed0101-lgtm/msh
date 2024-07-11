#include "config.h"
#include "input.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>

char *read_line() {
    char *line = NULL;

    line = (char *)malloc(BUFSIZ * sizeof(char) + 1);
    if (line == NULL) {
        fprintf(stderr, RED "Failed to allocate memory : %s\n" reset, strerror(errno));
        exit(EXIT_FAILURE);
    }

    if (fgets(line, BUFSIZ, stdin) == NULL) {
        if (feof(stdin)) {
            free(line);
            exit(EXIT_SUCCESS);
        } else {
            fprintf(stderr, RED "Error reading input from 'stdin'.\n" reset);
            free(line);
            exit(EXIT_FAILURE);
        }
    }

    if (line != NULL) {
        size_t len = strlen(line);
        if (len > 0 && line[len - 1] == '\n') 
            line[len - 1] = '\0';
    }

    return line;
}

char *read_stream() {
    int i = 0, character;
    size_t buf_size = BUFSIZ;
    
    char *line = (char *)malloc(buf_size * sizeof(char));

    if (line == NULL) {
        fprintf(stderr, "Memory allocation failed!\n");
        exit(EXIT_FAILURE);
    }

    while ((character = getchar()) != EOF && character != '\n') {
        line[i] = character;
        i++;

        if (i >= buf_size) {
            buf_size *= 2;
            char *temp = (char *)realloc(line, buf_size * sizeof(char));

            if (temp == NULL) {
                fprintf(stderr, "Memory reallocation failed (read_stream)! : %s\n", strerror(errno));
                free(line);
                exit(EXIT_FAILURE);
            }

            line = temp;
        }
    }

    line[i] = '\0';
    return line;
}


char **tok_line(char *line) {
    int buf_size = 64;
    char **tokens = (char **)malloc(buf_size * sizeof(char *));
    if (tokens == NULL) {
        fprintf(stderr, "Memory allocation failed (tokenize line) : %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    char *token = NULL;
    int i = 0;
    token = strtok(line, TOK_DELIM);

    while (token != NULL) {
        if (token[0] == '#') 
            break;
        
        tokens[i] = token;
        i++;
        if (i >= buf_size) {
            buf_size += buf_size;
            char *temp = (char *)realloc(tokens, buf_size * sizeof(char));
            if (temp == NULL) {
                fprintf(stderr, "Memory reallocation failed (read_stream)! : %s\n", strerror(errno));
                free(line);  
                exit(EXIT_FAILURE);
            }
            tokens = &temp;
        }
        token = strtok(NULL, TOK_DELIM);
    }
    tokens[i] = NULL;
    return tokens;
}
