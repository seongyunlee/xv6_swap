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
    int a,b;

    char *p = malloc(409600);
    printf(1,"allocate 10 pages%x\n",(int)p);
    swapstat(&a, &b);
    for(int i=0;i<10;i++){
        fork();
    }

    printf(1,"swapstat %d %d\n",a,b);
    exit();
}
