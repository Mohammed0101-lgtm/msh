
// This file containes the implemenetation of the supported shell commands
#include "command.h"
#include "config.h"
#include "shell_inter.h"

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

const int MAX_FILES = 128;
const int rows = 4, width = 84;

// print the directory content : ls helper function
int recDir(char *path) {
    DIR *dir = opendir(path);
    if (!dir) return ERR_STATUS;
    // entry of the directory (file or dir)
    struct dirent *entry;
    // the current directory name
    printf(BLUE "\t%s :\n" reset, path);

    char *files[MAX_FILES]; // entries array
    int j = 0, max_len = 0;

    // iterate through the directoy
    while ((entry = readdir(dir))) {
        // get the extention of the file
        if (strncmp(entry->d_name, ".", 1) != 0) {
            if (j < MAX_FILES) {
                // add the file name to the array
                files[j] = strdup(entry->d_name);
                // compute the current maximum length
                // for the file printing justification
                size_t len = strlen(files[j]);
                if (len > max_len) 
                    max_len = len;
                j++;
            } else {
                break; // files with no extension are not accounted for
            }
        }
    }
    // distance between columns
    int separation = max_len + 4;

    // print files
    for (int k = 0; k < j; k++) {
        printf(GREEN "%-*s" reset, (int)(max_len + separation), files[k]);

        if ((k + 1) % 4 == 0){
            printf("\n");
        }
    }

    // cleanup:
    for (int k = 0; k < j; k++) {
        free(files[k]);
    }

    printf("\n");

    closedir(dir);
    return SUC_STATUS;
}

// print the current working directory
int pwd(char **args) {
    // no arguments are required for this command
    if (args[1] != NULL) {
        fprintf(stderr, "Too many arguments\n");
        return ERR_STATUS;
    } 

    // allocate memory for the path string
    char *path = (char *)malloc((PATH_MAX + 1) * sizeof(char));
    if (path == NULL) {
        fprintf(stderr, "Failed to allocate memory\n");
        return ERR_STATUS;
    }

    // get the current working directory path
    path = getcwd(path, PATH_MAX + 1);
    if (path == NULL) {
        fprintf(stderr, "Failed to get current working directory\n");
        return ERR_STATUS;
    }

    // print and free
    printf("%s\n", path);
    free(path);
    return SUC_STATUS;
}

// list a directoy 
int ls(char **args) {
    // open the current directory
    DIR *dir = opendir(".");
    if (dir == NULL) {
        perror("Error opening current directory");
        return ERR_STATUS;
    }

    char cwd[PATH_MAX];
    // if no arguments are provided then print cwd
    if (args[1] == NULL) {
        // use the recDir function for the current directory to list
        if (recDir(getcwd(cwd, sizeof(cwd))) != 0) {
            return ERR_STATUS;
        }
    } else {
        // get the length of the provided argument
        size_t len = strlen(args[1]);
        // check if the provided directoy path
        // has a valid directory path
        if (len > 0 && args[1][len - 1] != '/') {
            // add the '/' at the end of the path for opening
            // this is assuming UNIX like operating system
            args[1] = (char*)realloc(args[1], len + 2);
            if (args[1] == NULL) {
                fprintf(stderr, "Failed to allocate memory\n");
                return ERR_STATUS;
            }
            strcat(args[1], "/");
        }
        // call recDir on the provided argument
        if (recDir(args[1]) != 0) {
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
        fprintf(stderr, "Usage : echo..[string]..[string]..\n");
        return ERR_STATUS;
    }
    // print provided arguments
    while (args[i] != NULL) {
        printf("%s ", args[i]);
        i++;
    }

    printf("\n");
    
    return (i > 1) ? SUC_STATUS : ERR_STATUS;
}

void cd_usage() {
    fprintf(stderr, "cd usage : cd [directory]\n");
    fprintf(stderr, "cd .. => go to parent dir\n");
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
    if (strcmp(args[1], ".") == 0) {
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
    fprintf(stderr, "rm usage : rm *[file]\n");
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
        fprintf(stderr, "failed to remove file : %s\n", args[1]);
        return NOTSUP_STATUS;
    } 
    
    return SUC_STATUS;
}

void mv_usage() {
    fprintf(stderr, "mv usage : mv *[filename] -- ""newfilename""");
}

// rename a file
int mv(char **args) {
    // at least two arguments are required
    if (args[1] == NULL || args[2] == NULL) {
        mv_usage();
        return NOTSUP_STATUS;
    } else {
        if (rename(args[1], args[2]) != 0) {
            fprintf(stderr, "Error renaming file : %s\n", originalFilename);
            return NOTSUP_STATUS;
        }
    }
    
    return SUC_STATUS;
}

// open a file

int shopen(char **args) {
    if (args[1] == NULL) {
        fprintf(stderr, "open: missing file path");
        return NOTSUP_STATUS;
    } 

    size_t buf_size = MAX_CANON;
    char *command = (char *)malloc((buf_size + 1) * sizeof(char));
    if (command == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
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
            command = (char*)realloc(command, (buf_size + 1) * sizeof(char));
            if (command == NULL) {
                fprintf(stderr, "Memory reallocation failed!\n");
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
        fprintf(stderr, "Error in creating child process: %s\n", strerror(errno));
        return ERR_STATUS;
    } else {
        int status;
        do {
            waitpid(pid, &status, WUNTRACED);
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
    }

    return SUC_STATUS;
}


int touch(char **args) {
    if (args[1] == NULL) {
        fprintf(stderr, "touch: missing file path\n");
        return ERR_STATUS;
    }

    char *filename = (char *)malloc(PATH_MAX + 1);
    if (filename == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        return ERR_STATUS;
    }

    int i = 2;
    strcpy(filename, args[1]);
    strcat(filename, " ");

    while (args[i] != NULL) {
        if (strlen(filename) + strlen(args[i]) + 1 <= MAXNAMLEN) {
            strcat(filename, args[i]);
            if (args[i + 1] != NULL) {
                strcat(filename, " ");
            }
        } else {
            fprintf(stderr, "Concatenation would exceed MAXNAMLEN\n");
            free(filename);
            return ERR_STATUS;
        }
        i++;
    }

    char *dot_position = strrchr(filename, '.');

    if (dot_position != NULL) {
        *dot_position = '\0';
    }

    char extension[255] = "";
    if (dot_position != NULL) {
        strcpy(extension, dot_position + 1);
    }

    char *filepath = strdup(filename);
    free(filename);

    if (strlen(extension) > 0) {
        strcat(filepath, ".");
        strcat(filepath, extension);
    }

    FILE *newFile = fopen(filepath, "w");
    if (newFile == NULL) {
        fprintf(stderr, "Failed to create file: %s\n", filepath);
        free(filepath);
        return ERR_STATUS;
    }

    fclose(newFile);
    free(filepath);

    return SUC_STATUS;
}


int cmake(char **args) {
    if (args[1] == NULL) {
        fprintf(stderr, "cmake: missing filepath\n");
        return ERR_STATUS;
    }

    char *filename = strdup(args[1]);
    char *executableFile = strdup(filename);

    char *dot_position = strrchr(executableFile, '.');

    if (dot_position != NULL) {
        *dot_position = '\0';
    }

    char *envPath = getenv("PATH");
    if (envPath == NULL) {
        fprintf(stderr, "Failed to retrieve environment 'PATH' variable: %s\n", strerror(errno));
        free(filename);
        free(executableFile);
        return ERR_STATUS;
    }

    const char *compCmd = "clang";
    const char delimiter = ':';
    char searchPath[PATH_MAX];
    strncpy(searchPath, envPath, sizeof(searchPath));
    searchPath[sizeof(searchPath) - 1] = '\0';

    char *dir = strtok(searchPath, &delimiter);
    while (dir != NULL) {
        char fullPath[PATH_MAX];
        snprintf(fullPath, sizeof(fullPath), "%s/%s", dir, compCmd);

        if (access(fullPath, X_OK) == 0) {
            pid_t pid = fork();

            if (pid == 0) {
                execl(fullPath, compCmd, filename, "-o", executableFile, (char *)NULL);
                fprintf(stderr, "Error in execl: %s\n", strerror(errno));
                free(filename);
                free(executableFile);
                _exit(EXIT_FAILURE);
            } else if (pid == -1) {
                fprintf(stderr, "Error in creating child process: %s\n", strerror(errno));
                free(filename);
                free(executableFile);
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
    free(filename);
    free(executableFile);
    return SUC_STATUS;
}


int isExecutable(const char *filePath) {
    if (access(filePath, X_OK) == 0) {
        return 1;
    } else {
        return 0;
    }
}


int run(char **args) {
    if (args[1] == NULL) {
        fprintf(stderr, "run: missing executable file path\n");
        return ERR_STATUS;
    }

    if (access(args[1], X_OK) == 0) {
        char *execute_cmd = (char *)malloc(MAXNAMLEN + 3);
        if (execute_cmd == NULL) {
            fprintf(stderr, "Memory allocation failed\n");
            return ERR_STATUS;
        }

        snprintf(execute_cmd, MAXNAMLEN + 3, "./%s", args[1]);

        pid_t pid = fork();

        if (pid == 0) {
            execl(execute_cmd, args[1], (char *)NULL);
            fprintf(stderr, "Error in execl: %s\n", strerror(errno));
            free(execute_cmd);
            _exit(EXIT_FAILURE);
        } else if (pid == -1) {
            fprintf(stderr, "Error in creating child process: %s\n", strerror(errno));
            free(execute_cmd);
            return ERR_STATUS;
        } else {
            int status;
            do {
                waitpid(pid, &status, WUNTRACED);
            } while (!WIFEXITED(status) && !WIFSIGNALED(status));
        }

        free(execute_cmd);
    } else {
        fprintf(stderr, "%s: file is not executable\n", args[1]);
        return ERR_STATUS;
    }

    return SUC_STATUS;
}


int createDirectory(char **args) {
    if (args[1] == NULL) {
        fprintf(stderr, "Usage: createDirectory <directory_name>\n");
        return ERR_STATUS;
    }

    struct stat st = {0};

    if (stat(args[1], &st) == -1) {
        if (mkdir(args[1], 0777) != 0) {
            perror("mkdir");
            return ERR_STATUS;
        }
    }

    struct stat path_stat;
    stat(args[1], &path_stat);

    if (!S_ISDIR(path_stat.st_mode)) {
        fprintf(stderr, "Failed to create directory\n");
        return ERR_STATUS;
    }

    return SUC_STATUS; 
}


int clang(char **args) {
    char *clangCmd = strdup(args[0]);
    if (clangCmd == NULL) {
        fprintf(stderr, "Failed to allocate memory : strdup()\n");
        return ERR_STATUS;
    }

    size_t size = 0; 
    while (args[size] != NULL) {
        size++;
    }
    
    char *c_file = strdup(args[1]);
    if (c_file == NULL) {
        fprintf(stderr, "Failed to allocate memory : strdup()\n");
        free(clangCmd);
        return ERR_STATUS;
    }

    char *exec_file = strdup(args[3]);
    if (exec_file == NULL) {
        fprintf(stderr, "Failed to allocate memory : strdup()\n");
        free(clangCmd);
        free(c_file);
        return ERR_STATUS;
    }

    if (size < 4) {
        fprintf(stderr, "clang: missing arguments\n");
        free(clangCmd);
        free(c_file);
        return ERR_STATUS;
    } 

    char *envPath = getenv("PATH");
    if (envPath == NULL) {
        fprintf(stderr, "Failed to retrieve environment 'PATH' variable: %s\n", strerror(errno));
        free(clangCmd);
        return ERR_STATUS;
    }

    const char delimiter = ':';
    char searchPath[PATH_MAX];
    strncpy(searchPath, envPath, sizeof(searchPath));
    searchPath[sizeof(searchPath) - 1] = '\0';

    char *dir = strtok(searchPath, &delimiter);
    while (dir != NULL) {
        char fullPath[PATH_MAX];
        snprintf(fullPath, sizeof(fullPath), "%s/%s", dir, clangCmd);

        if (access(fullPath, X_OK) == 0) {
            pid_t pid = fork();

            if (pid == 0) {
                execl(fullPath, c_file, args[2], exec_file, (char *)NULL);
                fprintf(stderr, "Error in execl: %s\n", strerror(errno));
                free(clangCmd);
                _exit(EXIT_FAILURE);
            } else if (pid == -1) {
                fprintf(stderr, "Error in creating child process: %s\n", strerror(errno));
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


int comake(char **args) {
    if (args[1] == NULL) {
        fprintf(stderr, "Usage: comake -<filename>-..[OPTION] : <flag>\n");
        return ERR_STATUS;
    }

    char *filename = strdup(args[1]);
    char *objectFile = strdup(filename);

    char *dot_position = strrchr(objectFile, '.');

    if (dot_position != NULL) {
        *dot_position = '\0';
    }
    strcat(objectFile, ".o");

    char *envPath = getenv("PATH");
    if (envPath == NULL) {
        fprintf(stderr, "Failed to retrieve environment 'PATH' variable: %s\n", strerror(errno));
        free(filename);
        free(objectFile);
        return ERR_STATUS;
    }

    const char *compCmd = "clang";
    const char delimiter = ':';
    char searchPath[PATH_MAX];
    strncpy(searchPath, envPath, sizeof(searchPath));
    searchPath[sizeof(searchPath) - 1] = '\0';

    char *dir = strtok(searchPath, &delimiter);
    while (dir != NULL) {
        char fullPath[PATH_MAX];
        snprintf(fullPath, sizeof(fullPath), "%s/%s", dir, compCmd);

        if (access(fullPath, X_OK) == 0) {
            pid_t pid = fork();

            if (pid == 0) {
                execl(fullPath, compCmd, "-c", filename, "-o", objectFile, (char *)NULL);
                fprintf(stderr, "Error in execl: %s\n", strerror(errno));
                _exit(EXIT_FAILURE);
            } else if (pid == -1) {
                fprintf(stderr, "Error in creating child process: %s\n", strerror(errno));
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

    free(filename);
    free(objectFile);
    
    return SUC_STATUS;
}


int build(char **args) {
    char compCmd[] = "clang";
    int i = 1, j = 0;
    char cwd[PATH_MAX];
    
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        perror("getcwd() error");
        return ERR_STATUS;
    }

    int size = 0;
    while (args[size] != NULL) {
        size++;
    }

    if (size < 2) {
        fprintf(stderr, "Not enough arguments provided\n");
        return ERR_STATUS;
    }

    char **files = (char **)malloc(MAX_FILES * sizeof(char *));
    if (files == NULL) {
        fprintf(stderr, "Failed to allocate memory\n");
        return ERR_STATUS;
    }

    for (i = 1; i < size - 1 && j < MAX_FILES; i++, j++) {
        files[j] = strdup(args[i]);
        if (files[j] == NULL) {
            fprintf(stderr, "Failed to allocate memory : strdup()\n");
            for (int k = 0; k < j; k++) {
                free(files[k]);
            }
            free(files);
            return ERR_STATUS;
        }
    }
    files[j] = NULL;  // Null-terminate the files array

    char *exec_file = strdup(args[size - 1]);
    if (exec_file == NULL) {
        fprintf(stderr, "Failed to allocate memory : strdup()\n");
        for (int k = 0; k < j; k++) {
            free(files[k]);
        }
        free(files);
        return ERR_STATUS;
    }

    char *envPath = getenv("PATH");
    if (envPath == NULL) {
        fprintf(stderr, "Failed to get the environment path variable : build()\n");
        for (int k = 0; k < j; k++) {
            free(files[k]);
        }
        free(files);
        free(exec_file);
        return ERR_STATUS;
    }

    const char delimiter = ':';
    char *searchPath = strdup(envPath);
    if (searchPath == NULL) {
        fprintf(stderr, "Failed to allocate memory : strdup()\n");
        for (int k = 0; k < j; k++) {
            free(files[k]);
        }
        free(files);
        free(exec_file);
        return ERR_STATUS;
    }

    char *dir = strtok(searchPath, &delimiter);
    int found = 0;

    while (dir != NULL && !found) {
        char fullPath[PATH_MAX];
        snprintf(fullPath, sizeof(fullPath), "%s/%s", dir, compCmd);

        if (access(fullPath, X_OK) == 0) {
            pid_t pid = fork();

            if (pid == 0) {
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
                    found = 1;
                } else {
                    fprintf(stderr, "Compilation failed\n");
                }
            }
        }
        dir = strtok(NULL, &delimiter);
    }

    for (int k = 0; k < j; k++) {
        free(files[k]);
    }
    free(files);
    free(exec_file);
    free(searchPath);

    if (!found) {
        return ERR_STATUS;
    }

    return SUC_STATUS;
}


int print_head(char *filename, int size) {
    FILE *fp = fopen(filename, "r");
    if (fp == NULL) {
        fprintf(stderr, "file not found : %s\n", filename);
        return ERR_STATUS;
    }     

    const int max_lines = 10, MAX_WIDTH = 256;
    size_t buf_siz = max_lines * MAX_WIDTH * sizeof(char) + 1;
    char *buffer = (char*)malloc(buf_siz);
    
    if (buffer == NULL) {
        fprintf(stderr, "Failed to allocate memory\n");
        return ERR_STATUS;
    }

    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    rewind(fp);

    if (file_size > buf_siz) {
        if (fread(buffer, 1, buf_siz, fp) != buf_siz) {
            fprintf(stderr, "Error reading file\n");
            fclose(fp);
            free(buffer);
            return ERR_STATUS;
        }
        buffer[buf_siz] = '\0';
            
    } else {
        if (fread(buffer, 1, file_size, fp) != file_size) {
            fprintf(stderr, "Error reading file : %s\n", filename);
            free(buffer);
            fclose(fp);
            return ERR_STATUS;
        }
        buffer[file_size] = '\0';
    }
    fclose(fp);

    int lineCounter = 0; 
    int lastline;
    for (int i = 0; i < buf_siz; i++) {
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


int head(char **args) {
    if (args[1] == NULL) {
        fprintf(stderr, "head : missing file path\n");
        return ERR_STATUS;
    } else if (args[2] == NULL) {
        return print_head(args[1], 0);
    } 

    int i = 1;
    while (args[i] != NULL) {
        if (print_head(args[i], 1) != SUC_STATUS) {
            return ERR_STATUS;
        }
        i++;
    }   

    return SUC_STATUS;
}

