#include "param.h"
#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"
#include "fcntl.h"
#include "syscall.h"
#include "traps.h"

#include "memlayout.h"

char* arr[100];

int main () {
    int *k = (int*)malloc(4);
    for (int i =0 ; i<100;i++){
        printf(1,"when swap? %d\n",i);
        char* p = sbrk(409600);
        if(p==(char*)-1) break;
        *p = 'c';
        arr[i]=p;
    }
    printf(1,"allocated done\n");
    for(int i=0;i<100;i++){
        printf(1,"print %d : %x ->%c\n",i,(int)arr[i],*arr[i]);
    }
    printf(1,"read after swapping %x %d",(int)k,*k);
    int a,b;
    swapstat(&a,&b);
    printf(1,"swapstat %d %d\n",a,b);
    exit();
}
