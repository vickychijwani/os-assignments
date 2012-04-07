/* this header file exposes and documents the public API of our filesystem */
/* nothing else should go in here */

#ifndef _FS_FUNCTIONS_H_
#define _FS_FUNCTIONS_H_

inumber_t get_inode_from_path(const char * filepath, inode_t * inode);

int myreaddir(const char * path, char *files[]);

/* create and format the filesystem, in the current directory itself, or
 * re-format it if it already exists */

void myformat(const char * fs_name);

/* Mount the filesystem in memory with name @name
 * Return 0 on success
 */

int mymount(const char * fs_name);

int myunmount(const char * fs_name);

/* Open a file with the given name if exists, else create the file with the given mode */

int myopen(const char * filename,char * mode);

void myclose(int fd);

/* create a new directory with name @name */

int mymkdir(const char * name);

/* read @nbytes bytes into @buf from the file @fd
 *
 * @return          integer (>= 0) indicating the number of bytes actually read,
 *                  else -1 */

int myread(int fd, void * buf, size_t nbytes);

/* write @nbytes bytes from @buf into the file @fd
 *
 * @return          integer (>= 0) indicating the number of bytes actually
 *                  written, else -1 */

int mywrite(int fd, void * buf, size_t nbytes);

/* remove the directory at @path, if it is empty */

int myrmdir(const char * path);

/* remove the file at @path */

int myrm(const char * path);


#endif /* _FS_FUNCTIONS_H_ */
