#include <unistd.h>

typedef struct _file
{
  char name[256];
  int fileOrDir; // 0 for dir, 1 for file
  struct _file *next_sameLevel;
  struct _file *child;
  char *data;
  mode_t mode; /* file type and mode*/
  nlink_t nlink; /* number of hard links*/
  uid_t uid; /* user ID of owner */
  gid_t gid; /* group ID of owner */
  off_t size; /* total size in bytes */
  time_t atime; /* time of last access */ 
  time_t mtime; /* time of last modification */ 
  time_t ctime; /* time of last status change */ 
}File;

File *root;
long free_space; /* Available memory space */

void init();
int isPathExists(const char *path);
File *resolvePath(const char *path);
void insertFile(File *path, File *f);
//void delete_file(file *f);
