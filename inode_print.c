#include "ext2.h"

uint32_t block_size;
int fd;

void print_zeros(uint64_t to_write) {
    uint64_t chunk_size = 1048576;
    char *zeros = (char *)calloc(1, chunk_size);
    if (!zeros) {
        perror("calloc"); exit(1);
    }

    uint64_t written = 0;
    while (written < to_write) {
        uint64_t buf_size = chunk_size;
        if (to_write - written < chunk_size) {
            buf_size = to_write - written;
        }

        size_t res = fwrite(zeros, 1, buf_size, stdout);
        if (res != buf_size) {
            perror("write");
            exit(1);
        }
        written += buf_size;
    }
    free(zeros);
}

void print_block(uint32_t sparse, uint64_t *rem) {
    if (*rem <= 0) return;

    uint64_t to_write = *rem;

    if (*rem >= block_size) {
        to_write = block_size;
    }

    if (sparse == 0) {
        print_zeros(to_write);
    } else {
        char *buf = (char *)(malloc(block_size));
        if (!buf) {
            perror("malloc");
            exit(1);
        }
        pread(fd, buf, block_size, (off_t)sparse * block_size);
        fwrite(buf, 1, to_write, stdout);
        free(buf);
    }
    *rem -= to_write;
}

void walk_indirect(uint32_t sparse, int level, uint64_t *rem) {
    if (*rem <= 0) return;

    if (level == 0) {
        print_block(sparse, rem);
        return;
    }

    if (sparse == 0) {
        uint64_t bytes = block_size;
        for (int i = 0; i < level; i++) bytes *= (block_size / 4);
        uint64_t to_write = *rem;
        if (*rem >= bytes) {
            to_write = bytes;
        }
        print_zeros(to_write);
        *rem -= to_write;
        return;
    }

    uint32_t *ptrs = (uint32_t*)malloc(block_size);
    if (!ptrs) {
        perror("malloc");
        exit(1);
    }
    pread(fd, ptrs, block_size, (off_t)sparse * block_size);

    uint32_t num_ptrs = block_size / 4;
    for (uint32_t i = 0; i < num_ptrs; i++) {
        walk_indirect(le32toh(ptrs[i]), level - 1, rem);
    }
    free(ptrs);
}

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "Wrong amount of args\n");
        return 1;
    }

    fd = open(argv[1], O_RDONLY);
    if (fd < 0) {
        perror("open");
        return 1;
    }

    uint32_t inode_num = atoi(argv[2]);

    struct ext2_super s;
    if (pread(fd, &s, sizeof(s), 1024) != sizeof(s) || le16toh(s.sign) != EXT2_SIGN) {
        fprintf(stderr, "Invalid ext2\n");
        close(fd); return 1;
    }

    block_size = 1024 << le32toh(s.block_size);

    struct ext2_inode inode;
    find_inode(fd, &s, inode_num, &inode);

    uint64_t file_size = le32toh(inode.size);
    if (((le16toh(inode.type) & 0xF000) == EXT2_S_IFREG)  &&
        le32toh(s.major_ver) >= EXT2_FIRST_REV) {
        file_size |= ((uint64_t)le32toh(inode.dir_acl_or_upper_size) << 32);
        }

    uint64_t rem = file_size;
    for (int i = 0; i < 12; i++) {
        print_block(le32toh(inode.blocks[i]), &rem);
    }
    walk_indirect(le32toh(inode.blocks[12]), 1, &rem);
    walk_indirect(le32toh(inode.blocks[13]), 2, &rem);
    walk_indirect(le32toh(inode.blocks[14]), 3, &rem);

    close(fd);
    return 0;
}
