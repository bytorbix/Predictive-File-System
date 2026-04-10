CC     = gcc
CFLAGS = -Wall -Wextra -g -fsanitize=address -Iinclude
LIBS   = -lm

SRCS = main.c src/library/fs.c src/library/disk.c src/library/dir.c src/library/bitmap.c src/library/pfs.c
OBJS = $(SRCS:.c=.o)

TARGET = pfs

all: $(TARGET) pfs_fuse

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) $(LIBS)

mkfs: mkfs.o $(LIB_OBJS)
	$(CC) $(CFLAGS) -o mkfs mkfs.o $(LIB_OBJS) $(LIBS)


%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET) pfs_fuse src/fuse/vfs.o

src/fuse/vfs.o: src/fuse/vfs.c
	$(CC) $(CFLAGS) `pkg-config --cflags fuse` -c src/fuse/vfs.c -o src/fuse/vfs.o

LIB_OBJS = src/library/fs.o src/library/disk.o src/library/dir.o src/library/bitmap.o src/library/pfs.o
pfs_fuse: $(LIB_OBJS) src/fuse/vfs.o
	$(CC) $(CFLAGS) -o pfs_fuse $(LIB_OBJS) src/fuse/vfs.o `pkg-config --libs fuse` $(LIBS)