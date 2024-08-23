#include "config.h"
#include "input.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>

// for reading the command line input
char *read_line() {
    // initialize the line input
    char *line = NULL;
    line       = (char *)malloc(BUFSIZ * sizeof(char) + 1);
    
    if (line == NULL) {
        fprintf(stderr, 
                RED "Failed to allocate memory : %s\n" reset, strerror(errno));
        
        exit(EXIT_FAILURE);
    }

    // read the line input
    if (fgets(line, BUFSIZ, stdin) == NULL) {
        // incase the stdin is saturated
        if (feof(stdin)) {
            free(line);
            exit(EXIT_SUCCESS);
        } else {
            fprintf(stderr, 
                    RED "Error reading input from 'stdin'.\n" reset);
            
            free(line);
            exit(EXIT_FAILURE);
        }
    }

    // strip newline
    if (line != NULL) {
        size_t len = strlen(line);
        
        if (len > 0 && line[len - 1] == '\n') 
            line[len - 1] = '\0';
    }

    return line; 
}

// read commands from a file
char *read_stream() {
    int i           = 0;
    size_t buf_size = 64;
    char *line      = (char *)malloc(buf_size * sizeof(char));
    int character;
    
    if (line == NULL) {
        fprintf(stderr,     
                RED "Memory allocation failed!\n" reset);
        
        exit(EXIT_FAILURE);
    }

    // while the current character is not 'EOF' or '\n' 
    while ((character = getchar()) != EOF && character != '\n') {
        // append the current char to the 'line'
        line[i++] = character;

        // if 'line' is not enough of a buffer
        // then double the size to realloc
        if (i >= buf_size) {
            buf_size  *= 2; 
            char *temp = (char *)realloc(line, buf_size * sizeof(char));

            if (temp == NULL) {
                fprintf(stderr, 
                        RED "Memory reallocation failed (read_stream)! : %s\n" reset, strerror(errno));
                
                free(line);
                exit(EXIT_FAILURE);
            }

            line = temp;
        }
    }

    line[i] = '\0';
    return line;
}

// tokenize the input stream by space seperators
char **tok_line(char *line) {
    // buffer to hold tokens
    int buf_size  = 64;
    char **tokens = (char **)malloc(buf_size * sizeof(char *));
    
    if (tokens == NULL) {
        fprintf(stderr,     
                RED "Memory allocation failed (tokenize line) : %s\n" reset, strerror(errno));
        
        exit(EXIT_FAILURE);
    }

    // go token by token
    char *token = NULL;
    int i       = 0;
    token       = strtok(line, TOK_DELIM); // using strtok that returns token by delimiters

    // loop goes until the end of input stream token by token
    while (token != NULL) {
        if (token[0] == '#') { // ignore comments
            break;
        }
        
        tokens[i++] = token; // insert token at the end of the array

        // in case the buffer is not sufficient for more tokens
        if (i >= buf_size) {
            buf_size += buf_size; // double the buffer size
            // realloc memory at the same region
            char *temp = (char *)realloc(tokens, buf_size * sizeof(char));
            if (temp == NULL) {
                fprintf(stderr, 
                        RED "Memory reallocation failed (read_stream)! : %s\n" reset, strerror(errno));
                free(line);  
                exit(EXIT_FAILURE);
            }

            tokens = &temp;
        }

        token = strtok(NULL, TOK_DELIM); // go to the next token
    }

    tokens[i] = NULL; // null terminate the tokens array
    
    return tokens;
}
