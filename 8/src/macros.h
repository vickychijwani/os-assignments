#ifndef _MACROS_H_
#define _MACROS_H_

/* convenience macros */

/* Get the ceil integer of an integer/integer division  */
#define CEIL(a,b) ((a%b)==0 ? (a/b) : ((a/b) + 1))

/* Write the superblock to disk */
#define SUPER_BLOCK_WRITE(super_blk) BLK_SEEK(BLK_SUPER_ADDR), fwrite(super_blk, sizeof(super_block_t), 1, BB_DATA->fs)

/* seek to the block given by @addr (which is a blk_addr_t) */
#define BLK_SEEK(addr) fseek(BB_DATA->fs, (addr) * BLK_SIZE, SEEK_SET)

/* read the data block at @addr into @block (must be of type void *) */
#define BLK_READ_DATA(addr, block) BLK_SEEK(addr), fread(block, BLK_SIZE, 1, BB_DATA->fs)

/* read the indirect block at @addr into @block (must be of type blk_addr_t[]) */
#define BLK_READ_INDIRECT(addr, block) BLK_SEEK(addr), fread(block, sizeof(blk_addr_t), MAX_ADDR_PER_BLOCK, BB_DATA->fs)

/* write the data block @block (of type void *) to disk, at block address @addr */
#define BLK_WRITE_DATA(addr, block) BLK_SEEK(addr), fwrite(block, BLK_SIZE, 1, BB_DATA->fs)

/* write the indirect block @block (of type blk_addr_t[]) to disk, at block address @addr */
#define BLK_WRITE_INDIRECT(addr, block) BLK_SEEK(addr), fwrite(block, sizeof(blk_addr_t), MAX_ADDR_PER_BLOCK, BB_DATA->fs)

/* write the inode block @block (of type inode_t *) to disk, at block address @addr */
#define BLK_WRITE_INODE(addr, block) BLK_SEEK(addr), fwrite(block, sizeof(inode_t), 1, BB_DATA->fs)

/* seek to an inode on the fs, given its inumber */
#define INODE_SEEK(i) fseek(BB_DATA->fs, INODE_LIST_ADDR + (i-1) * BLK_SIZE, SEEK_SET)

/* seek to inode @i and read it into @inode (which is a pointer to inode_t) */
#define INODE_READ(i, inode) INODE_SEEK(i), fread(inode, sizeof(inode_t), 1, BB_DATA->fs)

/* seek to inode @i and write @inode into it (which is a pointer to inode_t) */
#define INODE_WRITE(i, inode) INODE_SEEK(i), fwrite(inode, sizeof(inode_t), 1, BB_DATA->fs)

/* read the directory block at @addr into the file_entry_t array @dir */
#define DIR_BLOCK_READ(dir, addr) BLK_SEEK(addr), fread(dir, sizeof(file_entry_t), MAX_FILES_PER_BLOCK, BB_DATA->fs)

/* read the directory block at @addr into the file_entry_t array @dir */
#define DIR_BLOCK_WRITE(dir, addr) BLK_SEEK(addr), fwrite(dir, sizeof(file_entry_t), MAX_FILES_PER_BLOCK, BB_DATA->fs)

#endif /* _MACROS_H_ */
