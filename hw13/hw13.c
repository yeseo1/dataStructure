#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <time.h>
#include <limits.h>
#include <errno.h>
#ifdef _WIN32
#include <windows.h>
#endif

#define MAX_N 20000
#define REPEATS 100
typedef struct { uint64_t partition, pivot; } Counts;
typedef struct { int lo, hi; } Range;

/* 데이터 생성과 무작위 피벗 선택용 난수. */
static unsigned random_below(unsigned bound)
{
    const uint32_t range = UINT32_C(1) << 30;
    uint32_t v;
    do {
        v = ((uint32_t)(rand() & 32767) << 15) | (uint32_t)(rand() & 32767);
    } while (v >= range - range % bound);
    return v % bound;
}
static void swap(int *x, int *y) { int t = *x; *x = *y; *y = t; }
static int less(int x, int y, Counts *c) { ++c->pivot; return x < y; }

static int choose_pivot(const int *a, int lo, int hi, int method, Counts *c)
{
    int mid = lo + (hi - lo) / 2;
    if (method == 0) return lo;
    if (method == 1) return lo + (int)random_below((unsigned)(hi - lo + 1));
    /* 2~3회의 값 비교로 중앙값 위치를 결정한다. */
    if (less(a[lo], a[mid], c)) {
        if (less(a[mid], a[hi], c)) return mid;
        return less(a[lo], a[hi], c) ? hi : lo;
    }
    if (less(a[lo], a[hi], c)) return lo;
    return less(a[mid], a[hi], c) ? hi : mid;
}

/* 세 피벗 방식과 재귀/반복 구현이 공유하는 Lomuto 분할. */
static int partition(int *a, int lo, int hi, int method, Counts *c)
{
    int p = choose_pivot(a, lo, hi, method, c), pivot, i = lo, j;
    swap(&a[p], &a[hi]);
    pivot = a[hi];
    for (j = lo; j < hi; ++j) {
        ++c->partition;
        if (a[j] < pivot) { swap(&a[i], &a[j]); ++i; }
    }
    swap(&a[i], &a[hi]);
    return i;
}
/* 재귀 깊이 계측은 시간 실험과 별도로 수행한다. */
static int current_depth, maximum_depth;
static int measure_depth;
void quick_recursive(int *a, int lo, int hi, int method, Counts *c)
{
    if (measure_depth) {
        ++current_depth;
        if (current_depth > maximum_depth) maximum_depth = current_depth;
    }
    if (lo < hi) {
        int p = partition(a, lo, hi, method, c);
        quick_recursive(a, lo, p - 1, method, c);
        quick_recursive(a, p + 1, hi, method, c);
    }
    if (measure_depth) --current_depth;
}
void quick_iterative(int *a, int n, int method, Counts *c, Range *stack)
{
    int top = 0;
    if (n > 1) stack[top++] = (Range){0, n - 1};
    while (top) {
        Range r = stack[--top];
        int p = partition(a, r.lo, r.hi, method, c);
        /* 오른쪽을 먼저 넣어 재귀와 동일하게 왼쪽부터 처리한다. */
        if (p + 1 < r.hi) stack[top++] = (Range){p + 1, r.hi};
        if (r.lo < p - 1) stack[top++] = (Range){r.lo, p - 1};
    }
}
static int compare_int(const void *x, const void *y)
{
    int a = *(const int *)x, b = *(const int *)y;
    return (a > b) - (a < b);
}
static int correct(const int *a, const int *reference, int n)
{
    int i;
    for (i = 0; i < n; ++i) if (a[i] != reference[i]) return 0;
    return 1;
}
#ifdef _WIN32
static double frequency;
static void init_timer(void)
{
    LARGE_INTEGER f;
    if (!QueryPerformanceFrequency(&f)) { fputs("Timer failed\n", stderr); exit(1); }
    frequency = (double)f.QuadPart;
}
static double milliseconds(void)
{
    LARGE_INTEGER t; QueryPerformanceCounter(&t);
    return (double)t.QuadPart * 1000.0 / frequency;
}
#else
static void init_timer(void) { }
static double milliseconds(void) { return (double)clock() * 1000.0 / CLOCKS_PER_SEC; }
#endif
/* 매개변수와 지역 변수의 저장량을 나타내는 계산 모델(ABI 스택 실측 아님). */
typedef struct { int *a; Counts *c; int lo, hi, method, p; } QuickFrame;
typedef struct { int *a, *temp; Counts *c; int lo, hi, mid, i, j, k; } MergeFrame;
typedef struct { int *a; Counts *c; int lo, hi, method, p, pivot, i, j; } PartitionFrame;
typedef struct { const int *a; Counts *c; int lo, hi, method, mid; } PivotFrame;
/* less, swap, random_below 중 가장 큰 명시적 변수 저장량: random_below */
typedef struct { unsigned bound; uint32_t range, v; } RandomFrame;
typedef struct { int x, y; Counts *c; } LessFrame;
typedef struct { int *x, *y; int t; } SwapFrame;

static size_t larger(size_t a, size_t b) { return a > b ? a : b; }
static size_t quick_memory(int depth)
{
    size_t leaf = larger(sizeof(RandomFrame), sizeof(LessFrame));
    size_t helpers = sizeof(PartitionFrame) + larger(sizeof(PivotFrame) + leaf, sizeof(SwapFrame));
    /* 깊이는 기저 호출까지 포함. 보조 호출 공간은 보수적으로 더한다. */
    return (size_t)depth * sizeof(QuickFrame) + helpers;
}
static void merge_sort(int *a, int *temp, int lo, int hi, Counts *c)
{
    int mid, i, j, k;
    if (measure_depth) {
        ++current_depth;
        if (current_depth > maximum_depth) maximum_depth = current_depth;
    }
    if (lo < hi) {
        mid = lo + (hi - lo) / 2;
        merge_sort(a, temp, lo, mid, c);
        merge_sort(a, temp, mid + 1, hi, c);
        i = lo; j = mid + 1; k = lo;
        while (i <= mid && j <= hi) {
            ++c->partition;
            if (a[i] <= a[j]) temp[k++] = a[i++];
            else temp[k++] = a[j++];
        }
        while (i <= mid) temp[k++] = a[i++];
        while (j <= hi) temp[k++] = a[j++];
        for (k = lo; k <= hi; ++k) a[k] = temp[k];
    }
    if (measure_depth) --current_depth;
}

int main(int argc, char **argv)
{
    const int sizes[] = {1000, 2000, 4000, 8000, 16000, 20000};
    const char *names[] = {"Merge Sort", "Quick (First)", "Quick (Random)", "Quick (Median3)"};
    unsigned seed = 20261104U;
    int *original = NULL, *work = NULL, *reference = NULL, *temp = NULL;
    int k, i, alg;
    if (argc > 2) { fprintf(stderr, "Usage: %s [seed]\n", argv[0]); return 1; }
    if (argc == 2) {
        char *end; unsigned long v; errno = 0; v = strtoul(argv[1], &end, 10);
        if (argv[1][0] == '-' || end == argv[1] || *end || errno == ERANGE || v > UINT_MAX) {
            fputs("Invalid seed\n", stderr); return 1;
        }
        seed = (unsigned)v;
    }
    original = malloc(MAX_N * sizeof(int)); work = malloc(MAX_N * sizeof(int));
    reference = malloc(MAX_N * sizeof(int));
    if (!original || !work || !reference) goto failure;
    init_timer();
    printf("Seed: %u (default reproduces assignment 12 input)\n", seed);
    puts("Values: 0..1000000; duplicates allowed; one input per size");
    printf("Time: average of %d runs, ms; memory: calculated auxiliary payload estimate\n", REPEATS);
    printf("sizeof(int)=%zu; MergeFrame=%zu; QuickFrame=%zu; Quick helper allowance=%zu\n",
           sizeof(int), sizeof(MergeFrame), sizeof(QuickFrame), quick_memory(0));
    for (k = 0; k < 6; ++k) {
        int n = sizes[k], depths[4], trial, position;
        Counts baseline[4]; size_t memories[4]; double times[4] = {0,0,0,0};
        temp = malloc((size_t)n * sizeof(int));
        if (!temp) goto failure;
        srand(seed + (unsigned)n);
        for (i = 0; i < n; ++i) original[i] = (int)random_below(1000001);
        memcpy(reference, original, (size_t)n * sizeof(int));
        qsort(reference, (size_t)n, sizeof(int), compare_int);
        for (alg = 0; alg < 4; ++alg) {
            Counts c = {0,0};
            memcpy(work, original, (size_t)n * sizeof(int));
            srand(seed ^ (unsigned)n ^ 0x9e3779b9U);
            current_depth = maximum_depth = 0; measure_depth = 1;
            if (alg == 0) merge_sort(work, temp, 0, n - 1, &c);
            else quick_recursive(work, 0, n - 1, alg - 1, &c);
            measure_depth = 0;
            if (!correct(work, reference, n)) goto failure;
            depths[alg] = maximum_depth; baseline[alg] = c;
            memories[alg] = alg == 0 ? (size_t)n * sizeof(int) + (size_t)maximum_depth * sizeof(MergeFrame) : quick_memory(maximum_depth);
        }
        for (trial = -1; trial < REPEATS; ++trial) {
            for (position = 0; position < 4; ++position) {
                Counts c = {0,0}; double start, elapsed;
                alg = (position + (trial < 0 ? 0 : trial)) % 4;
                memcpy(work, original, (size_t)n * sizeof(int));
                srand(seed ^ (unsigned)n ^ 0x9e3779b9U);
                start = milliseconds();
                if (alg == 0) merge_sort(work, temp, 0, n - 1, &c);
                else quick_recursive(work, 0, n - 1, alg - 1, &c);
                elapsed = milliseconds() - start;
                if (!correct(work, reference, n) || c.partition != baseline[alg].partition || c.pivot != baseline[alg].pivot) goto failure;
                if (trial >= 0) times[alg] += elapsed;
            }
        }
        printf("\nData Size : %d\nAlgorithm          Comparisons    Time(ms)  Memory estimate(Bytes)  Depth\n", n);
        for (alg = 0; alg < 4; ++alg)
            printf("%-17s %12" PRIu64 " %11.6f %15zu %9d  PASS\n", names[alg], baseline[alg].partition + baseline[alg].pivot, times[alg]/REPEATS, memories[alg], depths[alg]);
        free(temp); temp = NULL;
    }
    puts("\nAll sorting, value preservation and repeated comparison checks: PASS");
    free(original); free(work); free(reference); return 0;
failure:
    fputs("Allocation or validation failed\n", stderr);
    free(original); free(work); free(reference); free(temp); return 1;
}

