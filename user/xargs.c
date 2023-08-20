#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/param.h"
 
int main(int argc, char* argv[])
{
  int len = 0, idx = 0;
  char* exec_argv[MAXARG], buf[512];
  
  for(int i = 1; i < argc; ++i)
    exec_argv[i - 1] = argv[i];
  
  while(1){
    idx = 0;
    // 读取单行输入，一次读取一个字符，直到出现换行符 ('\n')。
    while((len = read(0, buf + idx, sizeof(char))) > 0){
      // 替换'\n'为'\0'
      if(buf[idx] == '\n'){
        buf[idx] = '\0';
        break;
      }
      ++idx;
    }
  
    // 没有命令需要读取了，退出循环
    if(len == 0 && idx == 0)
      break;
    exec_argv[argc - 1] = buf;
    // 使用fork和exec对每一行输入调用命令
    if(fork() == 0){
      exec(exec_argv[0], exec_argv);
      exit(0);
    }
    // 在父级中使用wait等待子级完成命令
    else{
      wait(0);
    }
  }
  exit(0);
}
