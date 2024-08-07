CC = clang
CFLAGS = -Wall -Wextra -g
TARGET = myshell
OBJECTS = main.o shell_inter.o exec.o input.o command.o  

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) -o $(TARGET)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean
clean:
	rm -f $(TARGET) $(OBJECTS)
