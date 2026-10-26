#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <limits.h>

#define N 10000
#define TRIALS 100
#define ALGORITHMS 4

typedef struct Data { int key1, key2, order; } Data;
typedef struct Stats { uint64_t comparisons, movements; } Stats;
const char *names[ALGORITHMS] = {"Selection", "Bubble", "Bubble early", "Insertion (>)"};

/* RAND_MAX가 32767인 환경에서도 0~1000000 전체를 생성한다. */
unsigned random_below(unsigned bound)
{
    const uint32_t range = UINT32_C(1) << 30;
    uint32_t limit = range - range % bound, value;
    do {
        value = ((uint32_t)(rand() & 32767) << 15) | (uint32_t)(rand() & 32767);
    } while (value >= limit);
    return value % bound;
}
void generate(Data *a, int n)
{
    int i;
    for (i=0;i<n;i++) {
        a[i].key1=(int)random_below(1000001);
        a[i].key2=(int)random_below(1000001);
        a[i].order=i;
    }
}
void swap_data(Data *a, Data *b, Stats *s)
{
    Data temp=*a; *a=*b; *b=temp;
    s->movements+=3;
}
Stats selection_sort(Data *a,int n)
{
    Stats s={0,0}; int i,j;
    for(i=0;i<n-1;i++) {
        int minimum=i;
        for(j=i+1;j<n;j++) {
            s.comparisons++;
            if(a[j].key1<a[minimum].key1)minimum=j;
        }
        if(minimum!=i)swap_data(&a[i],&a[minimum],&s);
    }
    return s;
}
Stats bubble_sort(Data *a,int n,int early)
{
    Stats s={0,0};int end,j;
    for(end=n-1;end>0;end--) {
        int changed=0;
        for(j=0;j<end;j++) {
            s.comparisons++;
            if(a[j].key1>a[j+1].key1){swap_data(&a[j],&a[j+1],&s);changed=1;}
        }
        if(early&&!changed)break;
    }
    return s;
}
Stats insertion_sort(Data *a,int n,int equal_moves)
{
    Stats s={0,0};int i;
    for(i=1;i<n;i++) {
        Data current=a[i];int j=i-1;
        s.movements++;
        while(j>=0) {
            int move;
            s.comparisons++;
            move=equal_moves ? a[j].key1>=current.key1 : a[j].key1>current.key1;
            if(!move)break;
            a[j+1]=a[j];s.movements++;j--;
        }
        a[j+1]=current;s.movements++;
    }
    return s;
}
Stats run_sort(Data *a,int n,int algorithm)
{
    if(algorithm==0)return selection_sort(a,n);
    if(algorithm==1)return bubble_sort(a,n,0);
    if(algorithm==2)return bubble_sort(a,n,1);
    return insertion_sort(a,n,0);
}
/* 키 순서뿐 아니라 두 키의 보존과 원래 상대 순서도 검사한다. */
int verify(const Data *original,const Data *a,int n,int *stable)
{
    unsigned char seen[N]={0};int i;*stable=1;
    for(i=0;i<n;i++) {
        int id=a[i].order;
        if(id<0||id>=n||seen[id])return 0;
        seen[id]=1;
        if(a[i].key1!=original[id].key1||a[i].key2!=original[id].key2)return 0;
        if(i>0) {
            if(a[i-1].key1>a[i].key1)return 0;
            if(a[i-1].key1==a[i].key1&&a[i-1].order>a[i].order)*stable=0;
        }
    }
    return 1;
}
void print_stats(const char *name,Stats s,int trials)
{
    printf("%s\nTotal Comparisons: %" PRIu64 "\nAverage Comparisons: %.2f\n",name,s.comparisons,(double)s.comparisons/trials);
    printf("Total Movements: %" PRIu64 "\nAverage Movements: %.2f\n",s.movements,(double)s.movements/trials);
}
int state_experiment(const char *name,Data *input,Data *work)
{
    int algorithm,i;
    /* 입력 상태를 만든 뒤의 순서가 이번 실험의 원래 순서이다. */
    for(i=0;i<N;i++)input[i].order=i;
    printf("\n%s (one trial, %d records)\n",name,N);
    for(algorithm=0;algorithm<ALGORITHMS;algorithm++) {
        Stats s;int stable;
        memcpy(work,input,N*sizeof(Data));s=run_sort(work,N,algorithm);
        if(!verify(input,work,N,&stable)||(algorithm>0&&!stable))return 0;
        printf("%s: Comparisons=%" PRIu64 ", Movements=%" PRIu64 ", Correct=PASS, Stable on this input=%s\n",
               names[algorithm],s.comparisons,s.movements,stable?"Yes":"No");
    }
    return 1;
}
void print_records(const Data *a,int n)
{
    int i;for(i=0;i<n;i++)printf("(%d,%d,#%d)%s",a[i].key1,a[i].key2,a[i].order,i+1==n?"\n":" ");
}
int stability_demo(void)
{
    Data input[]={{527,100,0},{527,200,1},{125,300,2},{892,400,3}}, work[4];
    int algorithm;
    puts("\nStability Verification: (key1,key2,#original order)");
    printf("Original: ");print_records(input,4);
    for(algorithm=0;algorithm<5;algorithm++) {
        int stable;
        memcpy(work,input,sizeof(input));
        if(algorithm==4)insertion_sort(work,4,1);else run_sort(work,4,algorithm);
        if(!verify(input,work,4,&stable))return 0;
        if(stable!=(algorithm>0&&algorithm<4))return 0;
        printf("%s: %s\n",algorithm==4?"Insertion (>=)":names[algorithm],stable?"Stable":"Unstable");
        print_records(work,4);
    }
    return 1;
}
int main(int argc,char **argv)
{
    unsigned seed=(unsigned)time(NULL);int trial,algorithm,i;
    Stats total[ALGORITHMS]={{0,0}};
    int unstable[ALGORITHMS]={0};
    Data *input,*work,*base,*state;
    if(argc>2){fprintf(stderr,"Usage: %s [seed]\n",argv[0]);return 1;}
    if(argc==2){char *end;unsigned long v;errno=0;v=strtoul(argv[1],&end,10);
        if(argv[1][0]=='-'||end==argv[1]||*end||errno==ERANGE||v>UINT_MAX){fputs("Invalid seed\n",stderr);return 1;}seed=(unsigned)v;}
    input=(Data*)malloc(N*sizeof(Data));work=(Data*)malloc(N*sizeof(Data));
    base=(Data*)malloc(N*sizeof(Data));state=(Data*)malloc(N*sizeof(Data));
    if(!input||!work||!base||!state){free(input);free(work);free(base);free(state);return 1;}
    srand(seed);
    printf("Seed: %u\nData Count: %d\nRandom Data\nNumber of Trials: %d\n",seed,N,TRIALS);fflush(stdout);
    for(trial=0;trial<TRIALS;trial++) {
        generate(input,N);
        if(trial==0)memcpy(base,input,N*sizeof(Data));
        for(algorithm=0;algorithm<ALGORITHMS;algorithm++) {
            Stats s;int stable;
            memcpy(work,input,N*sizeof(Data));s=run_sort(work,N,algorithm);
            if(!verify(input,work,N,&stable)||(algorithm>0&&!stable))goto failure;
            unstable[algorithm]+=!stable;
            total[algorithm].comparisons+=s.comparisons;total[algorithm].movements+=s.movements;
        }
        if((trial+1)%10==0){fprintf(stderr,"Random trials completed: %d/%d\n",trial+1,TRIALS);}
    }
    for(algorithm=0;algorithm<ALGORITHMS;algorithm++) {
        print_stats(names[algorithm],total[algorithm],TRIALS);
        printf("Correct results: 100/100; Trials violating stability: %d\n\n",unstable[algorithm]);
    }
    /* 시험 준비 비용은 집계하지 않는다. 최초 무작위 입력의 동일한 값들을 사용한다. */
    insertion_sort(base,N,0);
    memcpy(state,base,N*sizeof(Data));
    if(!state_experiment("Sorted Data",state,work))goto failure;
    for(i=0;i<N;i++)state[i]=base[N-1-i];
    if(!state_experiment("Reverse-Sorted Data",state,work))goto failure;
    memcpy(state,base,N*sizeof(Data));
    {
        int positions[N/2],changed=0;
        for(i=0;i<N/2;i++)positions[i]=i;
        for(i=0;i<100;i++) {
            int j=i+(int)random_below(N/2-i),t=positions[i],p;Data temp;
            positions[i]=positions[j];positions[j]=t;p=2*positions[i];
            if(state[p].key1!=state[p+1].key1)changed++;
            temp=state[p];state[p]=state[p+1];state[p+1]=temp;
        }
        printf("\nPartial input: 100 disjoint adjacent pair swaps (200 records); strict inversions=%d\n",changed);
    }
    if(!state_experiment("Partially Sorted Data",state,work)||!stability_demo())goto failure;
    puts("\nAll sorting, record preservation and expected stability checks: PASS");
    free(input);free(work);free(base);free(state);return 0;
failure:
    fputs("Validation failed\n",stderr);free(input);free(work);free(base);free(state);return 1;
}
