#define FUSE_USE_VERSION 28

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
#include <sys/time.h>
#include <sys/xattr.h>

#include "aes-crypt.h"

#define DATA ((struct data *) fuse_get_context()->private_data)

struct data {
  char* rootdir;
  char* key;
};

void pathcat(char fpath[PATH_MAX], const char *path) {
  strcpy(fpath, DATA->rootdir);
  strncat(fpath, path, PATH_MAX);
}

static int chfs_getattr(const char *path, struct stat *stbuf)
{
  int res;
  char newpath[PATH_MAX];
  pathcat(newpath, path);

  res = lstat(newpath, stbuf);
  if (res == -1)
    return -errno;

  return 0;
}

static int chfs_access(const char *path, int mask)
{
  int res;
  char newpath[PATH_MAX];
  pathcat(newpath, path);


  res = access(newpath, mask);
  if (res == -1)
    return -errno;

  return 0;
}

static int chfs_readlink(const char *path, char *buf, size_t size)
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


static int chfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
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

static int chfs_mknod(const char *path, mode_t mode, dev_t rdev)
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

static int chfs_mkdir(const char *path, mode_t mode)
{
  int res;
  char newpath[PATH_MAX];
  pathcat(newpath, path);

  res = mkdir(newpath, mode);
  if (res == -1)
    return -errno;

  return 0;
}

static int chfs_unlink(const char *path)
{
  int res;
  char newpath[PATH_MAX];
  pathcat(newpath, path);

  res = unlink(newpath);
  if (res == -1)
    return -errno;

  return 0;
}

static int chfs_rmdir(const char *path)
{
  int res;
  char newpath[PATH_MAX];
  pathcat(newpath, path);

  res = rmdir(newpath);
  if (res == -1)
    return -errno;

  return 0;
}

static int chfs_symlink(const char *from, const char *to)
{
  int res;

  res = symlink(from, to);
  if (res == -1)
    return -errno;

  return 0;
}

static int chfs_rename(const char *from, const char *to)
{
  int res;

  res = rename(from, to);
  if (res == -1)
    return -errno;

  return 0;
}

static int chfs_link(const char *from, const char *to)
{
  int res;

  res = link(from, to);
  if (res == -1)
    return -errno;

  return 0;
}

static int chfs_chmod(const char *path, mode_t mode)
{
  int res;
  char newpath[PATH_MAX];
  pathcat(newpath, path);

  res = chmod(newpath, mode);
  if (res == -1)
    return -errno;

  return 0;
}

static int chfs_chown(const char *path, uid_t uid, gid_t gid)
{
  int res;
  char newpath[PATH_MAX];
  pathcat(newpath, path);

  res = lchown(newpath, uid, gid);
  if (res == -1)
    return -errno;

  return 0;
}

static int chfs_truncate(const char *path, off_t size)
{
  int res;
  char newpath[PATH_MAX];
  pathcat(newpath, path);

  res = truncate(newpath, size);
  if (res == -1)
    return -errno;

  return 0;
}

static int chfs_utimens(const char *path, const struct timespec ts[2])
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

static int chfs_open(const char *path, struct fuse_file_info *fi)
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

static int chfs_read(const char *path, char *buf, size_t size, off_t offset,
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

static int chfs_write(const char *path, const char *buf, size_t size,
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

static int chfs_statfs(const char *path, struct statvfs *stbuf)
{
  int res;
  char newpath[PATH_MAX];
  pathcat(newpath, path);

  res = statvfs(newpath, stbuf);
  if (res == -1)
    return -errno;

  return 0;
}

static int chfs_create(const char* path, mode_t mode, struct fuse_file_info* fi)
{
  (void) fi;
  char newpath[PATH_MAX];
  char temppath[PATH_MAX];
  pathcat(newpath, path);
  strcpy(temppath, newpath);
  strcat(temppath, ".enc");
  FILE * in;
  FILE * temp;
  char iscrypt[8];

  getxattr(newpath, "user.chfs.crypt", iscrypt, 8);

  int res;
  res = creat(newpath, mode);
  if(res == -1)
    return -errno; 

  close(res);

  in = fopen(newpath, "rb"); 
  temp = fopen(temppath, "wb+");

  if(!do_crypt(in, temp, 1, DATA->key)){
    fprintf(stderr, "do_crypt failed\n");
  }

  fclose(in);
  fclose(temp);

  setxattr(newpath, "user.chfs.crypt", "true", sizeof("true"), 0);
  
  return 0;
}


static int chfs_release(const char *path, struct fuse_file_info *fi)
{
  (void) path;
  (void) fi;
  return 0;
}

static int chfs_fsync(const char *path, int isdatasync,
		     struct fuse_file_info *fi)
{
  (void) path;
  (void) isdatasync;
  (void) fi;
  return 0;
}

static int chfs_setxattr(const char *path, const char *name, const char *value,
			size_t size, int flags)
{
  char newpath[PATH_MAX];
  pathcat(newpath, path);
  int res = lsetxattr(newpath, name, value, size, flags);
  if (res == -1)
    return -errno;
  return 0;
}

static int chfs_getxattr(const char *path, const char *name, char *value,
			size_t size)
{
  char newpath[PATH_MAX];
  pathcat(newpath, path);
  int res = lgetxattr(newpath, name, value, size);
  if (res == -1)
    return -errno;
  return res;
}

static int chfs_listxattr(const char *path, char *list, size_t size)
{
  char newpath[PATH_MAX];
  pathcat(newpath, path);
  int res = llistxattr(newpath, list, size);
  if (res == -1)
    return -errno;
  return res;
}

static int chfs_removexattr(const char *path, const char *name)
{
  char newpath[PATH_MAX];
  pathcat(newpath, path);
  int res = lremovexattr(newpath, name);
  if (res == -1)
    return -errno;
  return 0;
}

static struct fuse_operations chfs_oper = {
  .getattr	= chfs_getattr,
  .access	= chfs_access,
  .readlink	= chfs_readlink,
  .readdir	= chfs_readdir,
  .mknod	= chfs_mknod,
  .mkdir	= chfs_mkdir,
  .symlink	= chfs_symlink,
  .unlink	= chfs_unlink,
  .rmdir	= chfs_rmdir,
  .rename	= chfs_rename,
  .link		= chfs_link,
  .chmod	= chfs_chmod,
  .chown	= chfs_chown,
  .truncate	= chfs_truncate,
  .utimens	= chfs_utimens,
  .open		= chfs_open,
  .read		= chfs_read,
  .write	= chfs_write,
  .statfs	= chfs_statfs,
  .create       = chfs_create,
  .release	= chfs_release,
  .fsync	= chfs_fsync,
  .setxattr	= chfs_setxattr,
  .getxattr	= chfs_getxattr,
  .listxattr	= chfs_listxattr,
  .removexattr	= chfs_removexattr,
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
  return fuse_main(argc, argv, &chfs_oper, myargs);
}
