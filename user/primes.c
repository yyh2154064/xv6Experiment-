#include "kernel/types.h"
#include "user/user.h"
 
// 子进程调用函数
void process(int left_to_right[])
{
  int prime, curNum;
  int new_left_to_right[2];
  
  pipe(new_left_to_right);
  
  // 留在left_to_right中的都是质数，此质数作为本进程的被除数
  if(read(left_to_right[0], &prime, sizeof(int)) <= 0)
    return;
  fprintf(1, "prime %d\n", prime);
  
  // 子进程作为右邻居，关闭写管道文件描述符
  if(fork() == 0){
    close(new_left_to_right[1]);
    process(new_left_to_right);
    close(new_left_to_right[0]);
  }
  else{
    close(new_left_to_right[0]);
    // 筛选，能被prime整除的淘汰，否则输入到右邻居进行处理
    while(read(left_to_right[0], &curNum, sizeof(int)) > 0){
      if(curNum % prime != 0){
        write(new_left_to_right[1], &curNum, sizeof(int));
      }
    }
    close(new_left_to_right[1]);
    wait(0);
  }
  close(left_to_right[0]);
  close(left_to_right[1]);
}
 
int main(int argc, char* argv[])
{
  int left_to_right[2], i;
  pipe(left_to_right);
  
  if(fork() == 0){
    close(left_to_right[1]);
    process(left_to_right);
    close(left_to_right[0]);
  }
  else{
    close(left_to_right[0]);
    // 读入数据，2就是质数，因此不需要处理
    for(i = 2; i <= 35; ++i){
      write(left_to_right[1], &i, sizeof(int));   
    }
    close(left_to_right[1]);
    wait(0);
  }
  exit(0);
}
