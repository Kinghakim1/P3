CC = gcc
CFLAGS = -Wall -Wextra -g
TARGET = myshell
SRCS = mysh.c
OBJS = $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

clean:
	rm -f $(TARGET) $(OBJS)
