#include "params.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <libgen.h>
#include <math.h>
#include <errno.h>

#include "config.h"
#include "structs.h"
#include "logger.h"
#include "macros.h"

/* internal function prototypes */

void dir_entry_delete(const char * path);
blk_addr_t dir_entry_move_last(inode_t * inode, file_entry_t * entry);
void inode_free(inode_t * inode);
void block_free(blk_addr_t addr);
void block_load_next(int fd);
blk_addr_t block_allocate(int fd);
void write_zeroes(FILE * fs, int count);
void error_exit(char * errorStr);
blk_addr_t get_first_free_dir_block(inode_t * inode);
blk_addr_t get_free_block(void);
inumber_t get_free_inode(inode_t * free_inode);
inumber_t get_inode_in_block(blk_addr_t block_offset,char * name,int * count,file_type_t file_type);
inumber_t get_inode_from_name(inumber_t parent_inode_no,char * name,file_type_t file_type);
inumber_t get_inode_from_path(const char * filepath, inode_t * inode);
int strsplit(char *str , char * delim, char ** arr);
int is_filetype_same(inumber_t inode_no,file_type_t file_type);
int file_table_insert(inumber_t inode);
int file_table_delete(int fd);
inumber_t create_file(inode_t parent_inode,char * name,file_type_t file_type,file_mode_t mode);
void fs_check_mounted(void);
void print_filenames_in_block(blk_addr_t offset,int * count);
void get_filenames_in_block(blk_addr_t block_offset,int * file_count, char *files[], int * count);
int delete_filename_in_block(blk_addr_t addr,unsigned int * entry_count,inode_t * parent_inode,char * name);

int mymount(char * fs_name) {
    if(BB_DATA->super_blk != NULL)
        return 1;

    /* Load the superblock in memory */
    BB_DATA->super_blk = (super_block_t *)malloc(sizeof(super_block_t));
    BB_DATA->fs = fopen(fs_name,"r+");
    fseek(BB_DATA->fs,BLK_SUPER_ADDR,SEEK_SET);
    fread(BB_DATA->super_blk,sizeof(super_block_t),1,BB_DATA->fs);

    /* Initialise the Open File Table */
    BB_DATA->openFileTable = (table_entry_t **)malloc(MAX_OPEN_FILES * sizeof(table_entry_t *));
    int i ;
    for(i = 0;i<MAX_OPEN_FILES;i++)
        BB_DATA->openFileTable[i] = NULL;

    /* Initialise the first entry of file table with root inode */
    BB_DATA->openFileTable[0] = (table_entry_t *)malloc(sizeof(table_entry_t));
    inode_t root_inode;
    INODE_READ(ROOT_INODE_NUMBER,&root_inode);
    BB_DATA->openFileTable[0]->inode = root_inode;
    BB_DATA->openFileTable[0]->refCount = 1;

    return 0;
}

void myunmount(char * fs_name)
{
    (void) fs_name;

    /* Write the superblock back to disk*/
    fseek(BB_DATA->fs,BLK_SUPER_ADDR,SEEK_SET);
    fwrite(BB_DATA->super_blk,sizeof(super_block_t),1,BB_DATA->fs);

    /* Close the file */
    fclose(BB_DATA->fs);

    /* Make the superblock and fileTable null*/
    free(BB_DATA->super_blk);
    free(BB_DATA->openFileTable);
    BB_DATA->openFileTable = NULL;
    BB_DATA->super_blk = NULL;
}

int myopen(const char * filepath, char * mode_str) {
    fs_check_mounted();

    file_mode_t mode;
    if(strcmp(mode_str, "r")==0)
        mode = RO;
    else if(strcmp(mode_str, "w")==0)
        mode = RW;
    else
	{
            /* fprintf(stderr,"Error in myopen : Unknown mode\n"); */
            return -EINVAL;
	}

    /* Get the inode number of the file */
    inode_t file_inode;
    inumber_t inode_no = get_inode_from_path(filepath, &file_inode);

    if (inode_no > 0 && inode_no < INODE_COUNT + 1 && file_inode.attr.type == DIR_T) {
        return -EISDIR;
    }
    else if(inode_no >= INODE_COUNT || strcmp(filepath, "") == 0)
	{
            /* Path is incorrect */
            /* fprintf(stderr,"Error in myopen : Incorrect file path\n"); */
            return -ENOENT;
	}
    else if(inode_no == 0)
	{
            /*File does not exist : create a new file */
            inode_no = create_file(file_inode, basename(strdup(filepath)), FILE_T, mode);
            log_msg("open inode_no : %d\n", inode_no);
	}
    else
	{
            /* Check for mode mismatch of existing file */
            inode_t inode;
            INODE_READ(inode_no,&inode);
            if(inode.attr.mode != mode)
		{
                    /* fprintf(stderr,"Error in myopen : Mode Mismatch\n"); */
                    return -EACCES;
		}
	}

    /* make an entry in open file table and return fd */
    if(inode_no == 0)
	{
            /* fprintf(stderr,"Error in  myopen : error creating new file\n"); */
            log_msg("myopen ENOENT here\n");
            return -ENOENT;
	}
    return (file_table_insert(inode_no));
}


int mymkdir(const char * name) {

    fs_check_mounted();

    inode_t inode;
    inumber_t inumber;
    if (name == NULL || strcmp(name, "") == 0) {
        printf("mkdir: no argument given\n");
        return -1;
    }

    inumber = get_inode_from_path(name,&inode);
    if(inumber > 0 && inumber<INODE_COUNT)
	{
            fprintf(stderr,"mkdir : Directory with this name already exists\n");
            return -EEXIST;
	}
    else if(inumber >= INODE_COUNT)
	{
            fprintf(stderr,"mkdir : Incorrect filepath\n");
            return -ENOTDIR;
	}

    if(create_file(inode,basename(strdup(name)),DIR_T,RW) == 0)
	{
            fprintf(stderr,"mkdir : Error Creating directory\n");
            return -EIO;
	}
    return 0;
}

void myls(const char * name)
{
    fs_check_mounted();
    inumber_t inumber;
    inode_t inode;

    if(name==NULL || strcmp(name,"")==0 || strcmp(name,"/")==0) {

        inumber = ROOT_INODE_NUMBER;
    }
    else
        inumber = get_inode_from_path(name,&inode);
    if(inumber == 0 || inumber>=INODE_COUNT)
	{
            fprintf(stderr,"Incorrect path\n");
            return;
	}
    /* Listing the Directory contents */
    INODE_READ(inumber, &inode);

    int file_count = inode.attr.size;
    int blk_count = CEIL(file_count,MAX_FILES_PER_BLOCK);

    unsigned int i,j;
    /*Print the filenames in the direct blocks */
    for( i=0; (i<BLKS_DIRECT && blk_count>0);i++,blk_count--)
        print_filenames_in_block(inode.blocks_direct[i],&file_count);

    /* Print the fielnames in the indirect blocks */
    for(i = 0;i<BLKS_INDIRECT;i++)
	{
            if(inode.blocks_indirect[i] == 0)
                return;

            blk_addr_t blk_addresses[MAX_ADDR_PER_BLOCK];
            BLK_READ_INDIRECT(inode.blocks_indirect[i],blk_addresses);
            for(j = 0;(j<MAX_ADDR_PER_BLOCK && blk_count>0);j++,blk_count--)
		{
                    print_filenames_in_block(blk_addresses[j],&file_count);
		}
 	}
}

int myreaddir(const char * name, char *files[])
{
    fs_check_mounted();
    inumber_t inumber;
    inode_t inode;

    if(name==NULL || strcmp(name,"")==0 || strcmp(name,"/")==0) {

        inumber = ROOT_INODE_NUMBER;
    }
    else
        inumber = get_inode_from_path(name,&inode);
    if(inumber == 0)
	{
            fprintf(stderr,"Incorrect path\n");
            return -1;
	}
    /* Listing the Directory contents */
    INODE_READ(inumber, &inode);

    int file_count = inode.attr.size;
    int blk_count = CEIL(file_count,MAX_FILES_PER_BLOCK);
    unsigned int i,j;
    int count = 0;
    /*Print the filenames in the direct blocks */
    for( i=0; (i<BLKS_DIRECT && blk_count>0);i++,blk_count--) {
        get_filenames_in_block(inode.blocks_direct[i], &file_count, files, &count);
        if (count == inode.attr.size)
            return inode.attr.size;
    }

    /* Print the fielnames in the indirect blocks */
    for(i = 0;i<BLKS_INDIRECT;i++)
	{
            if(inode.blocks_indirect[i] == 0)
                return count;

            blk_addr_t blk_addresses[MAX_ADDR_PER_BLOCK];
            BLK_READ_INDIRECT(inode.blocks_indirect[i],blk_addresses);
            for(j = 0;(j<MAX_ADDR_PER_BLOCK && blk_count>0);j++,blk_count--)
		{
                    get_filenames_in_block(blk_addresses[j], &file_count, files, &count);
                    if (count == inode.attr.size)
                        return inode.attr.size;
                }
 	}

    return inode.attr.size;
}

void myclose(int fd)
{
    fs_check_mounted();
    file_table_delete(fd);
}

int myread(int fd, void * buf, size_t nbytes) {
    fs_check_mounted();

    size_t bytes_read;
    table_entry_t * table_entry = BB_DATA->openFileTable[fd];
    offset_t block_offset = table_entry->file_offset % BLK_SIZE; /* offset in current block */
    unsigned int block_leftover_bytes = (BLK_SIZE - (table_entry->file_offset % BLK_SIZE));
    file_size_t file_size = table_entry->inode.attr.size;

    /* error handling similar to read(2) */
    if (! table_entry) {
        return -EBADF;
    }

    /* check if current position is already EOF */
    if (table_entry->file_offset > (unsigned) table_entry->inode.attr.size - 1)
        return 0;

    /* adjust the number of bytes to read, if it will go past EOF */
    size_t bytes_till_eof = file_size - table_entry->file_offset;
    if (nbytes > bytes_till_eof) {
        nbytes = bytes_till_eof;
    }

    /* if the request for @nbytes bytes can be satisfied using the left-over
       bytes in the currently loaded block, read into @buf, update the
       file_offset and data fields, and return immediately */
    if (nbytes <= block_leftover_bytes) {
        bytes_read = nbytes;

        memcpy(buf, table_entry->data + block_offset, bytes_read);
        table_entry->file_offset += bytes_read;

        /* if the current data block has been read completely, load the next
           block */
        if (bytes_read == block_leftover_bytes)
            block_load_next(fd);

        return bytes_read;
    }

    else {
        bytes_read = block_leftover_bytes;

        memcpy(buf, table_entry->data + block_offset, bytes_read);
        table_entry->file_offset += bytes_read;

        /* load the next block, since the current one is definitely over */
        block_load_next(fd);

        /* recursively call myread() to fill up the buffer with data from the
           next block(s) */
        void * buf_new = buf + bytes_read;
        size_t nbytes_new = nbytes - bytes_read;
        return bytes_read + myread(fd,buf_new, nbytes_new);
    }
}

int mywrite(int fd, void * buf, size_t nbytes) {
    fs_check_mounted();

    table_entry_t * table_entry = BB_DATA->openFileTable[fd];
    inode_t * inode = &table_entry->inode;
    offset_t block_offset = table_entry->file_offset % BLK_SIZE; /* offset in current block */
    unsigned int block_leftover_bytes = (BLK_SIZE - (table_entry->file_offset % BLK_SIZE));

    /* error handling similar to write(2) */
    if (inode->attr.mode != RW || ! table_entry) {
        return -EBADF;
    }

    /* check if we are past the last block; if yes, allocate new block(s) */
    if (table_entry->addr == 0 && table_entry->data == NULL) {
        blk_addr_t addr;
        table_entry->data = malloc(BLK_SIZE);

        addr = block_allocate(fd);

        /* load the newly-allocated block into memory */
        table_entry->addr = addr;
        BLK_READ_DATA(table_entry->addr, table_entry->data);
    }

    /* if the request for @nbytes bytes can be satisfied using the left-over
       bytes in the currently loaded block, write into those from @buf, update
       the file_offset and data fields, and return immediately */
    if (nbytes <= block_leftover_bytes) {
        memcpy(table_entry->data + block_offset, buf, nbytes);
        table_entry->file_offset += nbytes;

        /* write the modified block to disk */
        BLK_WRITE_DATA(table_entry->addr, table_entry->data);

        /* if the current data block has been traversed completely, load the
           next block */
        if (nbytes == block_leftover_bytes)
            block_load_next(fd);

        /* update file size, if required */
        if (table_entry->file_offset >= inode->attr.size) {
            inode->attr.size = table_entry->file_offset;

            /* write the modified inode to disk */
            INODE_WRITE(inode->inumber, inode);
        }

        return nbytes;
    }

    else {
        memcpy(table_entry->data + block_offset, buf, block_leftover_bytes);
        table_entry->file_offset += block_leftover_bytes;

        /* write the modified block to disk */
        BLK_WRITE_DATA(table_entry->addr, table_entry->data);

        /* load the next block, since the current one is definitely over */
        block_load_next(fd);

        /* update file size, if required */
        if (table_entry->file_offset >= inode->attr.size) {
            inode->attr.size = table_entry->file_offset - 1;

            /* write the modified inode to disk */
            INODE_WRITE(inode->inumber, inode);
        }

        /* recursively call mywrite() to continue writing from @buf to the
           next block(s) */
        void * buf_new = buf + block_leftover_bytes;
        size_t nbytes_new = nbytes - block_leftover_bytes;
        return block_leftover_bytes + mywrite(fd, buf_new, nbytes_new);
    }
}

int myrmdir(const char * path) {
    inode_t * inode = (inode_t *) malloc(sizeof(inode_t));
    inumber_t inumber;

    inumber = get_inode_from_path(path, inode);

    /* check if directory exists */
    if (inumber == 0 || inumber >= INODE_COUNT) {
        /* fprintf(stderr, "myrmdir: %s: no such directory\n", path); */
        free(inode);
        return -ENOENT;
    }

    /* free @inode only if there are no files in the directory */
    if (inode->attr.size == 0) {
        inode_free(inode);

        /* update the parent */
        dir_entry_delete(path);

        free(inode);
        return 0;
    }
    else {
        /* fprintf(stderr, "myrmdir: %s: directory not empty\n", path); */
        free(inode);
        return -ENOTEMPTY;
    }
}

int myrm(const char * path) {
    inode_t * inode = (inode_t *) malloc(sizeof(inode_t));
    inumber_t inumber;

    inumber = get_inode_from_path(path, inode);
    log_msg("myrm inumber %d\n", inumber);

    /* check if file exists */
    if (inumber == INODE_COUNT + 1) {
        /* fprintf(stderr, "myrm: %s: no such file\n", path); */
        free(inode);
        return -ENOTDIR;
    }
    else if (inumber == 0) {
        free(inode);
        return -ENOENT;
    }

    int i;
    for (i = 0; i < MAX_OPEN_FILES; i++) {
        log_msg("myrm entry %d\n", i);
        if (BB_DATA->openFileTable[i] && BB_DATA->openFileTable[i]->inode.inumber == inumber)
            return -EBUSY;
    }

    /* free the file inode */
    log_msg("sadfsdf\n");
    inode_free(inode);
    log_msg("sadfsdfasdsad\n");

    /* update the parent */
    dir_entry_delete(path);
    log_msg("sadfsdffsdifjsdkjfsakjdfh\n");

    free(inode);

    return 0;
}

/* internal functions, not part of the public-facing API */


/* delete the directory entry of @path from its parent inode */

void dir_entry_delete(const char * path) {
    char * parent_path = dirname(strdup(path));
    char * name = basename(strdup(path));
    inode_t * parent_inode = (inode_t *) malloc(sizeof(inode_t));
    inumber_t inumber;
    blk_addr_t addr;
    unsigned int i, j, entry_count,blk_count;

    inumber = get_inode_from_path(parent_path, parent_inode);

    /* check if parent directory exists */
    if (inumber == 0 || inumber >= INODE_COUNT)
        return;

    entry_count = parent_inode->attr.size;
    blk_count = CEIL(entry_count,MAX_FILES_PER_BLOCK);

    /* Search in the direct blocks */
    for (i = 0; i < BLKS_DIRECT && blk_count>0; i++,blk_count--)
	{
            addr = parent_inode->blocks_direct[i];

            /* return if we're past the last valid block in the inode */
            if (addr < BLK_DATA_ADDR / BLK_SIZE || addr > BLK_COUNT - 1) {
                return;
            }

            if(delete_filename_in_block(addr,&entry_count,parent_inode,name))
                return;
        }

    /* Search in the indirect blocks */
    for(i = 0;i<BLKS_INDIRECT;i++)
	{
            if(parent_inode->blocks_indirect[i] == 0)
                return ;
            BLK_SEEK(parent_inode->blocks_indirect[i]);
            for(j = 0;(j<MAX_ADDR_PER_BLOCK && blk_count>0);j++,blk_count--)
		{
                    fread(&addr,sizeof(blk_addr_t),1,BB_DATA->fs);
                    if(delete_filename_in_block(addr,&entry_count,parent_inode,name))
                        return;
		}
 	}

    free(parent_inode);
}

/* Search for a filename in a block and if present return 1, else 0 */
int delete_filename_in_block(blk_addr_t addr,unsigned int * entry_count,inode_t * parent_inode,char * name)
{
    unsigned int j;
    blk_addr_t last_addr;
    file_entry_t file_entries[MAX_FILES_PER_BLOCK];
    DIR_BLOCK_READ(file_entries, addr);

    for (j = 0; j < MAX_FILES_PER_BLOCK && *entry_count>0; j++,(*entry_count)--)
	{
            if (strcmp(file_entries[j].name, name) == 0)
		{
                    /* Move the last entry to this position */
                    last_addr = dir_entry_move_last(parent_inode, &file_entries[j]);

                    /* Update the parent inode back to disk */
                    INODE_WRITE(parent_inode->inumber,parent_inode);

                    /* Update th entries in the block */
                    if (last_addr != addr)
                        DIR_BLOCK_WRITE(file_entries,addr);

                    return 1;
		}
	}
    return 0;
}

/* move the last directory entry in @inode to @entry (also updated the directory
 * entry count) */
blk_addr_t dir_entry_move_last(inode_t * inode, file_entry_t * entry)
{
    unsigned int entry_count_total = inode->attr.size;
    blk_addr_t addr;
    unsigned int file_entry_offset;
    file_entry_t file_entries[MAX_FILES_PER_BLOCK], * last;

    if (inode == NULL || entry == NULL)
        return 0;
    int blk_index = CEIL(entry_count_total,MAX_FILES_PER_BLOCK)-1;

    file_entry_offset = (entry_count_total-1) % MAX_FILES_PER_BLOCK;

    if(blk_index < BLKS_DIRECT)
	{
            /* Get the addr of the block from the direct blocks */
            addr = inode->blocks_direct[blk_index];
            if(file_entry_offset == 0)
                inode->blocks_direct[blk_index] = 0;
	}
    else
	{
            /* Get the addr of the block from the indirect blocks */

            blk_index-=BLKS_DIRECT;
            int blk_indirect_index = blk_index/MAX_ADDR_PER_BLOCK;
            blk_index-=blk_indirect_index*MAX_ADDR_PER_BLOCK;
            blk_addr_t  blk_addresses[MAX_ADDR_PER_BLOCK];
            DIR_BLOCK_READ(blk_addresses,inode->blocks_indirect[blk_indirect_index]);
            addr = blk_addresses[blk_index];

            /*free the block and also the indirect block if it is the last entry  */
            if(file_entry_offset == 0)
		{
                    blk_addresses[blk_index] = 0;
                    DIR_BLOCK_WRITE(blk_addresses,inode->blocks_indirect[blk_indirect_index]);
		}
            if(blk_index == 0 && file_entry_offset == 0) {
                block_free(inode->blocks_indirect[blk_index]);
                inode->blocks_indirect[blk_index] = 0;
            }
	}
    if (addr < BLK_DATA_ADDR / BLK_SIZE || addr > BLK_COUNT - 1)
        return 0;

    DIR_BLOCK_READ(file_entries, addr);
    last = &file_entries[file_entry_offset];

    /* copy @last to @entry */
    strcpy(entry->name, last->name);
    entry->inumber = last->inumber;

    /* zero @last */
    memset(last->name, 0, FILE_NAME_MAX + 1);
    last->inumber = 0;

    /* Free the block if the block has no more valid entries */
    if(file_entry_offset == 0)
        block_free(addr);

    /* write @last's block to disk */
    else
        DIR_BLOCK_WRITE(file_entries, addr);

    /* decrement the entry count of the directory */
    inode->attr.size--;

    return addr;
}

/* free the inode @inode, and all blocks associated with it */

void inode_free(inode_t * inode) {
    /* check if @inode points to a valid inode */
    if (inode == NULL)
        return;

    unsigned int i, j;
    bool done = false;
    blk_addr_t addr;

    for (i = 0; i < BLKS_DIRECT; i++) {
        addr = inode->blocks_direct[i];

        /* break if we're past the last valid block in the inode */
        if (addr < BLK_DATA_ADDR / BLK_SIZE || addr > BLK_COUNT - 1) {
            done = true;
            break;
        }

        block_free(addr);
    }

    if (! done) {
        for (i = 0; i < BLKS_INDIRECT; i++) {
            blk_addr_t indirect_block_addr = inode->blocks_indirect[i];
            blk_addr_t indirect_block[MAX_ADDR_PER_BLOCK];

            /* break if we're past the last valid block in the inode */
            if (indirect_block_addr < BLK_DATA_ADDR / BLK_SIZE || indirect_block_addr > BLK_COUNT - 1) {
                done = true;
                break;
            }

            /* read the indirect block into memory */
            BLK_READ_INDIRECT(indirect_block_addr, indirect_block);

            /* free all blocks pointed to by the indirect block */
            for (j = 0; j < MAX_ADDR_PER_BLOCK; j++) {
                addr = indirect_block[j];

                /* break if we're past the last valid block in the inode */
                if (addr < BLK_DATA_ADDR / BLK_SIZE || addr > BLK_COUNT - 1) {
                    done = true;
                    break;
                }

                block_free(addr);
            }

            /* break if we're past the last valid block in the inode */
            if (done)
                break;

            /* free the indirect block itself */
            block_free(indirect_block_addr);
        }
    }

    /* mark the inode as free */
    inode->used = false;

    /* write the modified inode to disk */
    INODE_WRITE(inode->inumber, inode);

    /* update super block stats */
    BB_DATA->super_blk->inode_free_count++;
    SUPER_BLOCK_WRITE(BB_DATA->super_blk);
}

/* free the block at @addr */

void block_free(blk_addr_t addr) {
    /* check if @addr is the address of a valid data block */
    if (addr < BLK_DATA_ADDR / BLK_SIZE || addr > BLK_COUNT - 1)
        return;

    /* zero the block to be freed */
    BLK_SEEK(addr);
    write_zeroes(BB_DATA->fs, BLK_SIZE);

    /* make the freed block point to the current head of the free block list */
    BLK_SEEK(addr);
    fwrite(&BB_DATA->super_blk->first_free_block, sizeof(blk_addr_t), 1, BB_DATA->fs);

    /* install the freed block as the new head of the free block list */
    BB_DATA->super_blk->first_free_block = addr;

    /* update block statistics in the super block */
    BB_DATA->super_blk->block_free_count++;
    BB_DATA->super_blk->block_used_count--;

    SUPER_BLOCK_WRITE(BB_DATA->super_blk);
}

/* load the next block in the open file @fd */

void block_load_next(int fd) {
    table_entry_t * table_entry = BB_DATA->openFileTable[fd];
    int block_next = (table_entry->file_offset - 1) / BLK_SIZE + 1;
    int block_last;

    /* last valid block no. (0-based) in the file */
    block_last = (table_entry->inode.attr.size - 1) / BLK_SIZE;

    /* check if @block_next lies after @block_last */
    if (block_next > block_last) {
        table_entry->addr = 0;
        table_entry->data = NULL;
        return;
    }

    /* if @block_next is a direct block, load it directly */
    if (block_next < BLKS_DIRECT) {
        table_entry->addr = table_entry->inode.blocks_direct[block_next];
        BLK_READ_DATA(table_entry->addr, table_entry->data);
    }
    /* else find the indirect block which contains @block_next, load that, and then load @block_next */
    else {
        int indirect_block_no = (block_next - BLKS_DIRECT) / MAX_ADDR_PER_BLOCK;
        int indirect_block_offset = (block_next - BLKS_DIRECT) % MAX_ADDR_PER_BLOCK;
        blk_addr_t indirect_block_addr = table_entry->inode.blocks_indirect[indirect_block_no];
        blk_addr_t indirect_block[MAX_ADDR_PER_BLOCK];

        BLK_READ_INDIRECT(indirect_block_addr, indirect_block);

        table_entry->addr = indirect_block[indirect_block_offset];
        BLK_READ_DATA(table_entry->addr, table_entry->data);
    }
}

/* allocate a new block to the open file @fd */

blk_addr_t block_allocate(int fd) {
    table_entry_t * table_entry = BB_DATA->openFileTable[fd];
    inode_t * inode = &table_entry->inode;
    int block_last = floor((table_entry->inode.attr.size - 1) * 1.0 / BLK_SIZE);
    int block_new = block_last + 1;

    /* if @block_new will be a direct block, allocate it directly */
    if (block_new < BLKS_DIRECT) {
        inode->blocks_direct[block_new] = get_free_block();

        /* write the modified inode to disk */
        INODE_WRITE(inode->inumber, inode);

        return inode->blocks_direct[block_new];
    }

    else {
        int indirect_block_no = (block_new - BLKS_DIRECT) / MAX_ADDR_PER_BLOCK;
        int indirect_block_offset = (block_new - BLKS_DIRECT) % MAX_ADDR_PER_BLOCK;
        blk_addr_t indirect_block_addr = table_entry->inode.blocks_indirect[indirect_block_no];
        blk_addr_t indirect_block[MAX_ADDR_PER_BLOCK];

        if (indirect_block_addr == 0) {
            indirect_block_addr = get_free_block();
            inode->blocks_indirect[indirect_block_no] = indirect_block_addr;

            /* write the modified inode to disk */
            INODE_WRITE(inode->inumber, inode);
        }

        BLK_READ_INDIRECT(indirect_block_addr, indirect_block);

        /* allocate a new block in the indirect block */
        indirect_block[indirect_block_offset] = get_free_block();

        /* write the modified indirect block to disk */
        BLK_WRITE_INDIRECT(indirect_block_addr, indirect_block);

        return indirect_block[indirect_block_offset];
    }
}

/* get the block address of the first block in the directory (represented by
 * @inode) which has free space for a new file entry. A new block may be
 * allocated to the directory in this process.
 *
 * @param inode     inode of the directory under consideration
 * @return          block address of the first block with free space for a new
 *                  entry in the directory */

blk_addr_t get_first_free_dir_block(inode_t * inode) {

    int i, block_offset;
    unsigned int first_free_dir_block_no = (inode->attr.size) / MAX_FILES_PER_BLOCK;

    if (first_free_dir_block_no < BLKS_DIRECT) {
        /* allocate a new block to the directory, if required */
        if (inode->attr.size % MAX_FILES_PER_BLOCK == 0) {
            inode->blocks_direct[first_free_dir_block_no] = get_free_block();

            /* write the modified inode back to disk */
            INODE_WRITE(inode->inumber, inode);
        }

        /* return the block address of the (possibly new) block */
        return inode->blocks_direct[first_free_dir_block_no];
    }

    else {

        for (i = 0; i < BLKS_INDIRECT; i++) {

            if (first_free_dir_block_no >= BLKS_DIRECT + i * MAX_ADDR_PER_BLOCK &&
                first_free_dir_block_no < BLKS_DIRECT + (i + 1) * MAX_ADDR_PER_BLOCK) {

                /* Allocate space for the indirect block, if required*/
                if(inode->blocks_indirect[i] == 0)
                    inode->blocks_indirect[i] = get_free_block();

                blk_addr_t indirect_block[MAX_ADDR_PER_BLOCK];
                blk_addr_t indirect_block_addr = inode->blocks_indirect[i];

                /* read the block addresses from the indirect block */
                BLK_READ_INDIRECT(indirect_block_addr, indirect_block);

                /* offset of the target block in the indirect block */
                block_offset = first_free_dir_block_no - (BLKS_DIRECT + i * MAX_ADDR_PER_BLOCK);

                /* allocate a new block to the directory, if required */
                if (inode->attr.size % MAX_FILES_PER_BLOCK == 0) {
                    indirect_block[block_offset] = get_free_block();

                    /* write the modified indirect block back to disk */
                    BLK_WRITE_INDIRECT(indirect_block_addr, indirect_block);
                }

                /* return the block address of the (possibly new) block */
                return indirect_block[block_offset];
            }

        }

    }

    return 0;
}

/* Create a file with given type
 * @param parent_inode : inode of parent_dir
 * @param file_type : file or directory
 * @param name : name of the file
 * @return : inode number of the file created, 0 on failure*/

inumber_t create_file(inode_t parent_inode,char * name,file_type_t file_type,file_mode_t mode)
{
    int i, entry_offset;
    inode_t inode;
    inumber_t inumber;
    blk_addr_t first_free_dir_block;
    file_entry_t parent_dir[MAX_FILES_PER_BLOCK];

    inumber = get_free_inode(&inode);

    /* create the new directory in a free inode */
    inode.attr.type = file_type;
    inode.attr.creation_time = time(NULL);
    inode.attr.mode = mode;
    inode.attr.size = 0;
    inode.used = true;
    for(i = 0; i < BLKS_DIRECT; i++)
        inode.blocks_direct[i] = 0;
    for(i = 0; i < BLKS_INDIRECT; i++)
        inode.blocks_indirect[i] = 0;

    INODE_WRITE(inumber, &inode);

    /* add an entry for the new directory in its parent directory inode */
    first_free_dir_block = get_first_free_dir_block(&parent_inode);
    DIR_BLOCK_READ(parent_dir, first_free_dir_block);
    entry_offset = parent_inode.attr.size % MAX_FILES_PER_BLOCK;

    parent_inode.attr.size++;
    INODE_WRITE(parent_inode.inumber, &parent_inode);

    strcpy(parent_dir[entry_offset].name, name);
    parent_dir[entry_offset].inumber = inumber;

    DIR_BLOCK_WRITE(parent_dir,first_free_dir_block);
    return inumber;
}

/* get a free data block
 *
 * @return          block address of the newly acquired block */

blk_addr_t get_free_block(void) {
    blk_addr_t ret = BB_DATA->super_blk->first_free_block;
    blk_addr_t next;

    /* if super_blk->first_free_block == BLK_COUNT, it means there are no more
       free blocks */
    if (ret == BLK_COUNT)
        return 0;

    /* update the first free block entry in the super block */
    BLK_SEEK(ret);
    fread(&next, sizeof(blk_addr_t), 1, BB_DATA->fs);
    BB_DATA->super_blk->first_free_block = next;

    /* zero the free block to be returned */
    BLK_SEEK(ret);
    write_zeroes(BB_DATA->fs, BLK_SIZE);

    /* update super block statistics */
    BB_DATA->super_blk->block_used_count++;
    BB_DATA->super_blk->block_free_count--;

    /* write the super block back to disk */
    SUPER_BLOCK_WRITE(BB_DATA->super_blk);

    return ret;
}

/* get a free inode, or NULL
 *
 * @param free_inode  the free inode found, if one exists, else NULL
 * @return            the inumber of the free inode, if found, else 0 */

inumber_t get_free_inode(inode_t * free_inode) {
    int i;

    for (i = 1; i <= INODE_COUNT; i++) {
        INODE_READ(i, free_inode);
        if(! free_inode->used) {
            /* update super block stats */
            BB_DATA->super_blk->inode_free_count--;
            SUPER_BLOCK_WRITE(BB_DATA->super_blk);

            return i;
        }
    }

    return 0;
}

/*This function returns the inode of the file from a path.
 *This function is called by almost every funtion in our filesystem
 * @param filepath : path to a file (accepts both formats with starting "/" or without "/" )
 * @param inode : inode of the file if it exists , else inode of the parent
 * @return : inumber of the file if it exists, else 0 or (INODE_COUNT + 1)
 */

inumber_t get_inode_from_path(const char * filepath, inode_t * inode)
{
    if (strcmp(filepath, "/") == 0 || strcmp(filepath,".")==0) {
        INODE_READ(ROOT_INODE_NUMBER, inode);
        return ROOT_INODE_NUMBER;
    }

    /* Format the input filename string to exclude '/' */
    char * str = strdup(filepath);
    if(filepath[0] == '/')
        str++;

    /* Split the filename with '/' to get the inode of the final file*/
    int i;
    inumber_t inode_no = ROOT_INODE_NUMBER;     // Start with root dir
    char ** splitarr = (char **)malloc(sizeof(char *)*PATH_DEPTH_MAX);

    /*Check if the path is valid  */
    int pathdepth = strsplit(str,"/",splitarr);
    for(i = 0;i < pathdepth-1 ;i++)
	{
            inode_no = get_inode_from_name(inode_no,splitarr[i],DIR_T);
            if(inode_no == 0)
                return (INODE_COUNT+1);
	}

    /* Check if the file exists */
    inumber_t parent_inumber = inode_no;
    inode_no = get_inode_from_name(inode_no,splitarr[i],FILE_T);

    /* Put the appropriate structure in the input param */
    if(inode_no == 0)
        INODE_READ(parent_inumber,inode);
    else
        INODE_READ(inode_no,inode);
    return inode_no;
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

/* Exit in case of error after printing the error Message */
void error_exit(char * errorStr){
    fprintf(stderr,"%s\n",errorStr);
    exit(EXIT_FAILURE);
}

/* Get the inode number of a given filename in a directory
 * @param inode_no : inode_number of parent directory
 * @param name : Name of the file
 * @return inode_no of file or 0,if not exists.
 */

inumber_t get_inode_from_name(inumber_t parent_inode_no,char * name,file_type_t file_type)
{
    /* Get the inode structure of the parent */
    inode_t parent_inode;
    if(parent_inode_no == 1)
        INODE_READ(ROOT_INODE_NUMBER, &parent_inode);
    else
	{
            INODE_READ(parent_inode_no,&parent_inode);
	}

    /* Traverse through the blocks of directory for the filename */
    unsigned int i,j;
    inumber_t inode_no;
    int file_count = parent_inode.attr.size;
    int blk_count = CEIL(file_count,MAX_FILES_PER_BLOCK);

    for( i=0; (i<BLKS_DIRECT && blk_count>0);i++,blk_count--)
	{
            inode_no = get_inode_in_block(parent_inode.blocks_direct[i],name,&file_count,file_type);
            if(inode_no!=0)
                return inode_no;
	}
    for(i = 0;i<BLKS_INDIRECT;i++)
	{
            if(parent_inode.blocks_indirect[i] == 0)
                return 0;
            blk_addr_t blk_addresses[MAX_ADDR_PER_BLOCK];
            BLK_READ_INDIRECT(parent_inode.blocks_indirect[i],blk_addresses);

            for(j = 0;(j<MAX_ADDR_PER_BLOCK && blk_count>0);j++,blk_count--)
		{
                    inode_no = get_inode_in_block(blk_addresses[j],name,&file_count,file_type);
                    if(inode_no!=0)
                        return inode_no;
		}
 	}
    return 0;
}

/* Checks a given directory block for a file name
 * @param block_offset : offset of the block in the file system
 * @param name : name of the file
 * @return : inode number of the block if found, else 0
 */
inumber_t get_inode_in_block(blk_addr_t addr,char * name,int * count,file_type_t file_type)
{
    unsigned int j;
    if (addr < BLK_DATA_ADDR / BLK_SIZE || addr > BLK_COUNT - 1)
        return 0;

    file_entry_t file_entries[MAX_FILES_PER_BLOCK];
    DIR_BLOCK_READ(file_entries,addr);

    /* Compare the filename with all the names in this block */
    for(j = 0;j<MAX_FILES_PER_BLOCK && (*count)>0;j++)
	{
            *count = *count -1;
            if(strcmp(file_entries[j].name,name)==0)
		{
                    if(file_type == DIR_T && is_filetype_same(file_entries[j].inumber,file_type))
                        return file_entries[j].inumber;
                    else if(file_type == FILE_T)
                        return file_entries[j].inumber;
		}
        }
    return 0;
}

void print_filenames_in_block(blk_addr_t addr,int * count)
{
    unsigned int j;
    if (addr < BLK_DATA_ADDR / BLK_SIZE || addr > BLK_COUNT - 1)
        return;

    file_entry_t file_entries[MAX_FILES_PER_BLOCK];

    DIR_BLOCK_READ(file_entries,addr);

    /* Print the filenames in this block */
    for(j = 0;j<MAX_FILES_PER_BLOCK && (*count)>0;j++)
	{
            *count = *count -1;
            printf("%s\n",file_entries[j].name);
        }
}

void get_filenames_in_block(blk_addr_t addr, int * file_count, char *files[], int * count)
{
    unsigned int j;
    if (addr < BLK_DATA_ADDR / BLK_SIZE || addr > BLK_COUNT - 1)
        return;

    file_entry_t file_entries[MAX_FILES_PER_BLOCK];

    DIR_BLOCK_READ(file_entries,addr);

    /* Print the filenames in this block */
    for(j = 0;j<MAX_FILES_PER_BLOCK && (*file_count)>0;j++)
	{
            *file_count = *file_count -1;
            files[*count] = malloc((strlen(file_entries[j].name) + 1) * sizeof(char));
            strcpy(files[*count], file_entries[j].name);
            log_msg("get_files name = %s\n", file_entries[j].name);
            (*count)++;
        }
}

/* Check if a inode has the same required file type */
int is_filetype_same(inumber_t inode_no,file_type_t file_type)
{
    inode_t inode;
    INODE_READ(inode_no,&inode);
    if(inode.attr.type == file_type)
        return 1;
    else
        return 0;
}

/* Split a string into an array of strings with a delimiter */
int strsplit(char * str, char * delim, char ** arr)
{
    char * origstr = (char * )malloc(sizeof(str));
    strcpy(origstr,str);
    int i = 0;
    arr[i++]= strtok(origstr,delim);
    while(arr[i-1]!=NULL)
        arr[i++] = strtok(NULL,delim);
    return i-1;
}

int file_table_insert(inumber_t inode_no)
{
    int i;
    for(i = 0;i<MAX_OPEN_FILES;i++)
	{
            if(BB_DATA->openFileTable[i]==NULL)
		{
                    BB_DATA->openFileTable[i] = (table_entry_t *)malloc(sizeof(table_entry_t));
                    INODE_READ(inode_no,&(BB_DATA->openFileTable[i]->inode));
                    BB_DATA->openFileTable[i]->refCount = 1;
                    BB_DATA->openFileTable[i]->file_offset = 0;
                    BB_DATA->openFileTable[i]->addr = BB_DATA->openFileTable[i]->inode.blocks_direct[0];
                    if (BB_DATA->openFileTable[i]->addr != 0) {
                        BB_DATA->openFileTable[i]->data = malloc(BLK_SIZE);
                        BLK_READ_DATA(BB_DATA->openFileTable[i]->addr, BB_DATA->openFileTable[i]->data);
                    }
                    else {
                        BB_DATA->openFileTable[i]->data = NULL;
                    }

                    return i;
		}
	}
    return -ENFILE;
}

int file_table_delete(int fd)
{
    if(BB_DATA->openFileTable[fd] == NULL)
	{
            fprintf(stderr,"Incorrect File Descriptor\n");
            return 0;
	}

    /* free(BB_DATA->openFileTable[fd]); */
    BB_DATA->openFileTable[fd] = NULL;

    return 1;
}

/* check whether the fs is mounted; if not, print error and exit */
void fs_check_mounted(void) {
    if(BB_DATA->super_blk == NULL) {
        fprintf(stderr,"error: filesystem not mounted\n");
        exit(0);
    }
}

void dump_used_inodes(void)
{
    printf("\nDumping used inode numbers\n");
    int i = 0;
    inode_t inode;
    for(i = 1;i<=INODE_COUNT;i++)
 	{
            INODE_READ(i,&inode);
            if(inode.used)
                printf("%d\n",inode.inumber);
 	}
}
