CC = gcc
CFLAGS = -Wall -Wextra -g

all: inode_info inode_print print_dir

ext2_info: inode_info.c ext2.h
	$(CC) $(CFLAGS) -o $@ $<

ext2_cat: inode_print.c ext2.h
	$(CC) $(CFLAGS) -o $@ $<

ext2_ls: print_dir.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(BINS)

test: all
	bash test.sh