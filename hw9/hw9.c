#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <limits.h>
#include <errno.h>

#define V 10
#define E 20
#define INF 1000000
#define MAX_EDGES (V * (V - 1) / 2)

typedef struct Graph { int weight[V][V]; } Graph;
typedef struct Pair { int u, v; } Pair;
typedef struct Paths { int distance[V][V]; int next[V][V]; } Paths;

int random_below(int bound)
{
    unsigned long range = (unsigned long)RAND_MAX + 1UL;
    unsigned long limit = range - range % (unsigned)bound, value;
    do { value = (unsigned long)rand(); } while (value >= limit);
    return (int)(value % (unsigned)bound);
}
void generate_graph(Graph *g)
{
    int order[V], i, j, count = 0, candidates = 0;
    Pair pool[MAX_EDGES];
    *g = (Graph){{{0}}};
    for (i = 0; i < V; i++) order[i] = i;
    for (i = V - 1; i > 0; i--) {
        int temp;
        j = random_below(i + 1);
        temp = order[i]; order[i] = order[j]; order[j] = temp;
    }
    /* 9개 간선으로 모든 정점을 먼저 연결한다. */
    for (i = 1; i < V; i++) {
        int u = order[i], v = order[random_below(i)], w = 1 + random_below(100);
        g->weight[u][v] = g->weight[v][u] = w;
        count++;
    }
    for (i = 0; i < V; i++) for (j = i + 1; j < V; j++)
        if (!g->weight[i][j]) pool[candidates++] = (Pair){i, j};
    for (i = 0; count < E; i++) {
        Pair temp;
        int w = 1 + random_below(100);
        j = i + random_below(candidates - i);
        temp = pool[i]; pool[i] = pool[j]; pool[j] = temp;
        g->weight[pool[i].u][pool[i].v] = g->weight[pool[i].v][pool[i].u] = w;
        count++;
    }
}
int valid_graph(const Graph *g)
{
    int seen[V] = {0}, queue[V], front = 0, back = 0, edges = 0, u, v;
    for (u = 0; u < V; u++) {
        if (g->weight[u][u]) return 0;
        for (v = 0; v < V; v++) {
            int w = g->weight[u][v];
            if (w < 0 || w > 100 || w != g->weight[v][u]) return 0;
            if (u < v && w) edges++;
        }
    }
    if (edges != E) return 0;
    seen[0] = 1; queue[back++] = 0;
    while (front < back) {
        u = queue[front++];
        for (v = 0; v < V; v++) if (g->weight[u][v] && !seen[v]) {
            seen[v] = 1; queue[back++] = v;
        }
    }
    return back == V;
}
void floyd_warshall(const Graph *g, Paths *paths)
{
    int i, j, k;
    for (i = 0; i < V; i++) for (j = 0; j < V; j++) {
        paths->distance[i][j] = i == j ? 0 : (g->weight[i][j] ? g->weight[i][j] : INF);
        paths->next[i][j] = i == j || g->weight[i][j] ? j : -1;
    }
    for (k = 0; k < V; k++) for (i = 0; i < V; i++) for (j = 0; j < V; j++) {
        if (paths->distance[i][k] != INF && paths->distance[k][j] != INF &&
            paths->distance[i][k] + paths->distance[k][j] < paths->distance[i][j]) {
            paths->distance[i][j] = paths->distance[i][k] + paths->distance[k][j];
            paths->next[i][j] = paths->next[i][k];
        }
    }
}
int restore_path(const Paths *paths, int u, int v, int route[V])
{
    int length = 0;
    if (paths->next[u][v] < 0) return 0;
    route[length++] = u;
    while (u != v) {
        if (length >= V) return 0;
        u = paths->next[u][v];
        if (u < 0 || u >= V) return 0;
        route[length++] = u;
    }
    return length;
}
/* 별도 알고리즘으로 최단거리의 정확성을 대조한다. */
void dijkstra(const Graph *g, int start, int distance[V])
{
    int used[V] = {0}, i, j;
    for (i = 0; i < V; i++) distance[i] = INF;
    distance[start] = 0;
    for (i = 0; i < V; i++) {
        int u = -1;
        for (j = 0; j < V; j++) if (!used[j] && (u < 0 || distance[j] < distance[u])) u = j;
        if (u < 0 || distance[u] == INF) break;
        used[u] = 1;
        for (j = 0; j < V; j++) if (g->weight[u][j] && !used[j] &&
            distance[u] + g->weight[u][j] < distance[j]) distance[j] = distance[u] + g->weight[u][j];
    }
}
int validate_paths(const Graph *g, const Paths *paths)
{
    int u, v;
    for (u = 0; u < V; u++) {
        int reference[V];
        dijkstra(g, u, reference);
        for (v = 0; v < V; v++) {
            int route[V], seen[V] = {0}, length, sum = 0, i;
            if (paths->distance[u][v] != reference[v]) return 0;
            length = restore_path(paths, u, v, route);
            if (!length || route[0] != u || route[length - 1] != v) return 0;
            for (i = 0; i < length; i++) {
                if (seen[route[i]]) return 0;
                seen[route[i]] = 1;
                if (i > 0) {
                    int w = g->weight[route[i - 1]][route[i]];
                    if (!w) return 0;
                    sum += w;
                }
            }
            if (sum != reference[v]) return 0;
        }
    }
    return 1;
}
int main(int argc, char **argv)
{
    unsigned seed = (unsigned)time(NULL);
    Graph graph; Paths paths;
    int u, v, pairs = 0;
    if (argc > 2) { fprintf(stderr, "Usage: %s [seed]\n", argv[0]); return 1; }
    if (argc == 2) {
        unsigned long value; char *end;
        errno = 0; value = strtoul(argv[1], &end, 10);
        if (argv[1][0] == '-' || end == argv[1] || *end || errno == ERANGE || value > UINT_MAX) {
            fputs("Error: invalid seed.\n", stderr); return 1;
        }
        seed = (unsigned)value;
    }
    srand(seed); generate_graph(&graph);
    if (!valid_graph(&graph)) { fputs("Error: invalid graph.\n", stderr); return 1; }
    printf("Seed: %u\nGraph\nVertices: %d\nEdges: %d\nConnected: Yes\n\nEdges\n", seed, V, E);
    for (u = 0; u < V; u++) for (v = u + 1; v < V; v++) if (graph.weight[u][v])
        printf("(%d, %d) Weight: %d\n", u, v, graph.weight[u][v]);
    floyd_warshall(&graph, &paths);
    if (!validate_paths(&graph, &paths)) { fputs("Error: shortest-path verification failed.\n", stderr); return 1; }
    puts("\nShortest Paths");
    for (u = 0; u < V; u++) for (v = u + 1; v < V; v++) {
        int route[V], length = restore_path(&paths, u, v, route), i;
        printf("\n%d -> %d\nPath     : ", u, v);
        for (i = 0; i < length; i++) printf("%s%d", i ? " -> " : "", route[i]);
        printf("\nDistance : %d\n", paths.distance[u][v]);
        pairs++;
    }
    printf("\nTotal vertex pairs: %d\n", pairs);
    puts("Verification: graph conditions, path weight sums and Dijkstra cross-check PASS");
    return 0;
}
