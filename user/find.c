#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

char* fmtname(char *path)
{
  static char buf[DIRSIZ+1];
  char *p;

  // Find first character after last slash.
  for(p=path+strlen(path); p >= path && *p != '/'; p--)
    ;
  p++;

  // Return blank-padded name.
  if(strlen(p) >= DIRSIZ)
    return p;
  memmove(buf, p, strlen(p));
  memset(buf+strlen(p), ' ', DIRSIZ-strlen(p));
  return buf;
}


//findName可能是目录名 或 文件名
void findFile(char *findName,char *path){
  char buf[512], *p;
  int fd;
  struct dirent de;
  struct stat st;

  if((fd = open(path, 0)) < 0){
    fprintf(2, "ls: cannot open %s\n", path);
    return;
  }

  if(fstat(fd, &st) < 0){
    fprintf(2, "ls: cannot stat %s\n", path);
    close(fd);
    return;
  }

  switch(st.type){
  //输入的path是文件直接打印出来
  case T_FILE:
    printf("%s %d %d %l\n", fmtname(path), st.type, st.ino, st.size);
    break;

  case T_DIR:
    if(strlen(path) + 1 + DIRSIZ + 1 > sizeof buf){
      printf("ls: path too long\n");
      break;
    }

    //path 复制到 buf 
    strcpy(buf, path);
    p = buf+strlen(buf);
    //buf 最后加一个/ -> path/
    *p++ = '/';
    
    //读目录下面所有文件
    while(read(fd, &de, sizeof(de)) == sizeof(de)){
      if(de.inum == 0)
        continue;

      // 给 path/ 后面加上这个文件- > path/filename
      memmove(p, de.name, DIRSIZ);
      p[DIRSIZ] = 0;
      if(stat(buf, &st) < 0){
        printf("ls: cannot stat %s\n", buf);
        continue;
      }
      //buf 是 path/filename  
        
      //只取最后的filename
      char *tmpfilename = buf;
      for (char *curP = buf; *curP != '\0'; curP++) {
        if (*curP == '/') {
            tmpfilename = curP + 1;
          }
      }
      //如果buf path/filename 是目录那么递归
      if (st.type==T_DIR){  
        
        if (strcmp(tmpfilename,".")==0 || strcmp(tmpfilename,"..")==0)
        {
            continue;
        }
        
        //目录名比较
        if (strcmp(tmpfilename, findName)==0){
          //printf("%s %d %d %d\n",buf , st.type, st.ino, st.size);
          printf("%s\n",buf);
        }
        
        //递归
        findFile(findName,buf);
      }else if (st.type==T_FILE){
      
        //文件名比较
        if (strcmp(tmpfilename, findName)==0){
          //printf("%s %d %d %d\n",buf , st.type, st.ino, st.size);
          printf("%s\n",buf);
        }
      }    
    }
    break;
  }
  close(fd);
  return;


}



int main(int argc, char *argv[])
{
  if(argc < 3){
    printf("miss parameters\n");
    exit(0);
  }
  char *findName = argv[2];
  char *findPath = argv[1];
  findFile(findName,findPath);
  exit(0);
}
