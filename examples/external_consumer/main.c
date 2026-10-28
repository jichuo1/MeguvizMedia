#include <meguviz/timeline.h>
#include <stdio.h>
int main(void){int64_t ns=0;MgmRational tb={1,90000};if(mgm_ticks_to_ns(45000,tb,&ns)||ns!=500000000)return 1;printf("installed Media C consumer: 45000/90000 ticks = %lld ns\n",(long long)ns);return 0;}
