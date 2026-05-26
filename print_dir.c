#include <endian.h>
#include <stdint.h>
#include <stdio.h>

int main() {
    uint8_t buf[8];
    while (fread(buf, 1, 8, stdin) == 8) {
        uint32_t inode = le32toh(*(uint32_t*)buf);
        uint16_t rec_len = le16toh(*(uint16_t*)(buf+4));
        uint8_t name_len = buf[6];

        char name[256] = {0};
        if (name_len > 0) {
            if (fread(name, 1, name_len, stdin) != name_len)
                break;
        }

        if (inode != 0) {
            printf("%u - %s\n", inode, name);
        }

        uint32_t remaining = rec_len - 8 - name_len;
        if (remaining > 0) {
            for (uint32_t i = 0; i < remaining; i++)
                fgetc(stdin);
        }
    }

    return 0;
}
