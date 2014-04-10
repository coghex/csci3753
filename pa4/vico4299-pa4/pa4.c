#define FUSE_USE_VERSION 28
#define HAVE_SETXATTR

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef linux
/* For pread()/pwrite() */
#define _XOPEN_SOURCE 500
#endif

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include "aes-crypt.h"
#include <sys/time.h>
#ifdef HAVE_SETXATTR
#include <sys/xattr.h>
#endif

#define DATA ((struct data *) fuse_get_context()->private_data)

struct data {
  char* rootdir;
  char* key;
};

void pathcat(char fpath[PATH_MAX], const char *path) {
  strcpy(fpath, DATA->rootdir);
  strncat(fpath, path, PATH_MAX);
}

static int xmp_getattr(const char *path, struct stat *stbuf)
{
  int res;
  char newpath[PATH_MAX];
  pathcat(newpath, path);

  res = lstat(newpath, stbuf);
  if (res == -1)
    return -errno;

  return 0;
}

static int xmp_access(const char *path, int mask)
{
  int res;
  char newpath[PATH_MAX];
  pathcat(newpath, path);


  res = access(newpath, mask);
  if (res == -1)
    return -errno;

  return 0;
}

static int xmp_readlink(const char *path, char *buf, size_t size)
{
  int res;
  char newpath[PATH_MAX];
  pathcat(newpath, path);

  res = readlink(newpath, buf, size - 1);
  if (res == -1)
    return -errno;

  buf[res] = '\0';
  return 0;
}


static int xmp_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
		       off_t offset, struct fuse_file_info *fi)
{
  DIR *dp;
  struct dirent *de;
  char newpath[PATH_MAX];
  pathcat(newpath, path);

  (void) offset;
  (void) fi;

  dp = opendir(newpath);
  if (dp == NULL)
    return -errno;

  while ((de = readdir(dp)) != NULL) {
    struct stat st;
    memset(&st, 0, sizeof(st));
    st.st_ino = de->d_ino;
    st.st_mode = de->d_type << 12;
    if (filler(buf, de->d_name, &st, 0))
      break;
  }

  closedir(dp);
  return 0;
}

static int xmp_mknod(const char *path, mode_t mode, dev_t rdev)
{
  int res;
  char newpath[PATH_MAX];
  pathcat(newpath, path);

  /* On Linux this could just be 'mknod(path, mode, rdev)' but this
   is more portable */
  if (S_ISREG(mode)) {
    res = open(newpath, O_CREAT | O_EXCL | O_WRONLY, mode);
    if (res >= 0)
      res = close(res);
    } else if (S_ISFIFO(mode))
      res = mkfifo(newpath, mode);
    else
      res = mknod(newpath, mode, rdev);
  if (res == -1)
    return -errno;

  return 0;
}

static int xmp_mkdir(const char *path, mode_t mode)
{
  int res;
  char newpath[PATH_MAX];
  pathcat(newpath, path);

  res = mkdir(newpath, mode);
  if (res == -1)
    return -errno;

  return 0;
}

static int xmp_unlink(const char *path)
{
  int res;
  char newpath[PATH_MAX];
  pathcat(newpath, path);

  res = unlink(newpath);
  if (res == -1)
    return -errno;

  return 0;
}

static int xmp_rmdir(const char *path)
{
  int res;
  char newpath[PATH_MAX];
  pathcat(newpath, path);

  res = rmdir(newpath);
  if (res == -1)
    return -errno;

  return 0;
}

static int xmp_symlink(const char *from, const char *to)
{
  int res;

  res = symlink(from, to);
  if (res == -1)
    return -errno;

  return 0;
}

static int xmp_rename(const char *from, const char *to)
{
  int res;

  res = rename(from, to);
  if (res == -1)
    return -errno;

  return 0;
}

static int xmp_link(const char *from, const char *to)
{
  int res;

  res = link(from, to);
  if (res == -1)
    return -errno;

  return 0;
}

static int xmp_chmod(const char *path, mode_t mode)
{
  int res;
  char newpath[PATH_MAX];
  pathcat(newpath, path);

  res = chmod(newpath, mode);
  if (res == -1)
    return -errno;

  return 0;
}

static int xmp_chown(const char *path, uid_t uid, gid_t gid)
{
  int res;
  char newpath[PATH_MAX];
  pathcat(newpath, path);

  res = lchown(newpath, uid, gid);
  if (res == -1)
    return -errno;

  return 0;
}

static int xmp_truncate(const char *path, off_t size)
{
  int res;
  char newpath[PATH_MAX];
  pathcat(newpath, path);

  res = truncate(newpath, size);
  if (res == -1)
    return -errno;

  return 0;
}

static int xmp_utimens(const char *path, const struct timespec ts[2])
{
  int res;
  struct timeval tv[2];
  char newpath[PATH_MAX];
  pathcat(newpath, path);

  tv[0].tv_sec = ts[0].tv_sec;
  tv[0].tv_usec = ts[0].tv_nsec / 1000;
  tv[1].tv_sec = ts[1].tv_sec;
  tv[1].tv_usec = ts[1].tv_nsec / 1000;

  res = utimes(newpath, tv);
  if (res == -1)
    return -errno;

  return 0;
}

static int xmp_open(const char *path, struct fuse_file_info *fi)
{
  int res;
  char newpath[PATH_MAX];
  pathcat(newpath, path);

  res = open(newpath, fi->flags);
  if (res == -1)
    return -errno;

  close(res);
  return 0;
}

static int xmp_read(const char *path, char *buf, size_t size, off_t offset,
		    struct fuse_file_info *fi)
{
  int fd;
  int res;
  char newpath[PATH_MAX];
  pathcat(newpath, path);

  (void) fi;
  fd = open(newpath, O_RDONLY);
  if (fd == -1)
    return -errno;

  res = pread(fd, buf, size, offset);
  if (res == -1)
    res = -errno;

  close(fd);
  return res;
}

static int xmp_write(const char *path, const char *buf, size_t size,
		     off_t offset, struct fuse_file_info *fi)
{
  int fd;
  int res;
  char newpath[PATH_MAX];
  pathcat(newpath, path);

  (void) fi;
  fd = open(newpath, O_WRONLY);
  if (fd == -1)
    return -errno;

  res = pwrite(fd, buf, size, offset);
  if (res == -1)
    res = -errno;

  close(fd);
  return res;
}

static int xmp_statfs(const char *path, struct statvfs *stbuf)
{
  int res;
  char newpath[PATH_MAX];
  pathcat(newpath, path);

  res = statvfs(newpath, stbuf);
  if (res == -1)
    return -errno;

  return 0;
}

static int xmp_create(const char* path, mode_t mode, struct fuse_file_info* fi)
{
  (void) fi;
  char newpath[PATH_MAX];
  pathcat(newpath, path);

  int res;
  res = creat(newpath, mode);
  if(res == -1)
    return -errno;

  close(res);

  return 0;
}


static int xmp_release(const char *path, struct fuse_file_info *fi)
{
  (void) path;
  (void) fi;
  return 0;
}

static int xmp_fsync(const char *path, int isdatasync,
		     struct fuse_file_info *fi)
{
  (void) path;
  (void) isdatasync;
  (void) fi;
  return 0;
}

static int xmp_setxattr(const char *path, const char *name, const char *value,
			size_t size, int flags)
{
  char newpath[PATH_MAX];
  pathcat(newpath, path);
  int res = lsetxattr(newpath, name, value, size, flags);
  if (res == -1)
    return -errno;
  return 0;
}

static int xmp_getxattr(const char *path, const char *name, char *value,
			size_t size)
{
  char newpath[PATH_MAX];
  pathcat(newpath, path);
  int res = lgetxattr(newpath, name, value, size);
  if (res == -1)
    return -errno;
  return res;
}

static int xmp_listxattr(const char *path, char *list, size_t size)
{
  char newpath[PATH_MAX];
  pathcat(newpath, path);
  int res = llistxattr(newpath, list, size);
  if (res == -1)
    return -errno;
  return res;
}

static int xmp_removexattr(const char *path, const char *name)
{
  char newpath[PATH_MAX];
  pathcat(newpath, path);
  int res = lremovexattr(newpath, name);
  if (res == -1)
    return -errno;
  return 0;
}

static struct fuse_operations xmp_oper = {
  .getattr	= xmp_getattr,
  .access	= xmp_access,
  .readlink	= xmp_readlink,
  .readdir	= xmp_readdir,
  .mknod	= xmp_mknod,
  .mkdir	= xmp_mkdir,
  .symlink	= xmp_symlink,
  .unlink	= xmp_unlink,
  .rmdir	= xmp_rmdir,
  .rename	= xmp_rename,
  .link		= xmp_link,
  .chmod	= xmp_chmod,
  .chown	= xmp_chown,
  .truncate	= xmp_truncate,
  .utimens	= xmp_utimens,
  .open		= xmp_open,
  .read		= xmp_read,
  .write	= xmp_write,
  .statfs	= xmp_statfs,
  .create       = xmp_create,
  .release	= xmp_release,
  .fsync	= xmp_fsync,
  .setxattr	= xmp_setxattr,
  .getxattr	= xmp_getxattr,
  .listxattr	= xmp_listxattr,
  .removexattr	= xmp_removexattr,
};

void usage() {
  printf("./pa4 <key> <mirrordir> <mountpoint>\n");
}

int main(int argc, char *argv[])
{
  struct data* myargs;
  myargs = malloc(sizeof(struct data));
  if (argc<4){
    usage();
    exit(0);
  }
  myargs->rootdir = realpath(argv[2],NULL);
  myargs->key = argv[1];

  argv[1] = argv[3];
  argv[3] = NULL;
  argv[2] = NULL;
  argc = argc - 2;
  
  umask(0);
  return fuse_main(argc, argv, &xmp_oper, myargs);
}
