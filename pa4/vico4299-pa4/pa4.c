#define FUSE_USE_VERSION 28

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef linux
#define _XOPEN_SOURCE 700
#endif

#include <fuse.h>
#include <stdio.h>
#include <stdlib.h>
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

struct data {
  char* rootdir;
  char* key;
  FILE* log;
};

void pathcat(char fpath[PATH_MAX], const char *path) {
  struct data * ohai = (struct data *)(fuse_get_context()->private_data);
  strcpy(fpath, ohai->rootdir);
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
  int res;
  char newpath[PATH_MAX];
  char *data;
  size_t msize;
  pathcat(newpath, path);
  FILE *in, *mf;
  char iscrypt[8];
  int crypt=-1;
  (void) fi;

  in = fopen(newpath, "r");
  mf = open_memstream(&data, &msize);

  struct data * ohai = (struct data *)(fuse_get_context()->private_data);

  fprintf(ohai->log, "read was called!\n");

  if (in==NULL) {
    return -errno;
  }
  if (mf==NULL) {
    return -errno;
  }

  if (getxattr(newpath, "user.chfs.crypt", iscrypt, 8) != -1){
    if (!strcmp(iscrypt, "true")) {
      crypt=0;
    }
  }

  do_crypt(in, mf, crypt, ohai->key);

  fclose(in);

  fflush(mf);
  fseek(mf, offset, SEEK_SET);
  res = fread(buf, 1, size, mf);
  fclose(mf);

  if (res == -1) {
    fprintf(ohai->log, "shit... read error\n");
    res = -errno;
  }

  return res;
}

static int chfs_write(const char *path, const char *buf, size_t size,
		     off_t offset, struct fuse_file_info *fi)
{
  FILE *mf, *in;
  char *data;
  size_t msize;
  int res;
  char newpath[PATH_MAX];
  pathcat(newpath, path);
  char iscrypt[8];
  int crypt = -1;
  struct data * ohai = (struct data *) (fuse_get_context()->private_data);
  fprintf(ohai->log, "write was called!\n");

  (void) fi;

  in = fopen(newpath, "r");
  mf = open_memstream(&data, &msize);

  if (in == NULL || mf == NULL) {
    return -errno;
  }

  getxattr(newpath, "user.chfs.crypt", iscrypt, 8);
  if (!strcmp(iscrypt, "true")) {
    fprintf(ohai->log, "This file is encrypted\n");
    crypt = 0;
  }


  do_crypt(in, mf, crypt, ohai->key);

  fclose(in);

  fseek(mf, offset, SEEK_SET);

  res = fwrite(buf, 1, size, mf);
  if (res == -1) {
    res = -errno;
  }

  fflush(mf);

  if (crypt == 0) {
    crypt = 1;
  }

  in = fopen(newpath, "w");
  fseek(mf, 0, SEEK_SET);

  do_crypt(mf, in, crypt, ohai->key);

  fclose(mf);
  fclose(in);

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
  (void) mode;
  char newpath[PATH_MAX];
  pathcat(newpath, path);
  FILE * in;
  FILE * temp;
  struct data * ohai = (struct data *) (fuse_get_context()->private_data);

  fprintf(ohai->log, "create was called!\n");

  in = fopen(newpath, "w");
  temp = tmpfile();

  if(in == NULL) {
    fprintf(ohai->log, "cannot open %s\n", newpath);
    return -errno;
  }

  do_crypt(temp, in, 1, ohai->key);
  fclose(temp);

  if(fsetxattr(fileno(in), "user.chfs.crypt", "true", 4, 0)) {
    fprintf(ohai->log, "Cannot set attribute\n");
    return -errno;
  }

  fclose(in);

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

  myargs->log = fopen("chfs.log", "w+");
  if (myargs->log == NULL) {
    printf("Fuck!\n");
    exit(1);
  }

  setvbuf(myargs->log, NULL, _IOLBF, 0);

  fprintf(myargs->log, "beginning log!\n");
  fprintf(myargs->log, "--------------\n");

  umask(0);
  return fuse_main(argc, argv, &chfs_oper, myargs);
}
