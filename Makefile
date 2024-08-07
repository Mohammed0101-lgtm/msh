# Variable definitions
CC = clang
CFLAGS = -Wall -Wextra -g
TARGET = myshell
OBJECTS = main.o shell_inter.o exec.o input.o command.o  

# Default rule to build the target executable
$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) -o $(TARGET)

# Rule to compile object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Rule to clean up build files
.PHONY: clean
clean:
	rm -f $(TARGET) $(OBJECTS)
