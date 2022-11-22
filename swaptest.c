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
    for (int i =0 ; i<100;i++){
        printf(1,"when swap? %d\n",i);
        sbrk(409600);
    }
    int a,b;
    swapstat(&a,&b);
    printf(1,"swapstat %d %d\n",a,b);
    exit();
}
