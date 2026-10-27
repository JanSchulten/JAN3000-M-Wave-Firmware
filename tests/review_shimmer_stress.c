#define SHIMMER_TEST_NO_MAIN 1
#include "shimmer_send_test.c"
#undef main
int main(void) {
    uint32_t b,i; int p, d;
    ui_power_on();
    for (p=0; p<=0; p++) for (d=0; d<=127; d+=127) {
        int32_t peak=0, late=0;
        reset_shimmer(); shim_p[0]=127; shim_p[1]=d; shim_p[2]=p; shim_p[3]=127;
        for (b=0;b<60000;b++) {
            for(i=0;i<CTL;i++) send_sh[i]=b<2000 ? (((b*CTL+i)&31)?131068:-131068):0;
            if(b<2000) shim_live=1;
            memset(wet,0,sizeof wet); memset(mix_l,0,sizeof mix_l); memset(mix_r,0,sizeof mix_r);
            shimmer_bus(wet,CTL);
            for(i=0;i<CTL;i++){int32_t a=abs(wet[i])+abs(mix_l[i]); if(a>peak)peak=a; if(b>57000 && a>late)late=a;}
        }
        printf("pitch %d damp %d peak %d late %d live %u\n",p,d,peak,late,shim_live); fflush(stdout);
    }
    return 0;
}
