#include "param.h"
#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"
#include "fcntl.h"
#include "syscall.h"
#include "traps.h"
#include "memlayout.h"


int main () {
    

    int *k = (int*)malloc(4);
    for (int i =0 ; i<100;i++){
        printf(1,"when swap? %d\n",i);
        sbrk(409600);
    }
    printf(1,"read after swapping %x %d",(int)k,*k);
    int a,b;
    swapstat(&a,&b);
    printf(1,"swapstat %d %d\n",a,b);
    exit();
}
