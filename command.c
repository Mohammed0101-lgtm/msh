
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

const int MAX_FILES       = 128;
const int rows = 4, width = 84;

// print the directory content : ls helper function
int recDir(char *path) {
    DIR *dir = opendir(path);
    
    if (!dir)
        return ERR_STATUS;
    
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
            } 
            else 
                break; // files with no extension are not accounted for
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
    if (args[1]) {
        fprintf(stderr, 
                    "Too many arguments\n");
        
        return ERR_STATUS;
    } 

    // allocate memory for the path string
    char *path = (char *)malloc((PATH_MAX + 1) * sizeof(char));
    if (!path) {
        fprintf(stderr, 
                    "Failed to allocate memory\n");
        
        return ERR_STATUS;
    }

    // get the current working directory path
    path = getcwd(path, PATH_MAX + 1);
    if (!path) {
        fprintf(stderr, 
                    "Failed to get current working directory\n");
        
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
    if (!dir) {
        perror("Error opening current directory");
        return ERR_STATUS;
    }

    char cwd[PATH_MAX];
    // if no arguments are provided then print cwd
    if (!args[1]) 
        // use the recDir function for the current directory to list
        if (recDir(getcwd(cwd, sizeof(cwd))) != 0) 
            return ERR_STATUS;
        
    else {
        // get the length of the provided argument
        size_t len = strlen(args[1]);
        
        // check if the provided directoy path
        // has a valid directory path
        if (len > 0 && args[1][len - 1] != '/') {
            // add the '/' at the end of the path for opening
            // this is assuming UNIX like operating system
            args[1] = (char*)realloc(args[1], len + 2);
            if (!args[1]) {
                fprintf(stderr, 
                            "Failed to allocate memory\n");
                
                return ERR_STATUS;
            }
            
            strcat(args[1], "/");
        }
        // call recDir on the provided argument
        if (recDir(args[1]) != 0) 
            return ERR_STATUS;
    }

    closedir(dir);
    return SUC_STATUS;
}

// this function does not have printing to a file
// functionality , you can add it if you want
int echo(char **args) {
    int i = 1;
    // provide a string to echo it 
    if (!args[1]) {
        fprintf(stderr, 
                    "Usage : echo..[string]..[string]..\n");
        
        return ERR_STATUS;
    }
    
    // print provided arguments
    while (args[i]) 
        printf("%s ", args[i++]);

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
    if (!args[1]) {
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
    fprintf(stderr, "rm usage : rm *[file]\n");
}

// remove a given file
int rm(char **args) {
    // a file path is required
    if (!args[1]) {
        rm_usage();
        return NOTSUP_STATUS;
    }
    // try to remove the file through a syscall
    if (remove(args[1]) != 0) {
        fprintf(stderr, 
                    "failed to remove file : %s\n", args[1]);
        
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
    if (!args[1] || !args[2]) {
        mv_usage();
        return NOTSUP_STATUS;
    } 
    else 
        // try renaming the file through a system call
        if (rename(args[1], args[2]) != 0) {
            fprintf(stderr, 
                        "Error renaming file : %s\n", args[1]);
            
            return NOTSUP_STATUS;
        }
    
    return SUC_STATUS;
}

// open a file

int shopen(char **args) {
    if (!args[1]) {
        fprintf(stderr, 
                    "open: missing file path");
        
        return NOTSUP_STATUS;
    } 

    size_t buf_size = MAX_CANON;
    char *command   = (char *)malloc((buf_size + 1) * sizeof(char));
    
    if (!command) {
        fprintf(stderr, 
                    "Memory allocation failed\n");
        
        return ERR_STATUS;
    }

    strcpy(command, args[0]);
    strcat(command, " ");

    int i = 1;
    while (args[i]) {
        if (strlen(command) + strlen(args[i]) <= buf_size) {
            strcat(command, args[i]);
            
            if (args[i + 1] != NULL) 
                strcat(command, " "); 
        } else {
            buf_size += buf_size;
            command   = (char*)realloc(command, (buf_size + 1) * sizeof(char));
            
            if (!command) {
                fprintf(stderr, 
                            "Memory reallocation failed!\n");
                
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
                    "Error in creating child process: %s\n", strerror(errno));
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
    if (!args[1]) {
        fprintf(stderr, 
                    "touch: missing file path\n");
     
        return ERR_STATUS;
    }

    // create the file
    FILE *new = fopen(args[1], "w");
    if (!new) {
        fprintf(stderr, 
                    "Failed to create file: %s\n", args[1]);
        
        return ERR_STATUS;
    }

    fclose(new);

    return SUC_STATUS;
}

// compile a c program
int cmake(char **args) {
    if (!args[1]) {
        fprintf(stderr, 
                    "cmake: missing filepath\n");
        
        return ERR_STATUS;
    }

    char *filename       = strdup(args[1]);
    char *executableFile = strdup(filename);
    char *dot_position   = strrchr(executableFile, '.');// get the '.' position to remove the extension

    if (dot_position) 
        *dot_position = '\0';

    // get the environment path variable which stores 
    // the path to the clang file which is used to compile
    // the program, if it doesn't find it then the user is 
    // forced to install it
    char *envPath = getenv("PATH"); 
    if (!envPath) {
        fprintf(stderr, 
                "Failed to retrieve environment 'PATH' variable: %s\n", strerror(errno));
        
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
    while (dir) {
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
                            "Error in execl: %s\n", strerror(errno));
              
                free(filename);
                free(executableFile);
                
                _exit(EXIT_FAILURE);
            } else if (pid == -1) {
                // if the forking fails
                fprintf(stderr, 
                            "Error in creating child process: %s\n", strerror(errno));
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

// check weather the file is an executable 
int isExecutable(const char *filePath) {
    if (access(filePath, X_OK)) 
        return 1;
    else 
        return 0;
}

// running an executable file 
int run(char **args) {
    if (!args[1]) {
        fprintf(stderr, 
                    "run: missing executable file path\n");
        
        return ERR_STATUS;
    }

    // if the argument an executable
    if (access(args[1], X_OK)) {
        // construct the executable command
        // for the system call - exporting
        char *exec_cmd = (char *)malloc(MAXNAMLEN + 3);
        if (!exec_cmd) {
            fprintf(stderr, 
                        "Memory allocation failed\n");
            
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
                        "Error in execl: %s\n", strerror(errno));
            
            free(exec_cmd);
            _exit(EXIT_FAILURE);
        } else if (pid == -1) {
            fprintf(stderr, 
                        "Error in creating child process: %s\n", strerror(errno));
            
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
        fprintf(stderr, "%s: file is not executable\n", args[1]);
        return ERR_STATUS;
    }

    return SUC_STATUS;
}

// implementing mkdir
int createDirectory(char **args) {
    // provide the dirname
    if (!args[1]) {
        fprintf(stderr, 
                    "Usage: createDirectory <directory_name>\n");
        
        return ERR_STATUS;
    }

    struct stat st = {0};
     
    // create dir
    if (stat(args[1], &st) == -1) 
        if (mkdir(args[1], 0777) != 0) {
            perror("mkdir");
            return ERR_STATUS;
        }

    // check if the dir making is successful
    struct stat path_stat;
    stat(args[1], &path_stat);

    if (!S_ISDIR(path_stat.st_mode)) {
        fprintf(stderr, "Failed to create directory\n");
        return ERR_STATUS;
    }

    return SUC_STATUS; 
}

// clang cmd in case 
int clang(char **args) {
    char *clangCmd = strdup(args[0]);
    if (!clangCmd) {
        fprintf(stderr,     
                    "Failed to allocate memory : strdup()\n");
        
        return ERR_STATUS;
    }

    size_t size = 0; 
    while (args[size++]) 
        ;
    
    char *c_file = strdup(args[1]);
    if (!c_file) {
        fprintf(stderr, 
                    "Failed to allocate memory : strdup()\n");
        
        free(clangCmd);
        return ERR_STATUS;
    }

    char *exec_file = strdup(args[3]);
    if (!exec_file) {
        fprintf(stderr, 
                    "Failed to allocate memory : strdup()\n");
        
        free(clangCmd);
        free(c_file);
        return ERR_STATUS;
    }

    if (size < 4) {
        fprintf(stderr, 
                    "clang: missing arguments\n");
        
        free(clangCmd);
        free(c_file);
        return ERR_STATUS;
    } 

    char *envPath = getenv("PATH");
    if (!envPath) {
        fprintf(stderr, 
                "Failed to retrieve environment 'PATH' variable: %s\n", strerror(errno));
        
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

        if (access(fullPath, X_OK)) {
            pid_t pid = fork();

            if (pid == 0) {
                execl(fullPath, c_file, args[2], exec_file, (char *)NULL);
                fprintf(stderr, 
                            "Error in execl: %s\n", strerror(errno));
                
                free(clangCmd);
                _exit(EXIT_FAILURE);
            } else if (pid == -1) {
                fprintf(stderr, 
                            "Error in creating child process: %s\n", strerror(errno));
                
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
    if (!args[1]) {
        fprintf(stderr, 
                    "Usage: comake -<filename>-..[OPTION] : <flag>\n");
        
        return ERR_STATUS;
    }

    char *filename     = strdup(args[1]); // copy the filename
    char *objectFile   = strdup(filename); // copy the filename for the object file
    char *dot_position = strrchr(objectFile, '.'); // remove the original file ext

    if (dot_position) 
        *dot_position = '\0';
        
    strcat(objectFile, ".o"); // add the '.o' add the end

    char *envPath = getenv("PATH");
    if (!envPath) {
        fprintf(stderr, 
                "Failed to retrieve environment 'PATH' variable: %s\n", strerror(errno));
        
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

        if (access(fullPath, X_OK)) {
            pid_t pid = fork();

            if (pid == 0) {
                execl(fullPath, compCmd, "-c", filename, "-o", objectFile, (char *)NULL);
                fprintf(stderr, 
                            "Error in execl: %s\n", strerror(errno));
                
                _exit(EXIT_FAILURE);
            } else if (pid == -1) {
                fprintf(stderr, 
                        "Error in creating child process: %s\n", strerror(errno));
                
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
    
    int i = 1
    int j = 0;

    // get the current working directory
    char cwd[PATH_MAX];
    if (!getcwd(cwd, sizeof(cwd))) {
        perror("getcwd() error");
        return ERR_STATUS;
    }

    // get the number of arguments provided
    int size = 0;
    while (args[size++]) 
        ;

    // too few arguments
    if (size < 2) {
        fprintf(stderr, "Not enough arguments provided\n");
        return ERR_STATUS;
    }

    // get the object files into a seperate array
    char **files = (char **)malloc(MAX_FILES * sizeof(char *));
    if (!files) {
        fprintf(stderr, 
                    "Failed to allocate memory\n");
        
        return ERR_STATUS;
    }

    for (i = 1; i < size - 1 && j < MAX_FILES; i++, j++) {
        files[j] = strdup(args[i]); // copy the filename

        // cleanup in case of memory limits
        if (!files[j]) {
            fprintf(stderr, 
                        "Failed to allocate memory : strdup()\n");
            
            for (int k = 0; k < j; k++)
                free(files[k]);
 
            free(files);
 
            return ERR_STATUS;
        }
    }

    files[j] = NULL;  // Null-terminate the files array

    // isolate the executable file
    char *exec_file = strdup(args[size - 1]);
    
    if (!exec_file) {
        fprintf(stderr, 
                    "Failed to allocate memory : strdup()\n");
        
        for (int k = 0; k < j; k++) 
            free(files[k]);
 
        free(files);
 
        return ERR_STATUS;
    }

    // get the environment path variable 
    char *envPath = getenv("PATH");
    if (!envPath) {
        // cleanup
        fprintf(stderr, 
                "Failed to get the environment path variable : build()\n");
        
        for (int k = 0; k < j; k++) 
            free(files[k]);
 
        free(files);
        free(exec_file);
 
        return ERR_STATUS;
    }


    // path delimiter
    const char delimiter = ':';

    // dynamic search path for clang
    char *searchPath = strdup(envPath);
    if (searchPath) {
        fprintf(stderr, 
                    "Failed to allocate memory : strdup()\n");
        
        for (int k = 0; k < j; k++) 
            free(files[k]);
 
        free(files);
        free(exec_file);
 
        return ERR_STATUS;
    }

    // tokenize path for the search
    char *dir = strtok(searchPath, &delimiter);
    int found = 0; // keep track of the state

    while (dir && !found) {

        char fullPath[PATH_MAX];
        snprintf(fullPath, sizeof(fullPath), "%s/%s", dir, compCmd);

        if (access(fullPath, X_OK) == 0) {
            pid_t pid = fork();

            if (pid == 0) {
                // executing all args at once
                char *execArgs[size + 1];
                execArgs[0] = compCmd;
 
                for (i = 0; i < j; i++) 
                    execArgs[i + 1] = files[i];
        
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
 
                if (WIFEXITED(status) && WEXITSTATUS(status) == 0) 
                    found = 1;
                else 
                    fprintf(stderr, 
                                "Compilation failed\n");
            }
        }
        
        dir = strtok(NULL, &delimiter);
    }

    // cleanup
    for (int k = 0; k < j; k++) 
        free(files[k]);
        
    free(files);
    free(exec_file);
    free(searchPath);

    if (!found) 
        return ERR_STATUS; // check state

    return SUC_STATUS;
}

// helper function for the function just below it
int print_head(char *filename, int size) {
    // open file in reading mode
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        fprintf(stderr, "file not found : %s\n", filename);
        return ERR_STATUS;
    }  

    // define current variables
    const int max_lines = 10;
    const int MAX_WIDTH = 256;
    size_t buf_siz      = max_lines * MAX_WIDTH * sizeof(char) + 1;
    
    char *buffer = (char*)malloc(buf_siz);
    if (!buffer) {
        fprintf(stderr, 
                "Failed to allocate memory\n");
        
        return ERR_STATUS;
    }

    // get the file_size
    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    rewind(fp);

    // if the file has fewer lines then ten
    if (file_size > buf_siz) {
        if (fread(buffer, 1, buf_siz, fp) != buf_siz) {
            fprintf(stderr, 
                "Error reading file\n");
            
            fclose(fp);
            free(buffer);
            return ERR_STATUS;
        }
        
        buffer[buf_siz] = '\0';    
    } else {
        if (fread(buffer, 1, file_size, fp) != file_size) {
            fprintf(stderr, 
                    "Error reading file : %s\n", filename);
           
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
        if (buffer[i] == '\n') 
            lineCounter++;
        
        if (lineCounter == 10) {
            lastline = i;
            break;
        }
    }
    
    buffer[lastline + 1] = '\0';
    
    if (size != 0) 
        printf(GREEN "==> %s <==\n" reset, filename);
    
    printf("%s", buffer);
    free(buffer);
    
    return SUC_STATUS;
}

// print the first ten lines of a readable file
int head(char **args) {
    if (!args[1]) {
        fprintf(stderr, 
                "head() : missing file path\n");
        return ERR_STATUS;
    } 
    else if (!args[2]) 
        return print_head(args[1], 0);

    // if there is more then one
    // file is provided in command
    int i = 1;
    while (args[i]) {
        if (print_head(args[i], 1) != SUC_STATUS) 
            return ERR_STATUS;
            
        i++;
    }   

    return SUC_STATUS;
}
