#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <limits.h>

#define V 100
#define MAX_EDGES (V * (V - 1) / 2)

typedef struct Edge { int u, v; } Edge;
typedef struct Matrix { int cells[V][V]; int vertices, edges; } Matrix;
typedef struct Link { int vertex; struct Link *next; } Link;
typedef struct List { Link *heads[V]; int vertices, edges; } List;
typedef struct Stats { size_t memory; long insert, delete, check, adjacent; } Stats;

int random_below(int bound)
{
    unsigned long range = (unsigned long)RAND_MAX + 1UL;
    unsigned long limit = range - range % (unsigned)bound, value;
    do { value = (unsigned long)rand(); } while (value >= limit);
    return (int)(value % (unsigned)bound);
}
int matrix_has(const Matrix *g, int u, int v, long *count)
{
    (*count)++;
    return g->cells[u][v] != 0;
}
int list_has(const List *g, int u, int v, long *count)
{
    const Link *p;
    for (p = g->heads[u]; p != NULL; p = p->next) {
        (*count)++;
        if (p->vertex == v) return 1;
    }
    return 0;
}
int matrix_insert(Matrix *g, int u, int v, long *count)
{
    if (u == v || matrix_has(g,u,v,count)) return 0;
    g->cells[u][v] = g->cells[v][u] = 1;
    g->edges++;
    return 1;
}
int list_insert(List *g, int u, int v, long *count)
{
    Link *a, *b;
    if (u == v || list_has(g,u,v,count)) return 0;
    a = (Link *)malloc(sizeof(Link));
    b = (Link *)malloc(sizeof(Link));
    if (a == NULL || b == NULL) { free(a); free(b); return -1; }
    a->vertex = v; a->next = g->heads[u];
    b->vertex = u; b->next = g->heads[v];
    g->heads[u] = a; g->heads[v] = b;
    g->edges++;
    return 1;
}
int matrix_delete(Matrix *g, int u, int v, long *count)
{
    if (!matrix_has(g,u,v,count)) return 0;
    g->cells[u][v] = g->cells[v][u] = 0;
    g->edges--;
    return 1;
}
Link **find_link(List *g, int u, int v, long *count)
{
    Link **p = &g->heads[u];
    while (*p != NULL) {
        (*count)++;
        if ((*p)->vertex == v) return p;
        p = &(*p)->next;
    }
    return NULL;
}
int list_delete(List *g, int u, int v, long *count)
{
    Link **a = find_link(g,u,v,count);
    Link **b;
    Link *old;
    if (a == NULL) return 0;
    b = find_link(g,v,u,count);
    if (b == NULL) return 0;
    old = *a; *a = old->next; free(old);
    old = *b; *b = old->next; free(old);
    g->edges--;
    return 1;
}
void matrix_neighbors(const Matrix *g, int u, long *count)
{
    int v, found = 0;
    for (v = 0; v < V; v++) {
        (*count)++;
        if (g->cells[u][v]) { printf(" %d",v); found = 1; }
    }
    if (!found) printf(" (none)");
    putchar('\n');
}
void list_neighbors(const List *g, int u, long *count)
{
    const Link *p;
    if (g->heads[u] == NULL) printf(" (none)");
    for (p = g->heads[u]; p != NULL; p = p->next) {
        (*count)++;
        printf(" %d",p->vertex);
    }
    putchar('\n');
}
void destroy_list(List *g)
{
    int u;
    for (u = 0; u < V; u++) {
        Link *p = g->heads[u];
        while (p != NULL) { Link *next = p->next; free(p); p = next; }
        g->heads[u] = NULL;
    }
    g->edges = 0;
}
/* 검증은 각 리스트를 한 번 순회하여 두 표현의 모든 간선을 대조한다. */
int verify(const Matrix *m, const List *l, int expected)
{
    int u,v,total=0;
    if(m->edges != expected || l->edges != expected) return 0;
    for(u=0;u<V;u++) {
        int seen[V]={0};
        const Link *p;
        for(p=l->heads[u];p!=NULL;p=p->next) {
            if(p->vertex<0 || p->vertex>=V || p->vertex==u || seen[p->vertex]) return 0;
            seen[p->vertex]=1;
        }
        for(v=0;v<V;v++) {
            if(m->cells[u][v]!=seen[v] || m->cells[u][v]!=m->cells[v][u]) return 0;
            total+=seen[v];
        }
    }
    return total==2*expected;
}
void print_stats(int id,const char *name,const Stats *s)
{
    printf("\nCase %d: %s\n",id,name);
    printf("Memory                         : %zu Bytes\n",s->memory);
    printf("Edge insertion comparisons     : %ld\n",s->insert);
    printf("Edge deletion comparisons      : %ld\n",s->delete);
    printf("Connectivity check comparisons : %ld\n",s->check);
    printf("Adjacent vertices comparisons  : %ld\n",s->adjacent);
}
int experiment(int edges,int first_case)
{
    Matrix matrix = {{{0}},V,0};
    List list = {{NULL},V,0};
    Edge pool[MAX_EDGES], insert_edge, delete_edge, query;
    Stats ms={0}, ls={0};
    int i,u,v,n=0,vertex,mfound,lfound;
    for(u=0;u<V;u++) for(v=u+1;v<V;v++) pool[n++]=(Edge){u,v};
    /* 부분 Fisher-Yates 섞기로 중복 없는 간선 집합을 선택한다. */
    for(i=0;i<edges;i++) {
        int j=i+random_below(MAX_EDGES-i);
        Edge temp=pool[i]; pool[i]=pool[j]; pool[j]=temp;
    }
    for(i=0;i<edges;i++) {
        long ignored=0;
        matrix_insert(&matrix,pool[i].u,pool[i].v,&ignored);
        if(list_insert(&list,pool[i].u,pool[i].v,&ignored)!=1) goto failed;
    }
    if(!verify(&matrix,&list,edges)) goto failed;
    ms.memory=sizeof(Matrix);
    ls.memory=sizeof(List)+(size_t)(2*list.edges)*sizeof(Link);
    insert_edge=pool[edges+random_below(MAX_EDGES-edges)];
    delete_edge=pool[random_below(edges)];
    query.u=random_below(V);
    query.v=random_below(V-1);
    if(query.v>=query.u) query.v++;
    vertex=random_below(V);
    printf("\n=== %s Graph: vertices=%d, edges=%d ===\n",edges==100?"Sparse":"Dense",V,edges);
    printf("Memory measured before extra operations.\n");
    printf("Insert absent edge: (%d, %d)\n",insert_edge.u,insert_edge.v);
    if(matrix_insert(&matrix,insert_edge.u,insert_edge.v,&ms.insert)!=1 ||
       list_insert(&list,insert_edge.u,insert_edge.v,&ls.insert)!=1) goto failed;
    if(!verify(&matrix,&list,edges+1)) goto failed;
    printf("Delete original edge: (%d, %d)\n",delete_edge.u,delete_edge.v);
    if(matrix_delete(&matrix,delete_edge.u,delete_edge.v,&ms.delete)!=1 ||
       list_delete(&list,delete_edge.u,delete_edge.v,&ls.delete)!=1) goto failed;
    if(!verify(&matrix,&list,edges)) goto failed;
    mfound=matrix_has(&matrix,query.u,query.v,&ms.check);
    lfound=list_has(&list,query.u,query.v,&ls.check);
    if(mfound!=lfound) goto failed;
    printf("Connectivity (%d, %d): matrix=%s, list=%s\n",query.u,query.v,
           mfound?"Connected":"Not connected",lfound?"Connected":"Not connected");
    printf("Matrix neighbors of %d:",vertex); matrix_neighbors(&matrix,vertex,&ms.adjacent);
    printf("List neighbors of %d:",vertex); list_neighbors(&list,vertex,&ls.adjacent);
    print_stats(first_case,edges==100?"Sparse Graph - Adjacency Matrix":"Dense Graph - Adjacency Matrix",&ms);
    print_stats(first_case+1,edges==100?"Sparse Graph - Adjacency List":"Dense Graph - Adjacency List",&ls);
    puts("Graph equality, edge counts and symmetry: PASS");
    destroy_list(&list);
    return 1;
failed:
    fputs("Error: allocation or graph verification failed.\n",stderr);
    destroy_list(&list);
    return 0;
}
int main(int argc,char **argv)
{
    unsigned seed=(unsigned)time(NULL);
    if(argc>2) { fprintf(stderr,"Usage: %s [seed]\n",argv[0]); return 1; }
    if(argc==2) {
        char *end; unsigned long value;
        errno=0; value=strtoul(argv[1],&end,10);
        if(argv[1][0]=='-' || end==argv[1] || *end || errno==ERANGE || value>UINT_MAX) {
            fputs("Error: invalid seed.\n",stderr); return 1;
        }
        seed=(unsigned)value;
    }
    srand(seed);
    printf("Seed: %u\nsizeof(int)=%zu, sizeof(pointer)=%zu, sizeof(Link)=%zu\n",seed,sizeof(int),sizeof(Link*),sizeof(Link));
    return experiment(100,1) && experiment(4000,3) ? 0 : 1;
}
