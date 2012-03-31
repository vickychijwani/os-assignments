/* this header file defines configuration parameters for our filesystem */
/* nothing else should go in here; data structures should go into structs.h */

#ifndef _CONFIG_H_
#define _CONFIG_H_

/* max length of fs name */
#define FS_NAME_MAX 255

/* fs size in bytes */
#define FS_SIZE (100 * 1024)

/* block size in bytes */
#define BLK_SIZE 64

/* no. of blocks on fs */
#define BLK_COUNT (FS_SIZE / BLK_SIZE)

/* no. of boot blocks */
#define BLK_BOOT_COUNT 1

/* no. of super blocks */
#define BLK_SUPER_COUNT (((FS_NAME_MAX + 1) / BLK_SIZE) + 1)

/* no. of data blocks */
#define BLK_DATA_COUNT (BLK_COUNT - (BLK_BOOT_COUNT + BLK_SUPER_COUNT + INODE_COUNT))

/* inode parameters */

/* NOTE: (BLKS_DIRECT + BLKS_INDIRECT) * sizeof(blk_addr_t) + size of other
   meta-data in 'struct inode_t' <= FS_SIZE */

/* total no. of inodes; 1 inode = 1 block */
#define INODE_COUNT (0.05 * BLK_COUNT)

/* location of inode list on disk */
#define INODE_LIST_ADDR (BLK_SIZE * (BLK_BOOT_COUNT + BLK_SUPER_COUNT))

/* number of direct blocks */
#define BLKS_DIRECT 8

/* number of indirect blocks */
#define BLKS_INDIRECT 2

/* maximum file size */
#define FILE_SIZE_MAX (BLK_SIZE * (BLKS_DIRECT + BLKS_INDIRECT * \
                                   (BLK_SIZE / sizeof(blk_addr_t))))

#endif /* _CONFIG_H_ */
