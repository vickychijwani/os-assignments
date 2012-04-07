#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <libgen.h>

#include "config.h"
#include "structs.h"

void super_init(const char * fs_name,super_block_t * super);
void block_seek_next(FILE * fs);
blk_addr_t block_get_next(FILE * fs);
void write_zeroes(FILE * fs, int count);

void myformat(const char * fs_name) {
    int i;
    long data_blocks_pos;
    blk_addr_t next;
    FILE * fs;

    /* setup data structures */
    super_block_t super;
    super_init(fs_name,&super);

    /* create storage if it does not already exist, else re-format it */
    fs = fopen(fs_name, "wb");

    /* write 0s to entir filesystem */
    write_zeroes(fs, FS_SIZE);
    fseek(fs, 0, SEEK_SET);

    /* write 0s to boot block */
    write_zeroes(fs, BLK_SIZE);

    /* write super blocks */
    fwrite(&super, sizeof(super_block_t), 1, fs);
    block_seek_next(fs);

    /*Write the rootdir structure to the superblock */
    inode_t rootdir;
    rootdir.inumber = 1;
    rootdir.used = true;
    rootdir.attr.size = 0;
    rootdir.attr.type = DIR_T;
    rootdir.attr.creation_time = time(NULL);
    for(i = 0; i<BLKS_DIRECT; i++)
        rootdir.blocks_direct[i] = 0;
    for(i = 0; i<BLKS_INDIRECT; i++)
        rootdir.blocks_indirect[i] = 0;
    fwrite(&rootdir,sizeof(inode_t),1,fs);
    block_seek_next(fs);
    /* write the rest of the inodes to inode list blocks */
    inode_t inode;
    int j;
    for(j = 1;j<INODE_COUNT;j++) {
        inode.inumber = j+1;
        inode.used = false;
        inode.attr.type = 0;
        inode.attr.mode = 0;
        inode.attr.creation_time = 0;
        inode.attr.size = 0;
        for(i = 0; i<BLKS_DIRECT; i++)
            rootdir.blocks_direct[i] = 0;
        for(i = 0; i<BLKS_INDIRECT; i++)
            rootdir.blocks_indirect[i] = 0;
        fwrite(&inode,sizeof(inode_t),1,fs);
        block_seek_next(fs);
    }
    /* write 0s to data blocks */
    data_blocks_pos = ftell(fs);
    write_zeroes(fs, BLK_DATA_COUNT * BLK_SIZE);

    /* make every data block point to the next one, for free space mgmt */
    fseek(fs, data_blocks_pos, SEEK_SET);
    for (i = 0; i < BLK_DATA_COUNT; i++) {
        next = block_get_next(fs);
        fwrite(&next, sizeof(next), 1, fs);
        block_seek_next(fs);
    }

    fclose(fs);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: ./format <filesystem_name>\n");
        return -1;
    }

    myformat(argv[1]);

    return 0;
}


/* internal functions */

/* initialize the super block data structure (@super)
 *
 * @param fs_name   the name to be given to the fs */

void super_init(const char * fs_name,super_block_t * super) {
    strcpy(super->fs_name, fs_name);
    super->fs_size = FS_SIZE;

    super->block_used_count = 0;
    super->block_free_count = BLK_DATA_COUNT;
    super->inode_free_count = INODE_COUNT;

    /* inumber of root directory = 1 (0 is a special value) */
    super->root = 1;

    /* block address of inode list */
    super->inode_list = INODE_LIST_ADDR;

    /* location of first free block: just after the inode list */
    super->first_free_block = BLK_DATA_ADDR / BLK_SIZE;
}

/* seek to the next block on @fs
 *
 * @param fs        the storage file for the fs */
void block_seek_next(FILE * fs) {
    blk_addr_t current = ftell(fs) / BLK_SIZE;
    fseek(fs, (current + 1) * BLK_SIZE, SEEK_SET);
}

/* get the block address of the next block on @fs
 *
 * @param fs        the storage file for the fs */
blk_addr_t block_get_next(FILE * fs) {
    blk_addr_t current = ftell(fs) / BLK_SIZE;
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
