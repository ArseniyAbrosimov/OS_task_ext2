#include "ext2.h"
#include <stdio.h>

int main(int argc, char **argv) {
  if (argc != 3) {
    fprintf(stderr, "Wrong amount of args\n");
    return 1;
  }

  int fd = open(argv[1], O_RDONLY);
  if (fd < 0) {
    perror("open");
    return 1;
  }

  uint32_t inode_num = atoi(argv[2]);

  struct ext2_super s;
  if (pread(fd, &s, sizeof(s), 1024) != sizeof(s) ||
      le16toh(s.sign) != EXT2_SIGN) {
    fprintf(stderr, "Invalid ext2\n");
    close(fd);
    return 1;
  }

  struct ext2_inode inode;
  find_inode(fd, &s, inode_num, &inode);

  uint64_t file_size = le32toh(inode.size);
  if (((le16toh(inode.type) & 0xF000) == EXT2_S_IFREG) &&
      le32toh(s.major_ver) >= EXT2_FIRST_REV) {
    file_size |= ((uint64_t)le32toh(inode.dir_acl_or_upper_size) << 32);
  }

  printf("Ext2 inode\n");
  printf("Mode:  %04o\n", le16toh(inode.type));
  printf("User id:   %u\n", le16toh(inode.user_id));
  printf("Size:  %llu\n", (unsigned long long)file_size);
  printf("Last Access Time:  %llu\n", (unsigned long long)inode.access_time);
  printf("Creation Time:  %llu\n", (unsigned long long)inode.access_time);
  printf("Last Modification Time:  %llu\n",
         (unsigned long long)inode.modify_time);
  printf("Deletion Time:  %llu\n", (unsigned long long)inode.delete_time);
  printf("Group id:   %u\n", le16toh(inode.group_id));
  printf("Links: %u\n", le16toh(inode.hlinks));
  printf("Disc sectors: %u\n", le32toh(inode.sectors));

  printf("Block addresses:\n");
  for (int i = 0; i < 12; i++) {
    if (inode.blocks[i] != 0)
      printf("  %d: %u\n", i, le32toh(inode.blocks[i]));
  }

  printf("  Indirect:\n");
  if (inode.blocks[12] != 0)
    printf("    %u\n", le32toh(inode.blocks[12]));
  if (inode.blocks[13] != 0)
    printf("    %u\n", le32toh(inode.blocks[13]));
  if (inode.blocks[14] != 0)
    printf("    %u\n", le32toh(inode.blocks[14]));

  close(fd);
  return 0;
}
