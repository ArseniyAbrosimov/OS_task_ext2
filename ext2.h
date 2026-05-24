#ifndef HW12_EXT2_H
#define HW12_EXT2_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <endian.h>
#include <unistd.h>
#include <sys/stat.h>

#define EXT2_SIGN 0xEF53
#define EXT2_FIRST_REV  1
#define EXT2_FEATURE_RO_COMPAT_LARGE_FILE 0x0002
#define EXT2_S_IFREG 0x8000

struct ext2_super {
    uint32_t inodes_count;
    uint32_t blocks_count;
    uint32_t res_blocks_count;
    uint32_t unalloc_blocks_count;
    uint32_t unalloc_inodes_count;
    uint32_t starting_block;
    uint32_t block_size;
    uint32_t frag_size;
    uint32_t blocks_per_group;
    uint32_t frags_per_group;
    uint32_t inodes_per_group;
    uint32_t mnt_time;
    uint32_t write_time;
    uint16_t mnt_count;
    uint16_t max_mnt_count;
    uint16_t sign;
    uint16_t state;
    uint16_t errors;
    uint16_t minor_ver;
    uint32_t lastcheck;
    uint32_t checkinterval;
    uint32_t os;
    uint32_t major_ver;
    uint16_t user_id;
    uint16_t group_id;
    // если major version > 0
    uint32_t non_res_inode;
    uint32_t inode_size;
    uint16_t block_group_nr;
    uint32_t feature_compat;
    uint32_t feature_incompat;
    uint32_t feature_ro_compat;
} __attribute__((packed));

struct ext2_inode {
    uint16_t type;
    uint16_t user_id;
    uint32_t size;
    uint32_t access_time;
    uint32_t create_time;
    uint32_t modify_time;
    uint32_t delete_time;
    uint16_t group_id;
    uint16_t hlinks;
    uint32_t sectors;
    uint32_t flags;
    uint32_t os;
    uint32_t blocks[15];
    uint32_t generation;
    uint32_t file_acl;
    uint32_t dir_acl_or_upper_size;
} __attribute__((packed));

struct ext2_group {
    uint32_t block_bitmap;
    uint32_t inode_bitmap;
    uint32_t inode_table;
    uint16_t unalloc_block_count;
    uint16_t unalloc_inode_count;
    uint16_t dirs_count;
    uint16_t res1;
    uint32_t res2[3];
} __attribute__((packed));


static void find_inode(int fd, struct ext2_super *s, uint32_t inode_id, struct ext2_inode *inode_ptr) {
    // Для удобства документация c osdev
    //Read the Superblock to find the size of each block, the number of blocks per group, number Inodes per group, and the starting block of the first group (Block Group Descriptor Table).
    //Determine which block group the inode belongs to.
    //Read the Block Group Descriptor corresponding to the Block Group which contains the inode to be looked up.
    //From the Block Group Descriptor, extract the location of the block group's inode table.
    //Determine the index of the inode in the inode table.
    //Index the inode table (taking into account non-standard inode size).

    uint32_t block_size = 1024 << le32toh(s->block_size);
    uint32_t inodes_per_group = le32toh(s->inodes_per_group);

    uint16_t inode_size = 128;
    if (le32toh(s->major_ver) > 0) {
        inode_size = le16toh(s->inode_size);
    }

    uint32_t group = (inode_id - 1) / inodes_per_group;
    uint32_t index = (inode_id - 1) % inodes_per_group;

    //The table is located in the block immediately following the Superblock.
    //So if the block size (determined from a field in the superblock) is 1024 bytes per block, the Block Group Descriptor Table will begin at block 2.
    //For any other block size, it will begin at block 1.
    //Remember that blocks are numbered starting at 0, and that block numbers don't usually correspond to physical block addresses.

    //или проще:
    uint32_t table_start = (le32toh(s->starting_block) + 1) * block_size;
    uint32_t table = table_start + group * sizeof(struct ext2_group);

    struct ext2_group group_table;
    if (pread(fd, &group_table, sizeof(group_table), table) != sizeof(group_table)) {
        perror("pread group");
        exit(1);
    }

    uint64_t inode_table_offset = (uint64_t)le32toh(group_table.inode_table) * block_size;
    uint64_t inode_offset = inode_table_offset + (uint64_t)index * inode_size;

    if (pread(fd, inode_ptr, sizeof(struct ext2_inode), inode_offset) != sizeof(struct ext2_inode)) {
        perror("pread inode");
        exit(1);
    }
}

#endif //HW12_EXT2_H
