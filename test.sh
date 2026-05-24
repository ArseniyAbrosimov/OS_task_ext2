#!/bin/bash
set -e


truncate --size 100M ext2.img
/sbin/mkfs.ext2 -b 2048 -F ext2.img

mkdir -p mnt_test
sudo mount -t ext2 ext2.img mnt_test
sudo chown $USER mnt_test

echo "Test" > mnt_test/base.txt

truncate -s 6G mnt_test/sparse.bin

echo "END_OF_LARGE_SPARSE" | dd of=mnt_test/sparse.bin bs=1 seek=6442450000 count=20 status=none

mkdir -p mnt_test/dir/dir3
mkdir -p mnt_test/dir2
echo "Dir test" > mnt_test/dir/dir3/dir_test.txt

INODE_BASE=$(stat -c %i mnt_test/base.txt)
INODE_SPARSE=$(stat -c %i mnt_test/sparse.bin)
INODE_DIR=$(stat -c %i mnt_test/dir)

SUM_BASE=$(sha512sum mnt_test/base.txt | awk '{print $1}')
SUM_SPARSE=$(sha512sum mnt_test/sparse.bin | awk '{print $1}')

sudo umount mnt_test


echo -e "Test #1 - base"
./inode_info ext2.img $INODE_BASE
./inode_print ext2.img $INODE_BASE > base.txt

PARSED_BASE_SUM=$(sha512sum base.txt | awk '{print $1}')
if [ "$SUM_BASE" == "$PARSED_BASE_SUM" ]; then
    echo -e "Control sums equal"
else
    echo -e "Control sums differ"; exit 1
fi


echo -e "Test #2 - sparse"
PARSED_SPARSE_SUM=$(./inode_print ext2.img $INODE_SPARSE | sha512sum | awk '{print $1}')
if [ "$SUM_SPARSE" == "$PARSED_SPARSE_SUM" ]; then
    echo -e "Control sums equal"
else
    echo -e "Control sums differ"; exit 1
fi

echo -e "Test #3 - directory"
./inode_print ext2.img $INODE_DIR | ./print_dir > parsed_dir.txt
cat parsed_dir.txt
if grep -q "dir" parsed_dir.txt; then
    echo -e "Dir found"
else
    echo -e "Dir not found"; exit 1
fi

echo -e "Test #4 - loop"
LOOP_DEV=$(sudo losetup -f)
sudo losetup $LOOP_DEV ext2.img
sudo chmod 644 $LOOP_DEV

./inode_info "$LOOP_DEV" "$INODE_BASE" > /dev/null
LOOP_SUM=$(./inode_print "$LOOP_DEV" "$INODE_BASE" | sha512sum | awk '{print $1}')

if [ "$SUM_BASE" == "$LOOP_SUM" ]; then
    echo -e "Successfully read loop"
else
    echo -e "Error while reading loop"; exit 1
fi

sudo losetup -d $LOOP_DEV

echo -e "All test finished"

rmdir mnt_test