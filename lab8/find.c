#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"
#include "kernel/fcntl.h"

void
find(char *path, char *name, int shownum)
{
  char buf[512], *p;
  int fd;
  struct dirent de;
  struct stat st;

  if ((fd = open(path, O_RDONLY)) < 0) {
    fprintf(2, "find: cannot open %s\n", path);
    return;
  }
  if (fstat(fd, &st) < 0) {
    fprintf(2, "find: cannot stat %s\n", path);
    close(fd);
    return;
  }
  if (st.type != T_DIR) {
    fprintf(2, "find: %s is not a directory\n", path);
    close(fd);
    return;
  }
  if (strlen(path) + 1 + DIRSIZ + 1 > sizeof buf) {
    fprintf(2, "find: path too long\n");
    close(fd);
    return;
  }

  strcpy(buf, path);
  p = buf + strlen(buf);
  *p++ = '/';

  while (read(fd, &de, sizeof(de)) == sizeof(de)) {
    if (de.inum == 0)
      continue;
    memmove(p, de.name, DIRSIZ);
    p[DIRSIZ] = 0;
    if (strcmp(p, ".") == 0 || strcmp(p, "..") == 0)
      continue;
    if (stat(buf, &st) < 0) {
      fprintf(2, "find: cannot stat %s\n", buf);
      continue;
    }
    if (strcmp(p, name) == 0) {
      if (shownum)
        printf("%s %d\n", buf, (int)st.ino);
      else
        printf("%s\n", buf);
    }
    if (st.type == T_DIR)
      find(buf, name, shownum);
  }
  close(fd);
}

int
main(int argc, char *argv[])
{
  int shownum = 0;

  if (argc < 3) {
    fprintf(2, "usage: find <dir> <name> [-n]\n");
    exit(1);
  }
  if (argc > 3 && strcmp(argv[3], "-n") == 0)
    shownum = 1;
  find(argv[1], argv[2], shownum);
  exit(0);
}
