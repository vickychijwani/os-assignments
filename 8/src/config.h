/* this header file defines configuration parameters for our filesystem */
/* nothing else should go in here; data structures should go into structs.h */

#ifndef _CONFIG_H_
#define _CONFIG_H_

/* filesystem-wide parameters */
/* -------------------------- */

/* max length of fs name */
#define FS_NAME_MAX 255

/* Inode number of root */
#define ROOT_INODE_NUMBER 1

/* fs size in bytes */
#define FS_SIZE (100 * 1024)

/* max length of a filename */
#define FILE_NAME_MAX 10

/*  max path depth */
#define PATH_DEPTH_MAX 10

/* max length of a path */
#define PATH_LEN_MAX 255

/* no of max open files at one time */
#define MAX_OPEN_FILES (INODE_COUNT)


/* block parameters */
/* ---------------- */

/* block size in bytes */
#define BLK_SIZE 64

/* no. of blocks on fs */
#define BLK_COUNT (FS_SIZE / BLK_SIZE)

/* no. of boot blocks */
#define BLK_BOOT_COUNT 1

/* no. of super blocks */
#define BLK_SUPER_COUNT (((FS_NAME_MAX + 1) / BLK_SIZE) + 1)

/* no. of inode blocks; 1 inode == 1 block */
#define BLK_INODE_COUNT INODE_COUNT

/* no. of data blocks */
#define BLK_DATA_COUNT (BLK_COUNT - (BLK_BOOT_COUNT + BLK_SUPER_COUNT + INODE_COUNT))

/* max file entries in one block */
#define MAX_FILES_PER_BLOCK (BLK_SIZE/sizeof(file_entry_t))

/* max block addresses per block */
#define MAX_ADDR_PER_BLOCK (BLK_SIZE/sizeof(blk_addr_t))


/* inode parameters */
/* ---------------- */

/* NOTE: (BLKS_DIRECT + BLKS_INDIRECT) * sizeof(blk_addr_t) + size of other
   meta-data in 'struct inode_t' <= FS_SIZE */

/* total no. of inodes, as a % of total block count; 1 inode == 1 block */
#define INODE_COUNT (int) (0.05 * BLK_COUNT)

/* number of direct blocks per inode */
#define BLKS_DIRECT 8

/* number of indirect blocks per inode */
#define BLKS_INDIRECT 2

/* maximum file size */
#define FILE_SIZE_MAX (BLK_SIZE * (BLKS_DIRECT + BLKS_INDIRECT * \
                                   MAX_ADDR_PER_BLOCK))


/* important disk locations */
/* ------------------------ */

/* location of super block on disk */
#define BLK_SUPER_ADDR (BLK_SIZE * BLK_BOOT_COUNT)

/* location of inode list on disk */
#define INODE_LIST_ADDR (BLK_SIZE * (BLK_BOOT_COUNT + BLK_SUPER_COUNT))

/* location of data blocks on disk */
#define BLK_DATA_ADDR (BLK_SIZE * (BLK_BOOT_COUNT + BLK_SUPER_COUNT + BLK_INODE_COUNT))


#endif /* _CONFIG_H_ */
