# Makefile for dlauncher

# Compiler
CC = gcc

# Compiler flags
CFLAGS = -Wall -Wextra `pkg-config --cflags --libs gtk+-3.0`

# Executable name
TARGET = dlauncher

# Source files
SRCS = dlauncher.c

# Object files
OBJS = $(SRCS:.c=.o)

# Default target
all: $(TARGET)

# Build the executable
$(TARGET): $(OBJS)
	$(CC) -o $@ $(OBJS) $(CFLAGS)

# Compile source files into object files
%.o: %.c
	$(CC) -c $< -o $@ $(CFLAGS)

# Clean up build artifacts
clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean
