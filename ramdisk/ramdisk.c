/*
  gcc -Wall ramdisk.c `pkg-config fuse --cflags --libs` -o ramdisk
*/

#define FUSE_USE_VERSION 29

#include <fuse.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

#include "ramdisk.h"

void init()
{
  root = (File *)malloc(sizeof(File));
  strcpy(root->name, "/");

  root->fileOrDir = 0;
  root->next_sameLevel = NULL;
  root->child = NULL;
  root->data = NULL;

  /* metadata for root */
  root->mode = S_IFDIR | 0755;
  root->nlink = 2;
  root->uid = 0;
  root->gid = 0;
  
  time_t t;
  time(&t);
  root->atime = t;
  root->ctime = t;
  root->mtime = t;

  free_space = free_space - (sizeof(File));
}

static char *getFilename(const char *path)
{
  char *fileName = strrchr(strdup(path), '/');

  if(fileName)
  {
    if(fileName[0]!='\0')
      return (char*)(fileName+1);
  }
  return fileName;
}

static File *createNewFile(char *filename)
{
  File *f = (File *)malloc(sizeof(File));
  strcpy(f->name,filename);
  return f;
}

void insertFile(File *parent, File *newFile)
{
  if(parent->child) 
  {
    File *f;
    for(f = parent->child; f->next_sameLevel != NULL; f = f->next_sameLevel)
                        ;
     f->next_sameLevel = newFile;
  }
  else
    parent->child = newFile;
}

/* Returns 1 if path is a exists in the file system, 0 if not*/
int isPathExists(const char *path)
{
  /*Make a copy of path in pathname*/
  char pathname[strlen(path)+1];
  strcpy(pathname, path);

  if(strcmp(pathname, "/") == 0)
    return 1;

  File *itr = root->child;
  char *token = strtok(strdup(pathname),"/");
  int match = 0;
  while(token)
  {
    match = 0;
    while(itr != NULL)
    {
      if(strcmp(itr->name, token) == 0)
      {
     	match = 1;
        break;
      }
      itr = itr->next_sameLevel;
    }
    token = strtok(NULL, "/");
    if(match)
    {
      if(token == NULL)
	return 1;
      else
	itr = itr->child;
    }
    else
	return 0;
  }
  return 0;
}

/* Returns the file node corresponding to the path */
File *resolvePath(const char *path)
{
  /*Make a copy of path in pathname*/
  char pathname[strlen(path)+1];
  strcpy(pathname, path);

  if(strcmp(pathname, "/") == 0)
    return root;

  File *parent = root;
  File *itr = root->child;
  int match = 0;
  char *token = strtok(strdup(pathname),"/");
  while(token)
  {
    match = 0;
    while(itr != NULL)
    {
      if(strcmp(itr->name, token) == 0)
      {
        match = 1;
        token = strtok(NULL, "/");
        if(token == NULL)
        {
          return itr;
        }
	parent = itr;
        itr = itr->child;
        break;
      }
      itr = itr->next_sameLevel;
    }
    if(!match)
    {
      if(strtok(NULL, "/") == NULL)
	return parent;
      break;
    }
  }
  return NULL;
}

static char *getParentDirectory(const char *path)
{
  /*Make a copy of path in pathname*/
  char pathname[strlen(path)+1];
  strcpy(pathname, path);

  int len = strlen(pathname);
  int i,new_len;
  for(i=len-1; i>=0; i--)
  {
    if(pathname[i] == '/')
    {
      if(i == len-1)
        continue;
      else
      {
        new_len=i+1;
        break;
      }
    }
  }
  if(new_len == 1)
    return "/";
  char *parent_dir = malloc(new_len);
  strncpy(parent_dir,pathname,new_len);
  parent_dir[new_len-1] = '\0';
  return parent_dir;
}

static int ramdisk_getattr(const char *path, struct stat *stbuf)
{
  int res = 0;

  //memset(stbuf, 0, sizeof(struct stat));
  if(isPathExists(path))
  {
    File *f = resolvePath(path);
    stbuf->st_mode = f->mode;
    stbuf->st_nlink = f->nlink;
    stbuf->st_uid = f->uid;
    stbuf->st_gid = f->gid;
    stbuf->st_size = f->size;
    stbuf->st_atime = f->atime;
    stbuf->st_ctime = f->ctime;
    stbuf->st_mtime = f->mtime;
  }
  else
    res = -ENOENT;

  return res;
}

static int ramdisk_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
			 off_t offset, struct fuse_file_info *fi)
{
  //(void) offset;
  //(void) fi;

  if (!isPathExists(path))
    return -ENOENT;

  File *f = resolvePath(path);

  filler(buf, ".", NULL, 0);
  filler(buf, "..", NULL, 0);
  File *itr;
  for(itr = f->child; itr != NULL; itr = itr->next_sameLevel)
    filler(buf, itr->name, NULL, 0);

  time_t t;
  time(&t);
  f->atime = t;

  return 0;
}

static int ramdisk_open(const char *path, struct fuse_file_info *fi)
{
	if (!isPathExists(path))
		return -ENOENT;
	//if ((fi->flags & 3) != O_RDONLY)
	//	return -EACCES;
	return 0;
}

static int ramdisk_read(const char *path, char *buf, size_t size, off_t offset,
		      struct fuse_file_info *fi)
{
  size_t len;
  (void) fi;

  if(!isPathExists(path))
    return -ENOENT;

  File *f = resolvePath(path);
  if(f->fileOrDir == 0)
    return -EISDIR;

  len = f->size;
  if (offset < len) 
  {
    if (offset + size > len)
      size = len - offset;
    memcpy(buf, f->data + offset, size);
  }
  else
    size = 0;

  /* update time of last access*/
  if(size > 0)
  {
    time_t t;
    time(&t);
    f->atime = t;
  }

  return size;
}

static int ramdisk_write(const char *path, const char *buf, size_t size,
                     off_t offset, struct fuse_file_info *fi)
{
  if(size > free_space)
    return -ENOSPC;

  File *f = resolvePath(path);
  if(f->fileOrDir == 0)
    return -EISDIR;

  size_t len = f->size;
  if(size > 0)
  {
    if(len == 0)
    {
      offset = 0;
      f->data = (char *)malloc(sizeof(char)*size);
      memcpy(f->data + offset, buf, size);
      free_space = free_space - size;
    }
    else
    {
      if(offset > len)
	offset = len;
      char *write_data = (char *)realloc(f->data, sizeof(char)*(offset + size));
      if(write_data == NULL)
	return -ENOSPC;
      else
      {
	f->data = write_data;
        memcpy(f->data + offset, buf, size);
 	free_space = free_space - (offset + size - len);
      }
    }
    f->size = offset + size;
    time_t t;
    time(&t);
    f->atime = t; // is updating atime and mtime correct
    f->mtime = t;
  }
  return size;
}

/* creates and open a file*/
static int ramdisk_create(const char *path, mode_t mode, struct fuse_file_info *fi)
{
  if(isPathExists(path))
    return 0;

  if(free_space < 0)
    return -ENOSPC;

  File *parent = resolvePath(path);
  File *f = createNewFile(getFilename(path));

  if(f == NULL)
    return -ENOSPC;
  
  f->fileOrDir = 1;
  f->next_sameLevel = NULL;
  f->child = NULL;

  f->mode = S_IFREG | mode;
  f->nlink = 1;
  f->uid = getuid();
  f->gid = getgid();
  f->size = 0;
  f->data = NULL;

  time_t t;
  time(&t);
  f->atime = t;
  f->ctime = t;
  f->mtime = t;

  free_space = free_space - sizeof(File);
  insertFile(parent, f);
  
  return 0;
}

static int ramdisk_mkdir(const char *path, mode_t mode)
{
  if(isPathExists(path))
    return -EEXIST;

  File *parent = resolvePath(path);
  File *f = createNewFile(getFilename(path));

  if(f == NULL)
    return -ENOSPC;
  
  free_space = free_space - sizeof(File);
  if(free_space < 0)
    return -ENOSPC;

  f->fileOrDir = 0;
  f->next_sameLevel = NULL;
  f->child = NULL;

  f->mode = S_IFDIR | mode;
  f->nlink = 2;
  f->uid = getuid();
  f->gid = getgid();
  f->size = 4096;
  f->data = NULL;

  time_t t;
  time(&t);
  f->atime = t;
  f->ctime = t;

  insertFile(parent, f);
  parent->nlink = parent->nlink+1;
  return 0;
}

static int ramdisk_rmdir(const char *path)
{
  if(!isPathExists(path))
    return -ENOENT;

  File *f = resolvePath(path);

  /*rmdir deletes only empty directory*/
  if(f->child)
    return -ENOTEMPTY;

  /*Handle delete*/
  File *parent = resolvePath(getParentDirectory(path));
  if(parent->child == f)
  {
    if(f->next_sameLevel)
      parent->child = f->next_sameLevel;
    else
      parent->child = NULL;
  }
  else
  {
    File *itr;
    for(itr = parent->child; itr!=NULL; itr = itr->next_sameLevel)
    {
      if(itr->next_sameLevel == f)
      {
	itr->next_sameLevel = f->next_sameLevel;
	break;
      }
    }
  }
  parent->nlink = parent->nlink - 1;
  free(f);
  free_space = free_space + sizeof(File);
  return 0;
}

static int ramdisk_unlink(const char *path)
{
  if(!isPathExists(path))
    return -ENOENT;

  File *f = resolvePath(path);

  /*Handle delete*/
  File *parent = resolvePath(getParentDirectory(path));
  if(parent->child == f)
  {
    if(f->next_sameLevel)
      parent->child = f->next_sameLevel;
    else
      parent->child = NULL;
  }
  else
  {
    File *itr;
    for(itr = parent->child; itr!=NULL; itr = itr->next_sameLevel)
    {
      if(itr->next_sameLevel == f)
      {
	itr->next_sameLevel = f->next_sameLevel;
	break;
      }
    }
  }

  if(f->fileOrDir == 0)
    parent->nlink = parent->nlink - 1; //If f is directory decrement nlink
  if(f->size != 0)
    free(f->data);
  free(f);
  free_space = free_space + sizeof(File) + f->size;
  return 0;
}

static int ramdisk_utime(const char *path, struct utimbuf *ubuf)
{
  return 0;
}

static int ramdisk_rename(const char *from, const char *to)
{
  if(!isPathExists(from))
    return -ENOENT;

  if(!isPathExists(getParentDirectory(to)))
    return -ENOENT;

  File *from_file = resolvePath(from);
  File *from_parent = resolvePath(getParentDirectory(from));
  File *to_parent = resolvePath(getParentDirectory(to));

  if(from_parent == to_parent)
  {
    if(isPathExists(to))
    {
      File *to_file = resolvePath(to);
      if((from_file->fileOrDir == 1) && (to_file->fileOrDir == 1))
      {
        ramdisk_unlink(to);
        memset(from_file->name, 0, 255);
        strcpy(from_file->name, getFilename(to));
      }
    }
    else
    {
      memset(from_file->name, 0, 255);
      strcpy(from_file->name, getFilename(to));
    }
    return 0;
  }
  else
  {
    if(isPathExists(to))
    {
      File *to_file = resolvePath(to);
      if((from_file->fileOrDir == 1) && (to_file->fileOrDir == 1))
      {
 	memset(to_file->data, 0, to_file->size);
	if(from_file->size > 0) {
        to_file->data = (char *)realloc(to_file->data, sizeof(char)*from_file->size);
	if(to_file->data) {
	  strcpy(to_file->data, from_file->data);
	  free_space = free_space - from_file->size;
	}
	else
	  return -ENOSPC;
        }

	to_file->size = from_file->size;
  	to_file->mode = from_file->mode;
  	to_file->nlink = from_file->nlink;
  	to_file->uid = from_file->uid;
  	to_file->gid = from_file->gid;

  	to_file->atime = from_file->atime;
  	to_file->ctime = from_file->ctime;
  	to_file->mtime = from_file->mtime;
	ramdisk_unlink(from);
      }
    }
    else
    {
      if((from_file->fileOrDir == 1))
      {
	ramdisk_create(to,from_file->mode, NULL);
	File *to_file = resolvePath(to);
	to_file->fileOrDir = 1;

	if(from_file->size > 0) {
        to_file->data = (char *)malloc(sizeof(char)*from_file->size);
	if(to_file->data) {
	  strcpy(to_file->data, from_file->data);
	  free_space = free_space - from_file->size;
	}
	else
	 return -ENOSPC;
	}

	to_file->size = from_file->size;
  	to_file->mode = from_file->mode;
  	to_file->nlink = from_file->nlink;
  	to_file->uid = from_file->uid;
  	to_file->gid = from_file->gid;

  	to_file->atime = from_file->atime;
  	to_file->ctime = from_file->ctime;
  	to_file->mtime = from_file->mtime;
	ramdisk_unlink(from);
      }
    }
  }
  
  return 0;
}

static struct fuse_operations ramdisk_oper = {
	.getattr	= ramdisk_getattr,
	.readdir	= ramdisk_readdir,
	.open		= ramdisk_open,
	.read		= ramdisk_read,
	.create 	= ramdisk_create,
	.mkdir 		= ramdisk_mkdir,
	.rmdir 		= ramdisk_rmdir,
	.unlink		= ramdisk_unlink,
     	.write 		= ramdisk_write,
	.utime		= ramdisk_utime,
	.rename		= ramdisk_rename,
};

int main(int argc, char *argv[])
{
  if(argc != 3)
  {
    printf("Error : Arguments too short or long: Usage : ramdisk /path/to/dir size\n");
    return -1;
  }

  long fs_size = ((long)atoi(argv[2]))*1024*1024;
  free_space = fs_size;
  init();
  argc--;
  return fuse_main(argc, argv, &ramdisk_oper, NULL);
}
