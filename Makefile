CC     = gcc
CFLAGS = -Wall -Wextra -g -fsanitize=address -Iinclude
LIBS   = -lm

SRCS = main.c src/library/fs.c src/library/disk.c src/library/dir.c
OBJS = $(SRCS:.c=.o)

TARGET = mfs

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)
