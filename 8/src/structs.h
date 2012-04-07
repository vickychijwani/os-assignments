/* this header file contains data structures and types used by our filesystem */
/* nothing else should go in here; configuration should go into config.h */

#ifndef _STRUCTS_H_
#define _STRUCTS_H_

#include "config.h"
#include <stdbool.h>
#include <time.h>

typedef enum {
    FILE_T = 0,
    DIR_T = 1
} file_type_t;

typedef enum {
    RO = 0,
    RW = 1
} file_mode_t;

typedef unsigned short blk_addr_t;  /* required range: [0, BLK_COUNT] */
typedef unsigned char inumber_t;    /* required range: [0, INODE_COUNT] */
typedef unsigned short file_size_t; /* required range: [0, FILE_SIZE_MAX] */
typedef unsigned int offset_t;      /* required range: [0, FS_SIZE] */

/* file attributes structure */
typedef struct {
    file_type_t    type;
    file_mode_t    mode;
    time_t         creation_time;
    file_size_t    size;            /* stores no. of files when type == DIR_T */
} file_attr_t;

/* inode structure */
typedef struct {
    inumber_t      inumber;
    file_attr_t    attr;
    blk_addr_t     blocks_direct[BLKS_DIRECT];
    blk_addr_t     blocks_indirect[BLKS_INDIRECT];
    bool           used;
} inode_t;

/* super block structure */
typedef struct {
    char           fs_name[FS_NAME_MAX + 1];
    offset_t       fs_size;
    inumber_t      root;
    blk_addr_t     block_used_count;
    blk_addr_t     block_free_count;
    inumber_t      inode_free_count;
    blk_addr_t     inode_list;
    blk_addr_t     first_free_block;
} super_block_t;

/* File Table Entry structure  */
typedef struct {
    inode_t inode;
    unsigned char refCount;
    offset_t file_offset;
    blk_addr_t addr;
    void * data;
} table_entry_t;

/* File entry structure in a directory */
typedef struct {
    char name[FILE_NAME_MAX + 1];
    inumber_t inumber;
} file_entry_t;

#endif /* _STRUCTS_H_ */
