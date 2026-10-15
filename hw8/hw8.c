#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <limits.h>
#include <errno.h>
#ifdef _WIN32
#include <windows.h>
#endif
#define V 50
#define MAX_E (V*(V-1)/2)
#define RUNS 1000

typedef struct Edge { int u,v,w; } Edge;
typedef struct Link { int vertex,weight; struct Link *next; } Link;
typedef struct List { Link *heads[V]; int edges; } List;
typedef struct Matrix { int weight[V][V]; int edges; } Matrix;
typedef struct MST { Edge edges[V-1]; int count,cost; } MST;
typedef struct UF { int parent[V],rank[V]; } UF;
typedef struct PrimWork { int used[V],key[V],parent[V]; } PrimWork;
typedef struct Report { MST mst; size_t graph_memory,aux_memory; double milliseconds; } Report;

int random_below(int bound)
{
    unsigned long range=(unsigned long)RAND_MAX+1UL, limit=range-range%(unsigned)bound, r;
    do {r=(unsigned long)rand();} while(r>=limit);
    return (int)(r%(unsigned)bound);
}
void generate(Matrix *m,int count)
{
    int order[V],i,j,n=0; Edge pool[MAX_E];
    *m=(Matrix){{{0}},0};
    for(i=0;i<V;i++) order[i]=i;
    for(i=V-1;i>0;i--) {int t; j=random_below(i+1);t=order[i];order[i]=order[j];order[j]=t;}
    /* 무작위 순서의 각 정점을 이전 정점 하나에 연결하여 연결성을 보장한다. */
    for(i=1;i<V;i++) {
        int u=order[i],v=order[random_below(i)],w=1+random_below(1000);
        m->weight[u][v]=m->weight[v][u]=w; m->edges++;
    }
    for(i=0;i<V;i++) for(j=i+1;j<V;j++)
        if(!m->weight[i][j]) pool[n++]=(Edge){i,j,0};
    for(i=0;m->edges<count;i++) {
        Edge t; int w=1+random_below(1000);
        j=i+random_below(n-i);t=pool[i];pool[i]=pool[j];pool[j]=t;
        m->weight[pool[i].u][pool[i].v]=m->weight[pool[i].v][pool[i].u]=w;m->edges++;
    }
}
int connected(const Matrix *m)
{
    int seen[V]={0},queue[V],front=0,back=0;
    queue[back++]=0;seen[0]=1;
    while(front<back) {int u=queue[front++],v;for(v=0;v<V;v++)
        if(m->weight[u][v]&&!seen[v]) {seen[v]=1;queue[back++]=v;}}
    return back==V;
}
void free_list(List *g)
{
    int u;for(u=0;u<V;u++) {Link *p=g->heads[u];while(p){Link *next=p->next;free(p);p=next;}g->heads[u]=NULL;}
}
int make_list(const Matrix *m,List *g)
{
    int u,v;*g=(List){{NULL},m->edges};
    for(u=0;u<V;u++) for(v=0;v<V;v++) if(m->weight[u][v]) {
        Link *p=(Link*)malloc(sizeof(Link));
        if(!p){free_list(g);return 0;}
        p->vertex=v;p->weight=m->weight[u][v];p->next=g->heads[u];g->heads[u]=p;
    }
    return 1;
}
int root(UF *uf,int x)
{
    while(uf->parent[x]!=x){uf->parent[x]=uf->parent[uf->parent[x]];x=uf->parent[x];}
    return x;
}
int join(UF *uf,int a,int b)
{
    a=root(uf,a);b=root(uf,b);if(a==b)return 0;
    if(uf->rank[a]<uf->rank[b]) uf->parent[a]=b;
    else {uf->parent[b]=a;if(uf->rank[a]==uf->rank[b])uf->rank[a]++;}
    return 1;
}
/* 제자리 힙 정렬: 추가 정렬 배열과 재귀 호출을 사용하지 않는다. */
void sift(Edge *edges,int n,int i)
{
    for(;;){int child=2*i+1;Edge t;
        if(child>=n)return;
        if(child+1<n&&edges[child+1].w>edges[child].w)child++;
        if(edges[i].w>=edges[child].w)return;
        t=edges[i];edges[i]=edges[child];edges[child]=t;i=child;
    }
}
void sort_edges(Edge *edges,int n)
{
    int i;for(i=n/2-1;i>=0;i--)sift(edges,n,i);
    for(i=n-1;i>0;i--){Edge t=edges[0];edges[0]=edges[i];edges[i]=t;sift(edges,i,0);}
}
int kruskal(const List *g,MST *mst)
{
    Edge *edges=(Edge*)malloc((size_t)g->edges*sizeof(Edge));
    UF uf;int u,n=0,i;
    *mst=(MST){{{0,0,0}},0,0};
    if(!edges)return 0;
    for(u=0;u<V;u++) {
        const Link *p;uf.parent[u]=u;uf.rank[u]=0;
        for(p=g->heads[u];p;p=p->next) if(u<p->vertex)edges[n++]=(Edge){u,p->vertex,p->weight};
    }
    sort_edges(edges,n);
    for(i=0;i<n&&mst->count<V-1;i++)if(join(&uf,edges[i].u,edges[i].v)) {
        mst->edges[mst->count++]=edges[i];mst->cost+=edges[i].w;
    }
    free(edges);return mst->count==V-1;
}
int prim(const Matrix *g,MST *mst)
{
    PrimWork work={{0},{0},{0}};int i,j;
    *mst=(MST){{{0,0,0}},0,0};
    for(i=0;i<V;i++){work.key[i]=INT_MAX;work.parent[i]=-1;}
    work.key[0]=0;
    for(i=0;i<V;i++) {
        int u=-1;
        for(j=0;j<V;j++)if(!work.used[j]&&(u<0||work.key[j]<work.key[u]))u=j;
        if(u<0||work.key[u]==INT_MAX)return 0;
        work.used[u]=1;
        if(work.parent[u]>=0){mst->edges[mst->count++]=(Edge){work.parent[u],u,work.key[u]};mst->cost+=work.key[u];}
        for(j=0;j<V;j++)if(g->weight[u][j]&&!work.used[j]&&g->weight[u][j]<work.key[j]){
            work.key[j]=g->weight[u][j];work.parent[j]=u;
        }
    }
    return mst->count==V-1;
}
/* 신장 트리 조건과 모든 간선에 대한 사이클 최적성 조건을 확인한다. */
int validate(const Matrix *g,const MST *mst)
{
    int tree[V][V]={{0}},sum=0,i,u,v;UF uf;
    if(mst->count!=V-1)return 0;
    for(i=0;i<V;i++){uf.parent[i]=i;uf.rank[i]=0;}
    for(i=0;i<mst->count;i++){
        Edge e=mst->edges[i];
        if(e.u<0||e.u>=V||e.v<0||e.v>=V||e.w<=0||g->weight[e.u][e.v]!=e.w||!join(&uf,e.u,e.v))return 0;
        tree[e.u][e.v]=tree[e.v][e.u]=e.w;sum+=e.w;
    }
    if(sum!=mst->cost)return 0;
    for(u=0;u<V;u++){
        int seen[V]={0},max_weight[V]={0},queue[V],front=0,back=0;
        seen[u]=1;queue[back++]=u;
        while(front<back){int x=queue[front++];for(v=0;v<V;v++)if(tree[x][v]&&!seen[v]){
            seen[v]=1;max_weight[v]=max_weight[x]>tree[x][v]?max_weight[x]:tree[x][v];queue[back++]=v;
        }}
        if(back!=V)return 0;
        for(v=0;v<V;v++)if(g->weight[u][v]&&g->weight[u][v]<max_weight[v])return 0;
    }
    return 1;
}
double now_seconds(void)
{
#ifdef _WIN32
    LARGE_INTEGER counter,frequency;
    QueryPerformanceFrequency(&frequency);QueryPerformanceCounter(&counter);
    return (double)counter.QuadPart/(double)frequency.QuadPart;
#else
    return (double)clock()/CLOCKS_PER_SEC;
#endif
}
int benchmark(const Matrix *m,const List *l,int use_kruskal,Report *r)
{
    int i;double start=now_seconds();
    for(i=0;i<RUNS;i++)if(!(use_kruskal?kruskal(l,&r->mst):prim(m,&r->mst)))return 0;
    r->milliseconds=(now_seconds()-start)*1000.0/RUNS;
    r->graph_memory=use_kruskal?sizeof(List)+(size_t)2*l->edges*sizeof(Link):sizeof(Matrix);
    r->aux_memory=sizeof(MST)+(use_kruskal?(size_t)l->edges*sizeof(Edge)+sizeof(UF):sizeof(PrimWork));
    return validate(m,&r->mst);
}
void print_report(int id,const char *name,int edges,const Report *r)
{
    int i;printf("\nCase %d: %s\nVertices: %d\nEdges: %d\n",id,name,V,edges);
    printf("MST Edges: %d\nMST Cost: %d\n",r->mst.count,r->mst.cost);
    printf("Graph Memory: %zu Bytes\nAuxiliary Memory (including MST): %zu Bytes\nMemory: %zu Bytes\n",
        r->graph_memory,r->aux_memory,r->graph_memory+r->aux_memory);
    printf("Execution Time: %.6f ms (mean of %d complete runs)\n",r->milliseconds,RUNS);
    puts("MST edge list (u, v, weight):");
    for(i=0;i<r->mst.count;i++)printf("(%d, %d, %d)\n",r->mst.edges[i].u,r->mst.edges[i].v,r->mst.edges[i].w);
}
int main(int argc,char **argv)
{
    unsigned seed=(unsigned)time(NULL);Matrix sparse,dense;List sl,dl;Report reports[4];
    if(argc>2){fprintf(stderr,"Usage: %s [seed]\n",argv[0]);return 1;}
    if(argc==2){char *end;unsigned long n;errno=0;n=strtoul(argv[1],&end,10);
        if(argv[1][0]=='-'||end==argv[1]||*end||errno==ERANGE||n>UINT_MAX){fputs("Invalid seed\n",stderr);return 1;}seed=(unsigned)n;}
    srand(seed);generate(&sparse,100);generate(&dense,600);
    if(!connected(&sparse)||!connected(&dense)){fputs("Disconnected graph\n",stderr);return 1;}
    if(!make_list(&sparse,&sl))return 1;
    if(!make_list(&dense,&dl)){free_list(&sl);return 1;}
    if(!benchmark(&sparse,&sl,1,&reports[0])||!benchmark(&dense,&dl,1,&reports[1])||
       !benchmark(&sparse,&sl,0,&reports[2])||!benchmark(&dense,&dl,0,&reports[3])||
       reports[0].mst.cost!=reports[2].mst.cost||reports[1].mst.cost!=reports[3].mst.cost){
        fputs("MST verification failed\n",stderr);free_list(&sl);free_list(&dl);return 1;}
    printf("Seed: %u\nWeights: 1..1000\nConnected graphs: PASS\n",seed);
#ifdef _WIN32
    puts("Timer: QueryPerformanceCounter, milliseconds per complete MST run");
#else
    puts("Timer: clock(), CPU milliseconds per complete MST run");
#endif
    print_report(1,"Sparse - Adjacency List - Kruskal",100,&reports[0]);
    print_report(2,"Dense - Adjacency List - Kruskal",600,&reports[1]);
    print_report(3,"Sparse - Adjacency Matrix - Prim",100,&reports[2]);
    print_report(4,"Dense - Adjacency Matrix - Prim",600,&reports[3]);
    puts("\n49 edges, valid spanning trees, cycle optimality and matching costs: PASS");
    free_list(&sl);free_list(&dl);return 0;
}
