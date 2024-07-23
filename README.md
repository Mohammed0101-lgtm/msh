This repo contains the implementation for a basic interactive interface for a command interpreter also known as a shell, 
it should be notable that this does not create a shell from the ground up , the commands are implemented from scratch, but
the main program is running on the existing shell where the executable is executed.

## reading input :
    the input for the shell (command entered by the user) is registered as a simple string,
    the sequence of characters is terminated when the user presses 'ENTER'.
    '''
        ..
        // allocate memory for the input
        char *line = malloc(MAX_WIDTH);
        ...
        // .. read input
        fgets(line, MAX_WIDTH, stdin);
        ...
    '''

## input tokenisation 
        the commands should be (in most cases) seperated by spaces, that means that the tokens are 
    determined by the common space delimiters : '\t', '\r', '\n', ' '.
    Tokens are stored in an array for easy random access and the array is NULL terminated (adding
    a NULL pointer to the end to mark the end in a variable size array).
    '''
        // allocate memory for the tokens array 
        char **tokens = malloc(MAX_TOKENS);
        ...
        // store the tokens in the array
        char *token = strtok(line, TOKEN_DELIMITERS);
        int i = 0;
        while (token !=  NULL) {
            tokens[i++] = token;
            token = strtok(NULL, TOKEN_DELIMITERS);
        }
        // NULL terminate the array
        tokens[i] = NULL;
    '''

## input processing 
      Now that the arguments are saved in array, we will always (for the time being) consider the first 
    argument as the command to execute, while the rest of the arguments in the line are considered
    'command line arguments' for the command.
    '''
        char **args = tok_line(input);
        char *command = args[0];
    '''

## executing commands 
        commands are compared with a predefined list of supported commands, even in modern shells
    the list isn't that long for performance issues with linear search. If the command is found
    then we pass the corresponding function reference in the list to the execution.

    for example:

    '''
        static char *builtin_func_list[] = { "ls", "cd"};
        static int (*builtin_func[])(char **) = { &ls, &cd}
    ''' 

    We need to make sure that each commad string is alligned with the corresponding function reference
    so that we don't wind up executing a different command
    
## process forking
        Shells normally do not execute a command and then exit, you don't have to load the shell every
    time you execute the command, so we need to make the execution process seperate from the main shell
    process.
    For that we will create a child process to run the command under the shell process and handle
    errors and file operations seperately

    '''
        // this will fork the current process and create a child process 
        // returning the process id for the child process 
        pid_t process_id = fork();

        // we then execute the command seperately
    '''

## command functions 
        I defined the command functions in a seperate file, making adding and modifying functions easier.
    You just open the command file , implement/modify the command function , then go to the 'exec.c' file
    and add the function reference and you're good to go

## contact 
    For further assistance and collaboration :
    email : mohamed.bayt.alkhalij@gmail.com
    
    
