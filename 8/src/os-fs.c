#include "params.h"

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <fuse.h>
#include <libgen.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/xattr.h>

#include "fs_functions.h"
#include "logger.h"

// Report errors to logfile and give -errno to caller
static int bb_error(char *str)
{
    int ret = -errno;

    log_msg("    ERROR %s: %s\n", str, strerror(errno));

    return ret;
}

int bb_getattr(const char *path, struct stat *statbuf)
{
    int retstat = 0;

    log_msg("\nbb_getattr(path=\"%s\", statbuf=0x%08x)\n",
	  path, statbuf);

    inode_t inode;
    inumber_t inumber;
    inumber = get_inode_from_path(path, &inode);

    log_msg("inumber = %d\n", inumber);

    if (inumber == 0 || inumber == INODE_COUNT + 1) {
        return -ENOENT;
    }

    if (inode.attr.type == DIR_T) {
        statbuf->st_mode = S_IFDIR | 0755;
    }
    else if (inode.attr.type == FILE_T) {
        statbuf->st_mode = S_IFREG | 0644;
        statbuf->st_size = inode.attr.size;
    }

    statbuf->st_ino = inode.inumber;

    statbuf->st_uid = getuid();

    statbuf->st_gid = getgid();

    statbuf->st_atime = inode.attr.creation_time;

    statbuf->st_ctime = inode.attr.creation_time;

    statbuf->st_mtime = inode.attr.creation_time;

    log_stat(statbuf);

    return retstat;
}

int bb_readlink(const char *path, char *link, size_t size)
{
    int retstat = 0;

    log_msg("bb_readlink(path=\"%s\", link=\"%s\", size=%d)\n",
	  path, link, size);

    if (retstat < 0)
	retstat = bb_error("bb_readlink readlink");
    else  {
	link[retstat] = '\0';
	retstat = 0;
    }

    return retstat;
}

int bb_mknod(const char *path, mode_t mode, dev_t dev)
{
    int retstat = 0;
    char * mode_str;

    log_msg("\nbb_mknod(path=\"%s\", mode=0%3o, dev=%lld)\n",
            path, mode, dev);

    if (mode & O_RDONLY) {
        mode_str = "r";
    }
    else {
        mode_str = "w";
    }

    int fd = myopen(path, mode_str);
    myclose(fd);

    return retstat;
}

int bb_mkdir(const char *path, mode_t mode)
{
    int retstat = 0;

    log_msg("\nbb_mkdir(path=\"%s\", mode=0%3o)\n",
	    path, mode);

    retstat = mymkdir(path);
    log_msg("mkdir returned: %d\n", retstat);

    return retstat;
}

int bb_unlink(const char *path)
{
    int retstat = 0;

    log_msg("bb_unlink(path=\"%s\")\n",
	    path);

    retstat = myrm(path);

    errno = -retstat;
    if (retstat < 0)
	retstat = bb_error("bb_unlink unlink");

    return retstat;
}

int bb_rmdir(const char *path)
{
    int retstat = 0;

    log_msg("bb_rmdir(path=\"%s\")\n",
	    path);

    retstat = myrmdir(path);

    if (retstat < 0)
	retstat = bb_error("bb_rmdir rmdir");

    return retstat;
}

int bb_symlink(const char *path, const char *link)
{
    int retstat = 0;

    log_msg("\nbb_symlink(path=\"%s\", link=\"%s\")\n",
	    path, link);

    /* retstat = symlink(path, flink); */
    if (retstat < 0)
	retstat = bb_error("bb_symlink symlink");

    return retstat;
}

int bb_rename(const char *path, const char *newpath)
{
    int retstat = 0;

    log_msg("\nbb_rename(fpath=\"%s\", newpath=\"%s\")\n",
	    path, newpath);

    if (retstat < 0)
	retstat = bb_error("bb_rename rename");

    return retstat;
}

int bb_link(const char *path, const char *newpath)
{
    int retstat = 0;

    log_msg("\nbb_link(path=\"%s\", newpath=\"%s\")\n",
	    path, newpath);

    if (retstat < 0)
	retstat = bb_error("bb_link link");

    return retstat;
}

int bb_chmod(const char *path, mode_t mode)
{
    int retstat = 0;

    log_msg("\nbb_chmod(fpath=\"%s\", mode=0%03o)\n",
	    path, mode);

    if (retstat < 0)
	retstat = bb_error("bb_chmod chmod");

    return retstat;
}

int bb_chown(const char *path, uid_t uid, gid_t gid)

{
    int retstat = 0;

    log_msg("\nbb_chown(path=\"%s\", uid=%d, gid=%d)\n",
	    path, uid, gid);

    if (retstat < 0)
	retstat = bb_error("bb_chown chown");

    return retstat;
}

int bb_truncate(const char *path, off_t newsize)
{
    int retstat = 0;

    return 0;

    log_msg("\nbb_truncate(path=\"%s\", newsize=%lld)\n",
	    path, newsize);

    if (retstat < 0)
	bb_error("bb_truncate truncate");

    return retstat;
}

int bb_utime(const char *path, struct utimbuf *ubuf)
{
    int retstat = 0;

    return 0;

    log_msg("\nbb_utime(path=\"%s\", ubuf=0x%08x)\n",
	    path, ubuf);

    if (retstat < 0)
	retstat = bb_error("bb_utime utime");

    return retstat;
}

int bb_open(const char *path, struct fuse_file_info *fi)
{
    int retstat = 0;
    int fd;

    log_msg("\nbb_open(path\"%s\", fi=0x%08x)\n",
	    path, fi);

    char * mode_str;
    mode_t mode = fi->flags;
    if (mode & O_RDONLY) {
        mode_str = "r";
    }
    else {
        mode_str = "w";
    }

    fd = myopen(path, mode_str);

    fi->fh = fd;
    log_fi(fi);

    return retstat;
}

int bb_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
    int retstat = 0;

    log_msg("\nbb_read(path=\"%s\", buf=0x%08x, size=%d, offset=%lld, fi=0x%08x)\n",
	    path, buf, size, offset, fi);
    // no need to get fpath on this one, since I work from fi->fh not the path
    log_fi(fi);

    retstat = myread(fi->fh, buf, size);
    log_msg("myread: retstat = %d, size = %d, buf = %s\n", retstat, size, buf);
    if (retstat < 0)
	retstat = bb_error("bb_read read");

    return retstat;
}

int bb_write(const char *path, const char *buf, size_t size, off_t offset,
	     struct fuse_file_info *fi)
{
    int retstat = 0;

    log_msg("\nbb_write(path=\"%s\", buf=0x%08x, size=%d, offset=%lld, fi=0x%08x)\n",
	    path, buf, size, offset, fi);
    // no need to get fpath on this one, since I work from fi->fh not the path
    log_fi(fi);

    log_msg("mywrite: retstat = %d, size = %d, buf = %s\n", retstat, size, buf);
    retstat = mywrite(fi->fh, strdup(buf), size);
    log_msg("mywrite: retstat = %d, size = %d, buf = %s\n", retstat, size, buf);

    if (retstat < 0)
	retstat = bb_error("bb_write pwrite");

    return retstat;
}

int bb_statfs(const char *path, struct statvfs *statv)
{
    int retstat = 0;

    log_msg("\nbb_statfs(path=\"%s\", statv=0x%08x)\n",
	    path, statv);

    if (retstat < 0)
	retstat = bb_error("bb_statfs statvfs");

    log_statvfs(statv);

    return retstat;
}

int bb_flush(const char *path, struct fuse_file_info *fi)
{
    int retstat = 0;

    log_msg("\nbb_flush(path=\"%s\", fi=0x%08x)\n", path, fi);
    log_fi(fi);

    return retstat;
}

int bb_release(const char *path, struct fuse_file_info *fi)
{
    int retstat = 0;

    log_msg("\nbb_release(path=\"%s\", fi=0x%08x)\n", path, fi);
    log_fi(fi);

    // We need to close the file.  Had we allocated any resources
    // (buffers etc) we'd need to free them here as well.
    myclose(fi->fh);

    return retstat;
}

int bb_fsync(const char *path, int datasync, struct fuse_file_info *fi)
{
    int retstat = 0;

    log_msg("\nbb_fsync(path=\"%s\", datasync=%d, fi=0x%08x)\n",
	    path, datasync, fi);
    log_fi(fi);

    if (datasync)
	retstat = fdatasync(fi->fh);
    else
	retstat = fsync(fi->fh);

    if (retstat < 0)
	bb_error("bb_fsync fsync");

    return retstat;
}

int bb_setxattr(const char *path, const char *name, const char *value, size_t size, int flags)
{
    int retstat = 0;

    log_msg("\nbb_setxattr(path=\"%s\", name=\"%s\", value=\"%s\", size=%d, flags=0x%08x)\n",
	    path, name, value, size, flags);

    if (retstat < 0)
	retstat = bb_error("bb_setxattr lsetxattr");

    return retstat;
}

int bb_getxattr(const char *path, const char *name, char *value, size_t size)
{
    int retstat = 0;

    return 0;

    log_msg("\nbb_getxattr(path = \"%s\", name = \"%s\", value = 0x%08x, size = %d)\n",
	    path, name, value, size);

    if (retstat < 0)
	retstat = bb_error("bb_getxattr lgetxattr");
    else
	log_msg("    value = \"%s\"\n", value);

    return retstat;
}

int bb_listxattr(const char *path, char *list, size_t size)
{
    int retstat = 0;
    char *ptr;

    log_msg("bb_listxattr(path=\"%s\", list=0x%08x, size=%d)\n",
	    path, list, size);

    if (retstat < 0)
	retstat = bb_error("bb_listxattr llistxattr");

    log_msg("    returned attributes (length %d):\n", retstat);
    for (ptr = list; ptr < list + retstat; ptr += strlen(ptr)+1)
	log_msg("    \"%s\"\n", ptr);

    return retstat;
}

int bb_removexattr(const char *path, const char *name)
{
    int retstat = 0;

    log_msg("\nbb_removexattr(path=\"%s\", name=\"%s\")\n", path, name);

    if (retstat < 0)
	retstat = bb_error("bb_removexattr lrmovexattr");

    return retstat;
}

int bb_opendir(const char *path, struct fuse_file_info *fi)
{
    int retstat = 0;

    log_msg("\nbb_opendir(path=\"%s\", fi=0x%08x)\n", path, fi);

    inode_t inode;
    get_inode_from_path(path, &inode);
    fi->fh = 0;

    if (inode.inumber > 0 && inode.inumber < INODE_COUNT + 1) {
        return 0;
    }
    else {
        return -ENOENT;
    }

    log_fi(fi);

    return retstat;
}

int bb_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset,
	       struct fuse_file_info *fi)
{
    int retstat = 0;
    char *files[100];

    files[0] = ".";
    files[1] = "..";

    log_msg("\nbb_readdir(path=\"%s\", buf=0x%08x, filler=0x%08x, offset=%lld, fi=0x%08x)\n",
	    path, buf, filler, offset, fi);

    int i, count = myreaddir(path, &files[2]) + 2;

    log_msg("count: %d, files[2]: %s\n", count, files[1]);

    for (i = 0; i < count; i++) {
        log_msg("calling filler with name %s\n", files[i]);
        if (filler(buf, files[i], NULL, 0) != 0) {
            log_msg("    ERROR bb_readdir filler:  buffer full");
            return -ENOMEM;
        }
    }

    log_fi(fi);

    return retstat;
}

int bb_releasedir(const char *path, struct fuse_file_info *fi)
{
    int retstat = 0;

    log_msg("\nbb_releasedir(path=\"%s\", fi=0x%08x)\n", path, fi);
    log_fi(fi);

    return retstat;
}

int bb_fsyncdir(const char *path, int datasync, struct fuse_file_info *fi)
{
    int retstat = 0;

    log_msg("\nbb_fsyncdir(path=\"%s\", datasync=%d, fi=0x%08x)\n", path, datasync, fi);
    log_fi(fi);

    return retstat;
}

void *bb_init(struct fuse_conn_info *conn)
{
    (void) conn;

    log_msg("\nbb_init()\n");

    mymount(BB_DATA->rootdir);

    return BB_DATA;
}

void bb_destroy(void *userdata)
{
    log_msg("\nbb_destroy(userdata=0x%08x)\n", userdata);

    myunmount(BB_DATA->rootdir);
}

int bb_access(const char *path, int mask)
{
    int retstat = 0;

    log_msg("\nbb_access(path=\"%s\", mask=0%o)\n",
	    path, mask);

    /* retstat = access(fpath, mask); */

    if (retstat < 0)
	retstat = bb_error("bb_access access");

    return retstat;
}

int bb_fgetattr(const char *path, struct stat *statbuf, struct fuse_file_info *fi)
{
    int retstat = 0;

    log_msg("\nbb_fgetattr(path=\"%s\", statbuf=0x%08x, fi=0x%08x)\n",
	    path, statbuf, fi);
    log_fi(fi);

    return bb_getattr(path, statbuf);

    log_stat(statbuf);

    return retstat;
}

struct fuse_operations bb_oper = {
  .getattr = bb_getattr,
  .readlink = bb_readlink,
  .getdir = NULL,
  .mknod = bb_mknod,
  .mkdir = bb_mkdir,
  .unlink = bb_unlink,
  .rmdir = bb_rmdir,
  .symlink = bb_symlink,
  .rename = bb_rename,
  .link = bb_link,
  .chmod = bb_chmod,
  .chown = bb_chown,
  .truncate = bb_truncate,
  .utime = bb_utime,
  .open = bb_open,
  .read = bb_read,
  .write = bb_write,
  .statfs = bb_statfs,
  .flush = bb_flush,
  .release = bb_release,
  .fsync = bb_fsync,
  .setxattr = bb_setxattr,
  .getxattr = bb_getxattr,
  .listxattr = bb_listxattr,
  .removexattr = bb_removexattr,
  .opendir = bb_opendir,
  .readdir = bb_readdir,
  .releasedir = bb_releasedir,
  .fsyncdir = bb_fsyncdir,
  .init = bb_init,
  .destroy = bb_destroy,
  .access = bb_access,
  /* .create = bb_create, */
  /* .ftruncate = bb_ftruncate, */
  .fgetattr = bb_fgetattr
};

void bb_usage()
{
    fprintf(stderr, "usage: ./os-fs <fs> <mount_point>\n");
    exit(-1);
}

int main(int argc, char *argv[])
{
    int i;
    int fuse_stat;
    struct bb_state *bb_data;

    if ((getuid() == 0) || (geteuid() == 0)) {
	fprintf(stderr, "Running BBFS as root opens unnacceptable security holes\n");
	return 1;
    }

    bb_data = calloc(sizeof(struct bb_state), 1);
    if (bb_data == NULL) {
	perror("main calloc");
	abort();
    }

    bb_data->logfile = log_open();

    for (i = 1; (i < argc) && (argv[i][0] == '-'); i++)
	if (argv[i][1] == 'o') i++;

    if ((argc - i) != 2) bb_usage();

    bb_data->rootdir = realpath(argv[i], NULL);

    argv[i] = argv[i+1];
    argc--;

    fprintf(stderr, "initializing filesystem...\n");
    fuse_stat = fuse_main(argc, argv, &bb_oper, bb_data);
    fprintf(stderr, "return value of fuse_main = %d\n", fuse_stat);

    return fuse_stat;
}
