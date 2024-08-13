
// This file containes the implemenetation of the supported shell commands
#include "shell_inter.h"
#include "command.h"
#include "config.h"

#include <assert.h>
#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <limits.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdbool.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>

#define _POSIX_SOURCE
#define _XOPEN_SOURCE_EXTENDED 1

const int MAX_FILES       = 128;
const int rows = 4, width = 84;

// Print the directory contents
int recDir(char *path) {
    if (path == NULL) { 
        return ERR_STATUS;
    }

    // open the specified directory 
    DIR *dir = opendir(path);
    if (dir == NULL) {
        return ERR_STATUS;
    }

    // pointer for each entry in the directory
    struct dirent *entry; 

    // Print the directory name
    printf(BBLU "\t%s :\n" reset, path);

    size_t buf_size = 32; // initial buffer for the files

    char **files = (char **)malloc(buf_size * sizeof(char *)); // Allocate space for file names
    if (files == NULL) {
        fprintf(stderr, RED "Failed to allocate memory!\n" reset "recDir()\n");
        closedir(dir);
        return ERR_STATUS;
    }

    size_t j = 0; 
    size_t max_len = 0; 

    // Iterate through the directory
    while ((entry = readdir(dir))) {
        // Skip hidden files (names starting with '.')
        if (strncmp(entry->d_name, ".", 1) != 0) {
            // reallocate memory for extra files if needed
            if (j >= buf_size) {
                buf_size *= 2;

                char **temp = (char **)realloc(files, buf_size * sizeof(char *));
                if (temp == NULL) {
                    fprintf(stderr, RED "Failed to reallocate memory\n" reset "recDir()\n");
                    // Free previously allocated memory
                    for (size_t k = 0; k < j; k++) {
                        free(files[k]);
                    }

                    free(files);
                    closedir(dir);
                    return ERR_STATUS;
                }

                files = temp;
            }

            files[j] = strdup(entry->d_name);
            if (files[j]) {
                size_t len = strlen(files[j]);
                if (len > max_len) { 
                    max_len = len;
                }

                j++;
            } else {
                fprintf(stderr, RED "Failed to duplicate string\n" reset "recDir()\n");
                // Free allocated memory before returning
                for (size_t k = 0; k < j; k++) {
                    free(files[k]);
                }

                free(files);
                closedir(dir);
                return ERR_STATUS;
            }
        }
    }

    // Print files with justification
    int separation = max_len + 4;
    for (size_t k = 0; k < j; k++) {
        printf(GREEN "%-*s" reset, (int)(max_len + separation), files[k]);
        
        if ((k + 1) % 4 == 0) {
            printf("\n");
        }
    }

    printf("\n");

    // Cleanup
    for (size_t k = 0; k < j; k++) {
        free(files[k]);
    }

    free(files);
    closedir(dir);

    return SUC_STATUS;
}

// print the current working directory
int pwd(char **args) {
    // no arguments are required for this command
    if (args[1] != NULL) {
        fprintf(stderr, 
                RED "Too many arguments\n" reset "pwq()\n");
        
        return ERR_STATUS;
    } 

    // allocate memory for the path string
    char *path = (char *)malloc((PATH_MAX + 1) * sizeof(char));
    if (path == NULL) {
        fprintf(stderr, 
                RED "Failed to allocate memory\n" reset "pwd()\n");
        
        return ERR_STATUS;
    }

    // get the current working directory path
    path = getcwd(path, PATH_MAX + 1);
    if (path == NULL) {
        fprintf(stderr, 
                RED "Failed to get current working directory\n" reset "pwd()\n");
        
        return ERR_STATUS;
    }

    // print and free
    printf("%s\n", path);
    free(path);
    return SUC_STATUS;
}

// List the directory contents
int ls(char **args) {
    DIR *dir = opendir("."); // Open the current directory
    if (dir == NULL) {
        fprintf(stderr,
                RED "Error opening current directory\n" reset);
        
        return ERR_STATUS;
    }

    char cwd[PATH_MAX];
    // If no arguments are provided, list the current directory
    if (args[1] == NULL) {
        if (getcwd(cwd, sizeof(cwd)) == NULL) {
            fprintf(stderr, 
                    RED "Error getting current working directory\n" reset);
        
            closedir(dir);
            return ERR_STATUS;
        }
        
        if (recDir(cwd) != 0) {
            closedir(dir);
            return ERR_STATUS;
        }
    } else {
        // Check if the provided path needs a trailing slash
        size_t len = strlen(args[1]);
        if (len > 0 && args[1][len - 1] != '/') {
            char *new_path = (char *)realloc(args[1], len + 2);
            if (new_path == NULL) {
                fprintf(stderr, 
                        RED "Failed to allocate memory\n" reset "ls()\n");
        
                closedir(dir);
                return ERR_STATUS;
            }
        
            args[1] = new_path;
            strcat(args[1], "/");
        }

        // List the directory specified by the argument
        if (recDir(args[1]) != 0) {
            closedir(dir);
            return ERR_STATUS;
        }
    }

    closedir(dir);
    return SUC_STATUS;
}

// this function does not have printing to a file
// functionality , you can add it if you want
int echo(char **args) {
    int i = 1;
    // provide a string to echo it 
    if (args[1] == NULL) {
        fprintf(stderr, 
                YEL "Usage : echo..[string]..[string]..\n" reset);
        
        return ERR_STATUS;
    }
    
    // print provided arguments
    while (args[i] != NULL) {
        printf("%s ", args[i++]);
    }

    printf("\n");

    return (i > 1) ? SUC_STATUS : ERR_STATUS;
}

void cd_usage() {
    fprintf(stderr, YEL "cd usage : cd [directory]\n" reset);
    fprintf(stderr, YEL "cd .. => go to parent dir\n" reset);
}

// change the current working directory 
int cd(char **args) {
    // need to provide a directoy to change to 
    if (args[1] == NULL) {
        cd_usage();
        return ERR_STATUS;
    } 
    
    // if entered command is : cd .
    // then stay in the cwd
    if (strcmp(args[1], ".")) {
        char cwd[1024];
      
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            printf("Current directory: %s\n", cwd);
            return SUC_STATUS;
        } else {
            perror("getcwd");
            return ERR_STATUS;
        }
    } else if (strcmp(args[1], "..") == 0 && chdir("..") != 0) {
        perror("cd"); // if changing to the parent directory fails
        return ERR_STATUS; 
    } else if (chdir(args[1]) != 0) {
        perror("cd");
        return ERR_STATUS;
    } 

    return SUC_STATUS;
}

void rm_usage() {
    fprintf(stderr, YEL "rm usage : rm *[file]\n" reset);
}

// remove a given file
int rm(char **args) {
    // a file path is required
    if (args[1] == NULL) {
        rm_usage();
        return NOTSUP_STATUS;
    }

    // try to remove the file through a syscall
    if (remove(args[1]) != 0) {
        fprintf(stderr, 
                RED "failed to remove file : %s\n"reset, args[1]);
        
        return NOTSUP_STATUS;
    } 
    
    return SUC_STATUS;
}

void mv_usage() {
    fprintf(stderr, YEL "mv usage : mv *[filename] -- ""newfilename""" reset);
}

// rename a file
int mv(char **args) {
    // at least two arguments are required
    if (args[1] == NULL || args[2] == NULL) {
        mv_usage();
        return NOTSUP_STATUS;
    } 
    else 
        // try renaming the file through a system call
        if (rename(args[1], args[2]) != 0) {
            fprintf(stderr, 
                    RED "Error renaming file : %s\n" reset, args[1]);
            
            return NOTSUP_STATUS;
        }
    
    return SUC_STATUS;
}


// open a file
int shopen(char **args) {
    if (args[1] == NULL) {
        fprintf(stderr, 
                RED "open: missing file path" reset);
        
        return NOTSUP_STATUS;
    } 

    size_t buf_size = 64;
    char *command   = (char *)malloc((buf_size + 1) * sizeof(char));
    
    if (command == NULL) {
        fprintf(stderr, 
                RED "Memory allocation failed\n" reset "open()\n");
        
        return ERR_STATUS;
    }

    strcpy(command, args[0]);
    strcat(command, " ");

    int i = 1;
    while (args[i] != NULL) {
        if (strlen(command) + strlen(args[i]) <= buf_size) {
            strcat(command, args[i]);
            
            if (args[i + 1] != NULL) {
                strcat(command, " "); 
            }
        } else {
            buf_size += buf_size;
            command   = (char*)realloc(command, (buf_size + 1) * sizeof(char));
            
            if (command == NULL) {
                fprintf(stderr, 
                        RED "Memory reallocation failed!\n" reset "open()\n");
                
                free(command);
                return ERR_STATUS;
            }
            /* fprintf(stderr, "Concatenation would exceed MAXNAMLEN\n");
            free(command);
            return ERR_STATUS; */
        }

        i++;
    }
    // fork the process
    pid_t pid = fork();

    if (pid == 0) {
        int dev_null = open("/dev/null", O_RDWR);

        if (dev_null == -1) {
            perror("open");
            _exit(EXIT_FAILURE);
        }

        dup2(dev_null, STDIN_FILENO);
        dup2(dev_null, STDOUT_FILENO);
        dup2(dev_null, STDERR_FILENO);

        close(dev_null);

        if (execlp("open", "open", args[1], NULL) == -1) {
            perror("execvp");
            _exit(EXIT_FAILURE);
        }
    } else if (pid == -1) {
        fprintf(stderr, 
                RED "Error in creating child process: %s\n" reset, strerror(errno));
        return ERR_STATUS;
    } else {
        int status;
        do {
            waitpid(pid, &status, WUNTRACED);
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
    }

    return SUC_STATUS;
}

// create a new empty file 
int touch(char **args) {
    // no spaces all lower case should 
    // be the converntion of filenames
    if (args[1] == NULL) {
        fprintf(stderr, 
                RED "touch: missing file path\n" reset);
     
        return ERR_STATUS;
    }

    // create the file
    FILE *new_file = fopen(args[1], "w");
    if (new_file == NULL) {
        fprintf(stderr, 
                RED "Failed to create file: %s\n" reset, args[1]);
        
        return ERR_STATUS;
    }

    fclose(new_file);

    return SUC_STATUS;
}

// compile a c program
int cmake(char **args) {
    if (args[1] == NULL) {
        fprintf(stderr, 
                RED "cmake: missing filepath\n" reset);
        
        return ERR_STATUS;
    }

    char *filename       = strdup(args[1]);
    char *executableFile = strdup(filename);
    char *dot_position   = strrchr(executableFile, '.');// get the '.' position to remove the extension

    if (dot_position != NULL) {
        *dot_position = '\0';
    }

    // get the environment path variable which stores 
    // the path to the clang file which is used to compile
    // the program, if it doesn't find it then the user is 
    // forced to install it
    char *envPath = getenv("PATH"); 
    if (envPath == NULL) {
        fprintf(stderr, 
                RED "Failed to retrieve environment 'PATH' variable: %s\n" reset, strerror(errno));
        
        free(filename);
        free(executableFile);
        
        return ERR_STATUS;
    }

    const char *compCmd  = "clang"; // for the compilt command
    const char delimiter = ':'; // the seperator of the environment path variable
    
    // the path to clang
    char searchPath[PATH_MAX];
    // construct the full search path
    strncpy(searchPath, envPath, sizeof(searchPath));
    searchPath[strlen(searchPath) - 1] = '\0'; // enforce '\0'

    // tokenize the search path to iterate through it
    char *dir = strtok(searchPath, &delimiter);
    
    // go through earch dir in the path till finding clang
    while (dir != NULL) {
        // the expected path if 'clang' exists
        char fullPath[PATH_MAX];
        snprintf(fullPath, sizeof(fullPath), "%s/%s", dir, compCmd);

        // fork the current process to execute clang
        // and then maintain the current process in 
        // case of an error in the compilation time
        if (access(fullPath, X_OK) == 0) {
            pid_t pid = fork();

            if (pid == 0) {
                // execute clang with full path
                execl(fullPath, compCmd, filename, "-o", executableFile, (char *)NULL);
                fprintf(stderr, 
                        RED "Error in execl: %s\n" reset, strerror(errno));
              
                free(filename);
                free(executableFile);
                
                _exit(EXIT_FAILURE);
            } else if (pid == -1) {
                // if the forking fails
                fprintf(stderr, 
                        RED "Error in creating child process: %s\n" reset, strerror(errno));
                
                free(filename);
                free(executableFile);
                
                return ERR_STATUS;
            } else {
                // wait till the child process exits
                int status;
                do {
                    waitpid(pid, &status, WUNTRACED);
                } while (!WIFEXITED(status) && !WIFSIGNALED(status));
            }
        }

        // go the next directory if 'clang' is not in tge current one
        dir = strtok(NULL, &delimiter);
    }
    
    free(filename);
    free(executableFile);
    
    return SUC_STATUS;
}


// running an executable file 
int run(char **args) {
    if (args[1] == NULL) {
        fprintf(stderr, 
                RED "run: missing executable file path\n" reset);
        
        return ERR_STATUS;
    }

    // if the argument an executable
    if (access(args[1], X_OK) == 0) {
        // construct the executable command
        // for the system call - exporting
        char *exec_cmd = (char *)malloc(MAXNAMLEN + 3);
        if (exec_cmd == NULL) {
            fprintf(stderr, 
                    RED "Memory allocation failed\n" reset);
            
            return ERR_STATUS;
        }

        // adding the './' to the command
        // this discards the fact that you can in normal
        // shells execute a file that is not in the cwd
        snprintf(exec_cmd, MAXNAMLEN + 3, "./%s", args[1]);

        pid_t pid = fork();

        if (pid == 0) {
            // do the syscall
            execl(exec_cmd, args[1], (char *)NULL);
            fprintf(stderr, 
                    RED "Error in execl: %s\n" reset, strerror(errno));
            
            free(exec_cmd);
            _exit(EXIT_FAILURE);
        } else if (pid == -1) {
            fprintf(stderr, 
                    RED "Error in creating child process: %s\n" reset, strerror(errno));
            
            free(exec_cmd);
            return ERR_STATUS;
        } else {
            int status;
            do {
                waitpid(pid, &status, WUNTRACED);
            } while (!WIFEXITED(status) && !WIFSIGNALED(status));
        }

        free(exec_cmd);
    } else {
        // scream at the user hhhh
        fprintf(stderr, 
                RED "%s: file is not executable\n" reset, args[1]);
        
        return ERR_STATUS;
    }

    return SUC_STATUS;
}

// implementing mkdir
int createDirectory(char **args) {
    // provide the dirname
    if (args[1] == NULL) {
        fprintf(stderr, 
                YEL "Usage: createDirectory <directory_name>\n" reset);
        
        return ERR_STATUS;
    }

    struct stat st = {0};
     
    // create dir
    if (stat(args[1], &st) == -1) {
        if (mkdir(args[1], 0777) != 0) {
            perror("mkdir");
            return ERR_STATUS;
        }
    }

    // check if the dir making is successful
    struct stat path_stat;
    stat(args[1], &path_stat);

    if (!S_ISDIR(path_stat.st_mode)) {
        fprintf(stderr, 
                RED "Failed to create directory\n" reset);
        
        return ERR_STATUS;
    }

    return SUC_STATUS; 
}

// clang cmd in case 
int clang(char **args) {
    char *clangCmd = strdup(args[0]);
    if (clangCmd == NULL) {
        fprintf(stderr,     
                RED "Failed to allocate memory : strdup()\n" reset);
        
        return ERR_STATUS;
    }

    size_t size = 0; 
    while (args[size++]) 
        ;
    
    char *c_file = strdup(args[1]);
    if (c_file == NULL) {
        fprintf(stderr, 
                RED "Failed to allocate memory : strdup()\n" reset);
        
        free(clangCmd);
        return ERR_STATUS;
    }

    char *exec_file = strdup(args[3]);
    if (exec_file == NULL) {
        fprintf(stderr, 
                RED "Failed to allocate memory : strdup()\n" reset);
        
        free(clangCmd);
        free(c_file);
        return ERR_STATUS;
    }

    if (size < 4) {
        fprintf(stderr, 
                RED "clang: missing arguments\n" reset);
        
        free(clangCmd);
        free(c_file);
        return ERR_STATUS;
    } 

    char *envPath = getenv("PATH");
    if (envPath == NULL) {
        fprintf(stderr, 
                RED "Failed to retrieve environment 'PATH' variable: %s\n" reset, strerror(errno));
        
        free(clangCmd);
        return ERR_STATUS;
    }

    const char delimiter = ':';

    char       searchPath[PATH_MAX];    
    strncpy(searchPath, envPath, sizeof(searchPath));
    searchPath[sizeof(searchPath) - 1] = '\0';

    char *dir = strtok(searchPath, &delimiter);
    
    while (dir) {
        char fullPath[PATH_MAX];
        snprintf(fullPath, sizeof(fullPath), "%s/%s", dir, clangCmd);

        if (access(fullPath, X_OK) == 0) {
            pid_t pid = fork();

            if (pid == 0) {
                execl(fullPath, c_file, args[2], exec_file, (char *)NULL);
                fprintf(stderr, 
                        RED "Error in execl: %s\n" reset, strerror(errno));
                
                free(clangCmd);
                _exit(EXIT_FAILURE);
            } else if (pid == -1) {
                fprintf(stderr, 
                        RED "Error in creating child process: %s\n" reset, strerror(errno));
                
                free(clangCmd);
                return ERR_STATUS;
            } else {
                int status;
                do {
                    waitpid(pid, &status, WUNTRACED);
                } while (!WIFEXITED(status) && !WIFSIGNALED(status));
            }
        }

        dir = strtok(NULL, &delimiter);
    }

    free(clangCmd);
    free(c_file);
    free(exec_file);
    
    return SUC_STATUS;
}

// make an object file instead of an executable
int comake(char **args) {
    // clang is the default and object
    // files default to '.o' extention
    if (args[1] == NULL) {
        fprintf(stderr, 
                YEL "Usage: comake -<filename>-..[OPTION] : <flag>\n" reset);
        
        return ERR_STATUS;
    }

    char *filename     = strdup(args[1]); // copy the filename
    char *objectFile   = strdup(filename); // copy the filename for the object file
    char *dot_position = strrchr(objectFile, '.'); // remove the original file ext

    if (dot_position == NULL) { 
        *dot_position = '\0';
    }

    strcat(objectFile, ".o"); // add the '.o' add the end

    char *envPath = getenv("PATH");
    if (envPath == NULL) {
        fprintf(stderr, 
                RED "Failed to retrieve environment 'PATH' variable: %s\n" reset, strerror(errno));
        
        free(filename);
        free(objectFile);
        return ERR_STATUS;
    }

    // construct the search path for 'clang'
    const char *compCmd  = "clang";
    const char delimiter = ':';
    char       searchPath[PATH_MAX];
    
    strncpy(searchPath, envPath, sizeof(searchPath));
    searchPath[sizeof(searchPath) - 1] = '\0';

    // execute the command via syscall
    char *dir = strtok(searchPath, &delimiter);
    
    while (dir) {
        char fullPath[PATH_MAX];
        snprintf(fullPath, sizeof(fullPath), "%s/%s", dir, compCmd);

        if (access(fullPath, X_OK) == 0) {
            pid_t pid = fork();

            if (pid == 0) {
                execl(fullPath, compCmd, "-c", filename, "-o", objectFile, (char *)NULL);
                fprintf(stderr, 
                        RED "Error in execl: %s\n" reset, strerror(errno));
                
                _exit(EXIT_FAILURE);
            } else if (pid == -1) {
                fprintf(stderr, 
                        RED "Error in creating child process: %s\n" reset, strerror(errno));
                
                return ERR_STATUS;
            } else {
                int status;
                do {
                    waitpid(pid, &status, WUNTRACED);
                } while (!WIFEXITED(status) && !WIFSIGNALED(status));
            }
        }

        dir = strtok(NULL, &delimiter);
    }

    // cleanup
    free(filename);
    free(objectFile);
    
    return SUC_STATUS;
}

// linking multiple object files
int build(char **args) {
    // clang will be the default compiler throughout
    char *compCmd = "clang";
    
    int i = 1;
    int j = 0;

    // get the current working directory
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        perror("getcwd() error");
        return ERR_STATUS;
    }

    // get the number of arguments provided
    int size = 0;
    while (args[size++]) 
        ;

    // too few arguments
    if (size < 2) {
        fprintf(stderr, 
                RED "Not enough arguments provided\n" reset);
        return ERR_STATUS;
    }

    // get the object files into a seperate array
    char **files = (char **)malloc(MAX_FILES * sizeof(char *));
    if (files == NULL) {
        fprintf(stderr, 
                RED "Failed to allocate memory\n" reset);
        
        return ERR_STATUS;
    }

    for (i = 1; i < size - 1 && j < MAX_FILES; i++, j++) {
        files[j] = strdup(args[i]); // copy the filename

        // cleanup in case of memory limits
        if (files[j] == NULL) {
            fprintf(stderr, 
                    RED "Failed to allocate memory : strdup()\n" reset);
            
            for (int k = 0; k < j; k++) {
                free(files[k]);
            }

            free(files);
 
            return ERR_STATUS;
        }
    }

    files[j] = NULL;  // Null-terminate the files array

    // isolate the executable file
    char *exec_file = strdup(args[size - 1]);
    
    if (exec_file == NULL) {
        fprintf(stderr, 
                RED "Failed to allocate memory : strdup()\n" reset);
        
        for (int k = 0; k < j; k++) {
            free(files[k]);
        }

        free(files);
 
        return ERR_STATUS;
    }

    // get the environment path variable 
    char *envPath = getenv("PATH");
    if (envPath == NULL) {
        // cleanup
        fprintf(stderr, 
                RED "Failed to get the environment path variable : build()\n" reset);
        
        for (int k = 0; k < j; k++) {
            free(files[k]);
        }
 
        free(files);
        free(exec_file);
 
        return ERR_STATUS;
    }


    // path delimiter
    const char delimiter = ':';

    // dynamic search path for clang
    char *searchPath = strdup(envPath);
    if (searchPath == NULL) {
        fprintf(stderr, 
                RED "Failed to allocate memory : strdup()\n" reset);
        
        for (int k = 0; k < j; k++) {
            free(files[k]);
        }
 
        free(files);
        free(exec_file);
 
        return ERR_STATUS;
    }

    // tokenize path for the search
    char *dir = strtok(searchPath, &delimiter);
    bool found = 0; // keep track of the state

    while (dir && !found) {

        char fullPath[PATH_MAX];
        snprintf(fullPath, sizeof(fullPath), "%s/%s", dir, compCmd);

        if (access(fullPath, X_OK) == 0) {
            pid_t pid = fork();

            if (pid == 0) {
                // executing all args at once
                char *execArgs[size + 1];
                execArgs[0] = compCmd;
 
                for (i = 0; i < j; i++) {
                    execArgs[i + 1] = files[i];
                }
        
                execArgs[j + 1] = exec_file;
                execArgs[j + 2] = NULL;

                execv(fullPath, execArgs);
                perror("execv() error");
 
                exit(ERR_STATUS);
            } else if (pid < 0) {
                perror("fork() error");
                break;
            } else {
                int status;
 
                if (waitpid(pid, &status, 0) == -1) {
                    perror("waitpid() error");
                    break;
                }
 
                if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
                    found = true;
                } else {
                    fprintf(stderr, 
                            RED "Compilation failed\n" reset);
                } 
            }
        }
        
        dir = strtok(NULL, &delimiter);
    }

    // cleanup
    for (int k = 0; k < j; k++) {
        free(files[k]);
    }
        
    free(files);
    free(exec_file);
    free(searchPath);

    if (found == false) {
        return ERR_STATUS; // check state
    }

    return SUC_STATUS;
}

// helper function for the function just below it
int print_head(char *filename, int size) {
    // open file in reading mode
    FILE *fp = fopen(filename, "r");
    if (fp == NULL) {
        fprintf(stderr, 
                RED "file not found : %s\n" reset, filename);

        return ERR_STATUS;
    }  

    // define current variables
    const int max_lines = 10;
    const int MAX_WIDTH = 256;
    size_t buf_siz      = max_lines * MAX_WIDTH * sizeof(char) + 1;
    
    char *buffer = (char*)malloc(buf_siz);
    if (buffer == NULL) {
        fprintf(stderr, 
                RED "Failed to allocate memory\n" reset);
        
        return ERR_STATUS;
    }

    // get the file_size
    fseek(fp, 0, SEEK_END);
    size_t file_size = ftell(fp);
    rewind(fp);

    // if the file has fewer lines then ten
    if (file_size > buf_siz) {
        if (fread(buffer, 1, buf_siz, fp) != buf_siz) {
            fprintf(stderr, 
                    RED "Error reading file\n" reset);
            
            fclose(fp);
            free(buffer);
            return ERR_STATUS;
        }
        
        buffer[buf_siz] = '\0';    
    } else {
        if (fread(buffer, 1, file_size, fp) != file_size) {
            fprintf(stderr, 
                    RED "Error reading file : %s\n" reset, filename);
           
            free(buffer);
            fclose(fp);
            return ERR_STATUS;
        }
        
        buffer[file_size] = '\0';
    }

    fclose(fp);

    int lineCounter = 0; 
    int lastline = 0;
    
    for (size_t i = 0; i < buf_siz; i++) {
        if (buffer[i] == '\n') {
            lineCounter++;
        }
        
        if (lineCounter == 10) {
            lastline = i;
            break;
        }
    }
    
    buffer[lastline + 1] = '\0';
    
    if (size != 0) {
        printf(GREEN "==> %s <==\n" reset, filename);
    }
    
    printf("%s", buffer);
    free(buffer);
    
    return SUC_STATUS;
}

// print the first ten lines of a readable file
int head(char **args) {
    if (args[1] == NULL) {
        fprintf(stderr, 
                RED "head() : missing file path\n" reset);
    
        return ERR_STATUS;
    } else if (args[2] == NULL) {
        return print_head(args[1], 0);
    }

    // if there is more then one
    // file is provided in command
    int i = 1;
    while (args[i] != NULL) {
        if (print_head(args[i], 1) != SUC_STATUS) {
            return ERR_STATUS;
        }

        i++;
    }   

    return SUC_STATUS;
}



