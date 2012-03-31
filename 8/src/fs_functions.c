#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "structs.h"


/* convenience macros */

/* get the address of the "current" block on the fs */
#define BLK_CURRENT(fs) (ftell(fs) / BLK_SIZE)


/* internal function prototypes */

void super_init(const char * fs_name);
void block_seek_next(FILE * fs);
blk_addr_t block_get_next(FILE * fs);
void write_zeroes(FILE * fs, int count);


/* global variables */

super_block_t super;
FILE * fs;


/* implementation of public API; refer to the header file for their documentation */

void myformat(const char * fs_name) {
    int i;
    long data_blocks_pos;
    blk_addr_t next;

    /* setup data structures */
    super_init(fs_name);

    /* create storage if it does not already exist, else re-format it */
    fs = fopen(fs_name, "wb");

    /* write 0s to boot block */
    write_zeroes(fs, BLK_SIZE);

    /* write super blocks */
    fwrite(&super, sizeof(super), 1, fs);

    /* write 0s to inode list blocks */
    write_zeroes(fs, INODE_COUNT * sizeof(inode_t));

    /* write 0s to data blocks */
    data_blocks_pos = ftell(fs);
    write_zeroes(fs, BLK_DATA_COUNT * BLK_SIZE);

    /* make every data block point to the next one, for free space mgmt */
    fseek(fs, data_blocks_pos, SEEK_SET);
    for (i = 0; i < INODE_COUNT; i++) {
        next = block_get_next(fs);
        fwrite(&next, sizeof(next), 1, fs);
        block_seek_next(fs);
    }

    fclose(fs);
}



/* internal functions, not part of the public-facing API */

/* initialize the super block data structure (@super)
 *
 * @param fs_name   the name to be given to the fs */

void super_init(const char * fs_name) {
    strcpy(super.fs_name, fs_name);
    super.fs_size = FS_SIZE;

    super.block_used_count = 0;
    super.block_free_count = BLK_DATA_COUNT;
    super.inode_free_count = INODE_COUNT;

    /* inumber of root directory = 1 (0 is a special value) */
    super.root = 1;

    /* block address of inode list */
    super.inode_list = INODE_LIST_ADDR;

    /* location of first free block: just after the inode list */
    super.first_free_block = INODE_LIST_ADDR + INODE_COUNT * BLK_SIZE;
}


/* seek to the next block on @fs
 *
 * @param fs        the storage file for the fs */
void block_seek_next(FILE * fs) {
    blk_addr_t current = BLK_CURRENT(fs);
    fseek(fs, (current + 1) * BLK_SIZE, SEEK_SET);
}


/* get the block address of the next block on @fs
 *
 * @param fs        the storage file for the fs */

blk_addr_t block_get_next(FILE * fs) {
    blk_addr_t current = BLK_CURRENT(fs);
    return (current + 1);
}


/* write @count zeroes to @fs, starting from the current offset
 *
 * @param fs        the storage file for the fs
 * @param count     the number of times to write zeroes */

void write_zeroes(FILE * fs, int count) {
    int i;
    for (i = 0; i < count; i++) {
        fprintf(fs, "%c", '\x00');
    }
}
