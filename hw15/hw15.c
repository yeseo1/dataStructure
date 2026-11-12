#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <limits.h>
#ifdef _WIN32
#include <windows.h>
#endif

#define RUNS 10
typedef struct Node { int key, count, height; struct Node *left, *right; } Node;
typedef struct { int insert, value; } Operation;
typedef struct {
    uint64_t comparisons, moves;
    size_t storage, stack, peak, nodes, peak_nodes;
} Stats;
/* ABI 스택 실측이 아닌 소스 수준 활성 레코드 모델. */
typedef struct { int *a; Stats *s; int lo, hi, mid, p, pivot, i, j, t; } QuickFrame;
typedef struct { int *a, *length; Stats *s; int value, pos, i; } ArrayFrame;
typedef struct { Node *node, *next; Stats *s; int key, result; } TreeFrame;
typedef struct { Node *node, *child, *middle; int balance, h1, h2; } BalanceFrame;
typedef struct { Node *node, **minimum; Stats *s; } MinFrame;

static void peak(Stats *s) {
    size_t bytes = s->storage + s->stack;
    if (bytes > s->peak) s->peak = bytes;
    if (s->nodes > s->peak_nodes) s->peak_nodes = s->nodes;
}
static void enter(Stats *s, size_t bytes) { s->stack += bytes; peak(s); }
static void leave(Stats *s, size_t bytes) { s->stack -= bytes; }
static unsigned random_below(unsigned bound) {
    const uint32_t range = UINT32_C(1) << 30;
    uint32_t v;
    do { v = ((uint32_t)(rand() & 32767) << 15) | (uint32_t)(rand() & 32767); }
    while (v >= range - range % bound);
    return v % bound;
}
static int less(int a, int b, Stats *s) { ++s->comparisons; return a < b; }
static int equal(int a, int b, Stats *s) { ++s->comparisons; return a == b; }
static void exchange(int *a, int *b, Stats *s) {
    int t = *a; *a = *b; *b = t; s->moves += 3;
}
/* 과제 12~14의 Median-of-Three와 동일한 Lomuto 분할. */
static void quick_sort(int *a, int lo, int hi, Stats *s) {
    enter(s, sizeof(QuickFrame));
    if (lo < hi) {
        int mid = lo + (hi-lo)/2, p, pivot, i=lo, j;
        if (less(a[lo],a[mid],s)) {
            if (less(a[mid],a[hi],s)) p=mid;
            else p=less(a[lo],a[hi],s)?hi:lo;
        } else {
            if (less(a[lo],a[hi],s)) p=lo;
            else p=less(a[mid],a[hi],s)?hi:mid;
        }
        exchange(&a[p],&a[hi],s);
        pivot=a[hi]; ++s->moves;
        for(j=lo;j<hi;++j) if(less(a[j],pivot,s)) {exchange(&a[i],&a[j],s);++i;}
        exchange(&a[i],&a[hi],s);
        quick_sort(a,lo,i-1,s); quick_sort(a,i+1,hi,s);
    }
    leave(s,sizeof(QuickFrame));
}
static int array_operation(int *a,int *length,Operation op,Stats *s) {
    int pos=0,i,removed=0;
    enter(s,sizeof(ArrayFrame));
    if(op.insert) {
        while(pos<*length) {if(!less(a[pos],op.value,s))break;++pos;}
        for(i=*length;i>pos;--i){a[i]=a[i-1];++s->moves;}
        a[pos]=op.value;++s->moves;++*length;
    } else {
        while(pos<*length) {if(equal(a[pos],op.value,s))break;++pos;}
        if(pos<*length) {
            for(i=pos;i<*length-1;++i){a[i]=a[i+1];++s->moves;}
            --*length;removed=1;
        }
    }
    leave(s,sizeof(ArrayFrame));return removed;
}
static int height(Node *p){return p?p->height:0;}
static void update(Node *p){int a=height(p->left),b=height(p->right);p->height=1+(a>b?a:b);}
static Node *right_rotate(Node *p){Node *q=p->left;p->left=q->right;q->right=p;update(p);update(q);return q;}
static Node *left_rotate(Node *p){Node *q=p->right;p->right=q->left;q->left=p;update(p);update(q);return q;}
static Node *balance(Node *p,Stats *s) {
    int b;
    if(!p)return p;
    enter(s,sizeof(BalanceFrame));update(p);b=height(p->left)-height(p->right);
    if(b>1){if(height(p->left->left)<height(p->left->right))p->left=left_rotate(p->left);p=right_rotate(p);}
    else if(b < -1){if(height(p->right->right)<height(p->right->left))p->right=right_rotate(p->right);p=left_rotate(p);}
    leave(s,sizeof(BalanceFrame));return p;
}
static Node *insert(Node *p,int key,Stats *s) {
    enter(s,sizeof(TreeFrame));
    if(!p){
        p=malloc(sizeof(*p));if(!p){fputs("Allocation failed\n",stderr);exit(1);}
        p->key=key;++s->moves;p->count=1;p->height=1;p->left=p->right=NULL;
        s->storage+=sizeof(*p);++s->nodes;peak(s);
    } else if(equal(key,p->key,s)) ++p->count;
    else {if(less(key,p->key,s))p->left=insert(p->left,key,s);else p->right=insert(p->right,key,s);p=balance(p,s);}
    leave(s,sizeof(TreeFrame));return p;
}
/* 최소 노드 전체를 떼어낸다. 중복 개수도 함께 승계해야 한다. */
static Node *detach_min(Node *p,Node **minimum,Stats *s) {
    enter(s,sizeof(MinFrame));
    if(!p->left){*minimum=p;p=p->right;}
    else {p->left=detach_min(p->left,minimum,s);p=balance(p,s);}
    leave(s,sizeof(MinFrame));return p;
}
static Node *erase(Node *p,int key,Stats *s,int *removed) {
    enter(s,sizeof(TreeFrame));
    if(p){
        if(equal(key,p->key,s)){
            *removed=1;
            if(p->count>1)--p->count;
            else if(!p->left||!p->right){Node *next=p->left?p->left:p->right;free(p);p=next;s->storage-=sizeof(Node);--s->nodes;}
            else {
                Node *next=NULL;p->right=detach_min(p->right,&next,s);
                p->key=next->key;++s->moves;p->count=next->count;
                free(next);s->storage-=sizeof(Node);--s->nodes;p=balance(p,s);
            }
        } else {if(less(key,p->key,s))p->left=erase(p->left,key,s,removed);else p->right=erase(p->right,key,s,removed);p=balance(p,s);}
    }
    leave(s,sizeof(TreeFrame));return p;
}
static void destroy(Node *p){if(p){destroy(p->left);destroy(p->right);free(p);}}
/* 검증용 중위 순회: 정렬·균형·개수·저장된 높이를 검사한다. */
static int inorder(Node *p,int *out,int *used,int capacity,int *ok) {
    int l,r,i;
    if(!p)return 0;
    l=inorder(p->left,out,used,capacity,ok);
    if(p->count<1)*ok=0;
    if(*used && out[*used-1]>=p->key)*ok=0;
    for(i=0;i<p->count;++i){if(*used>=capacity){*ok=0;return 0;}out[(*used)++]=p->key;}
    r=inorder(p->right,out,used,capacity,ok);
    if(abs(l-r)>1||p->height!=1+(l>r?l:r))*ok=0;
    return 1+(l>r?l:r);
}
static int same(const int *a,int n,Node *root,int *out,int capacity){
    int used=0,ok=1,i;inorder(root,out,&used,capacity,&ok);
    if(!ok||used!=n)return 0;
    for(i=0;i<n;++i)if(a[i]!=out[i]||(i&&a[i-1]>a[i]))return 0;
    return 1;
}
#ifdef _WIN32
static double frequency;
static void timer_init(void){LARGE_INTEGER f;if(!QueryPerformanceFrequency(&f))exit(1);frequency=(double)f.QuadPart;}
static double now_ms(void){LARGE_INTEGER t;QueryPerformanceCounter(&t);return t.QuadPart*1000.0/frequency;}
#else
static void timer_init(void){}
static double now_ms(void){return (double)clock()*1000.0/CLOCKS_PER_SEC;}
#endif

int main(int argc,char **argv){
    const int sizes[]={1000,2000,4000,8000,16000,20000};unsigned seed=20261104U;
    int z;
    if(argc>2){fputs("Usage: hw15 [seed]\n",stderr);return 1;}
    if(argc==2){char *end;unsigned long v;errno=0;v=strtoul(argv[1],&end,10);if(argv[1][0]=='-'||end==argv[1]||*end||errno==ERANGE||v>UINT_MAX)return 1;seed=(unsigned)v;}
    timer_init();printf("Seed: %u; Quick pivot: Median-of-Three\nTime: %d-run mean, ms; Memory: modeled peak bytes\n",seed,RUNS);
    printf("sizeof(Node)=%zu; QuickFrame=%zu; ArrayFrame=%zu; TreeFrame=%zu; BalanceFrame=%zu; MinFrame=%zu\n",sizeof(Node),sizeof(QuickFrame),sizeof(ArrayFrame),sizeof(TreeFrame),sizeof(BalanceFrame),sizeof(MinFrame));
    for(z=0;z<6;++z){
        int n=sizes[z],m=n/5,capacity=n+m,i,t,variant,ins=0,deleted[2]={0,0},final[2]={0,0};
        int *input=malloc((size_t)n*sizeof(int)),*a=malloc((size_t)capacity*sizeof(int)),*out=malloc((size_t)capacity*sizeof(int));
        Operation *ops=malloc((size_t)m*sizeof(Operation));Stats results[2]={{0}},sortstats[2]={{0}};double times[2]={0,0};
        if(!input||!a||!out||!ops){fputs("Allocation failed\n",stderr);return 1;}
        srand(seed+(unsigned)n);for(i=0;i<n;++i)input[i]=(int)random_below(1000001);
        for(i=0;i<m;++i){ops[i].insert=(random_below(1000001)%2==0);ops[i].value=(int)random_below(1000001);ins+=ops[i].insert;}
        /* 시간 측정과 별도로 매 연산 직후의 데이터 구성을 검증한다. */
        {Node *root=NULL;Stats as={0},ts={0};int length=n;
            memcpy(a,input,(size_t)n*sizeof(int));quick_sort(a,0,n-1,&as);
            for(i=0;i<n;++i)root=insert(root,input[i],&ts);
            if(!same(a,length,root,out,capacity))return 2;
            for(i=0;i<m;++i){int removed=0;array_operation(a,&length,ops[i],&as);if(ops[i].insert)root=insert(root,ops[i].value,&ts);else root=erase(root,ops[i].value,&ts,&removed);if(!same(a,length,root,out,capacity)){fputs("Per-operation check failed\n",stderr);return 2;}}
            destroy(root);
        }
        for(t=0;t<RUNS;++t)for(variant=0;variant<2;++variant){
            int method=(variant+t)%2,length=n,removed_total=0;Node *root=NULL;Stats s={0},initial;double start;
            if(!method){memcpy(a,input,(size_t)n*sizeof(int));s.storage=(size_t)capacity*sizeof(int);peak(&s);}
            start=now_ms();
            if(!method)quick_sort(a,0,n-1,&s);else for(i=0;i<n;++i)root=insert(root,input[i],&s);
            initial=s;
            for(i=0;i<m;++i){if(!method)removed_total+=array_operation(a,&length,ops[i],&s);else {int removed=0;if(ops[i].insert){root=insert(root,ops[i].value,&s);++length;}else{root=erase(root,ops[i].value,&s,&removed);length-=removed;removed_total+=removed;}}}
            times[method]+=now_ms()-start;
            if(!method){int j;for(j=1;j<length;++j)if(a[j-1]>a[j])return 2;}else {int used=0,ok=1;inorder(root,out,&used,capacity,&ok);if(!ok||used!=length)return 2;}
            if(t && (s.comparisons!=results[method].comparisons||s.moves!=results[method].moves||s.peak!=results[method].peak))return 2;
            results[method]=s;sortstats[method]=initial;final[method]=length;deleted[method]=removed_total;destroy(root);
        }
        if(final[0]!=final[1]||deleted[0]!=deleted[1])return 2;
        printf("\nData Size: %d\nOperations: %d (insert=%d, delete=%d, successful deletes=%d)\n",n,m,ins,m-ins,deleted[0]);
        puts("Metric                          Quick Array         AVL Tree");
        printf("Total Time (ms)                 %12.6f     %12.6f\n",times[0]/RUNS,times[1]/RUNS);
        printf("Comparisons                    %12" PRIu64 "     %12" PRIu64 "\n",results[0].comparisons,results[1].comparisons);
        printf("Data Movements                 %12" PRIu64 "     %12" PRIu64 "\n",results[0].moves,results[1].moves);
        printf("Memory estimate (Bytes)         %12zu     %12zu\n",results[0].peak,results[1].peak);
        printf("Final Data Count               %12d     %12d\n",final[0],final[1]);
        printf("Initial sort comparisons       %12" PRIu64 "     %12" PRIu64 "\n",sortstats[0].comparisons,sortstats[1].comparisons);
        printf("Initial sort movements         %12" PRIu64 "     %12" PRIu64 "\n",sortstats[0].moves,sortstats[1].moves);
        printf("Operations comparisons         %12" PRIu64 "     %12" PRIu64 "\n",results[0].comparisons-sortstats[0].comparisons,results[1].comparisons-sortstats[1].comparisons);
        printf("Operations movements           %12" PRIu64 "     %12" PRIu64 "\n",results[0].moves-sortstats[0].moves,results[1].moves-sortstats[1].moves);
        printf("Array capacity: %d; AVL peak nodes: %zu\nAll per-operation multiset and AVL balance checks: PASS\n",capacity,results[1].peak_nodes);
        free(input);free(a);free(out);free(ops);
    }
    return 0;
}

