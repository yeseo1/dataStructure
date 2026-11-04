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
void quick_recursive(int *a, int lo, int hi, int method, Counts *c)
{
    if (lo < hi) {
        int p = partition(a, lo, hi, method, c);
        quick_recursive(a, lo, p - 1, method, c);
        quick_recursive(a, p + 1, hi, method, c);
    }
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

int main(int argc, char **argv)
{
    const int sizes[] = {1000, 2000, 4000, 8000, 16000, 20000};
    const char *names[] = {"First", "Random", "Median-3"};
    unsigned seed = (unsigned)time(NULL);
    int *original, *work, *reference, k, i, method;
    Range *stack;
    if (argc > 2) { fprintf(stderr, "Usage: %s [seed]\n", argv[0]); return 1; }
    if (argc == 2) {
        char *end; unsigned long v;
        errno = 0; v = strtoul(argv[1], &end, 10);
        if (argv[1][0] == '-' || end == argv[1] || *end || errno == ERANGE || v > UINT_MAX) {
            fputs("Invalid seed\n", stderr); return 1;
        }
        seed = (unsigned)v;
    }
    original = malloc(MAX_N * sizeof(int)); work = malloc(MAX_N * sizeof(int));
    reference = malloc(MAX_N * sizeof(int)); stack = malloc(MAX_N * sizeof(Range));
    if (!original || !work || !reference || !stack) {
        fputs("Allocation failed\n", stderr);
        free(original); free(work); free(reference); free(stack); return 1;
    }
    init_timer();
    printf("Seed: %u\nRandom values: 0..1000000; duplicates allowed\n", seed);
#ifdef _WIN32
    puts("Timer: QueryPerformanceCounter; unit: ms");
#else
    puts("Timer: clock (CPU time); unit: ms");
#endif
    for (k = 0; k < 6; ++k) {
        int n = sizes[k], trial, variant;
        double times[2] = {0, 0};
        Counts median = {0, 0};
        /* 크기별 시드로 데이터 난수를 피벗 난수 소비량과 분리한다. */
        srand(seed + (unsigned)n);
        for (i = 0; i < n; ++i) original[i] = (int)random_below(1000001);
        memcpy(reference, original, (size_t)n * sizeof(int));
        qsort(reference, (size_t)n, sizeof(int), compare_int);
        printf("\nData Size : %d\n", n);
        for (method = 0; method < 3; ++method) {
            Counts c = {0, 0};
            memcpy(work, original, (size_t)n * sizeof(int));
            if (method == 1) srand(seed ^ (unsigned)n ^ 0x9e3779b9U);
            quick_recursive(work, 0, n - 1, method, &c);
            if (!correct(work, reference, n)) goto failure;
            printf("Quick Sort (%s): %" PRIu64 " comparisons (partition=%" PRIu64 ", pivot=%" PRIu64 "); Correct=PASS\n",
                   names[method], c.partition + c.pivot, c.partition, c.pivot);
            if (method == 2) median = c;
        }
        /* 예열은 측정 제외. 복사, 검증, 출력, 스택 할당도 타이머 밖이다. */
        for (trial = -1; trial < REPEATS; ++trial) {
            for (variant = 0; variant < 2; ++variant) {
                int iterative = (trial >= 0 && trial % 2) ? 1 - variant : variant;
                Counts c = {0, 0}; double start, elapsed;
                memcpy(work, original, (size_t)n * sizeof(int));
                start = milliseconds();
                if (iterative) quick_iterative(work, n, 2, &c, stack);
                else quick_recursive(work, 0, n - 1, 2, &c);
                elapsed = milliseconds() - start;
                if (!correct(work, reference, n) || c.partition != median.partition || c.pivot != median.pivot) goto failure;
                if (trial >= 0) times[iterative] += elapsed;
            }
        }
        printf("Bonus: Median-of-Three, %d repetitions, alternating execution order\n", REPEATS);
        printf("Recursive Quick Sort: %.6f ms average\nIterative Quick Sort: %.6f ms average\n", times[0] / REPEATS, times[1] / REPEATS);
        puts("Bonus sorting and identical comparison counts: PASS");
    }
    puts("\nAll experiments: PASS");
    free(original); free(work); free(reference); free(stack); return 0;
failure:
    fputs("Validation failed\n", stderr);
    free(original); free(work); free(reference); free(stack); return 1;
}

