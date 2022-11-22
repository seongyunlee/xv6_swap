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
    int *a, *b;

    a = malloc(40960);
    swapstat(a, b);
    for(int i=0;i<10;i++){
        fork();
    }
    

    printf(1,"swapstat %d %d\n",*a,*b);
    exit();
}
