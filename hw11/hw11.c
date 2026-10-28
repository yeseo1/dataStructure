#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <limits.h>

/* 실제 데이터 비교만 센다. 마지막으로 거짓이 된 비교도 포함한다. */
static uint64_t gap_insertion(int *a, int n, int gap)
{
    uint64_t comparisons = 0;
    int i;
    for (i = gap; i < n; ++i) {
        int value = a[i], j = i;
        while (j >= gap) {
            ++comparisons;
            if (a[j - gap] <= value) break;
            a[j] = a[j - gap];
            j -= gap;
        }
        a[j] = value;
    }
    return comparisons;
}

uint64_t insertion_sort(int *a, int n)
{
    return gap_insertion(a, n, 1);
}

/* gaps에는 실제로 사용하는 순서(내림차순)를 기록한다. */
int make_gaps(int n, int knuth, int *gaps)
{
    int h, count = 0;
    if (n < 2) return 0;
    if (knuth) {
        h = 1;
        /* 3*h+1 < n과 동치이며 정수 오버플로를 피한다. */
        while (h <= (n - 2) / 3) h = 3 * h + 1;
        for (; h >= 1; h = (h - 1) / 3) gaps[count++] = h;
    } else {
        for (h = n / 2; h >= 1; h /= 2) gaps[count++] = h;
    }
    return count;
}

uint64_t shell_sort(int *a, int n, int knuth)
{
    int gaps[sizeof(int) * CHAR_BIT], count, i;
    uint64_t comparisons = 0;
    count = make_gaps(n, knuth, gaps);
    for (i = 0; i < count; ++i)
        comparisons += gap_insertion(a, n, gaps[i]);
    return comparisons;
}

/* Windows의 작은 RAND_MAX에서도 0~1,000,000 전체 범위를 사용한다. */
static int random_value(void)
{
    const uint32_t range = UINT32_C(1) << 30;
    const uint32_t bound = 1000001;
    uint32_t value;
    do {
        value = ((uint32_t)(rand() & 32767) << 15)
              | (uint32_t)(rand() & 32767);
    } while (value >= range - range % bound);
    return (int)(value % bound);
}

static int compare_int(const void *a, const void *b)
{
    int x = *(const int *)a, y = *(const int *)b;
    return (x > y) - (x < y);
}

int main(int argc, char **argv)
{
    const int sizes[] = {1000, 2000, 4000, 8000, 16000, 20000};
    const char *names[] = {"Insertion Sort", "Shell Sort (Original)", "Shell Sort (Knuth)"};
    unsigned seed = (unsigned)time(NULL);
    int *original, *work, *reference;
    int k, algorithm, i;
    if (argc > 2) { fprintf(stderr, "Usage: %s [seed]\n", argv[0]); return 1; }
    if (argc == 2) {
        char *end;
        unsigned long value;
        errno = 0;
        value = strtoul(argv[1], &end, 10);
        if (argv[1][0] == '-' || end == argv[1] || *end || errno == ERANGE || value > UINT_MAX) {
            fputs("Invalid seed\n", stderr); return 1;
        }
        seed = (unsigned)value;
    }
    original = malloc(20000 * sizeof(int));
    work = malloc(20000 * sizeof(int));
    reference = malloc(20000 * sizeof(int));
    if (!original || !work || !reference) {
        fputs("Allocation failed\n", stderr);
        free(original); free(work); free(reference); return 1;
    }
    srand(seed);
    printf("Seed: %u\nRandom values: 0..1000000 (duplicates allowed)\nTrials per size: 1\n", seed);
    for (k = 0; k < 6; ++k) {
        int n = sizes[k];
        for (i = 0; i < n; ++i) original[i] = random_value();
        memcpy(reference, original, (size_t)n * sizeof(int));
        /* qsort는 검증 기준 생성용이며 세 정렬 및 비교 집계와 무관하다. */
        qsort(reference, (size_t)n, sizeof(int), compare_int);
        printf("\nData Size : %d\n", n);
        for (algorithm = 0; algorithm < 3; ++algorithm) {
            uint64_t comparisons;
            memcpy(work, original, (size_t)n * sizeof(int));
            comparisons = algorithm == 0 ? insertion_sort(work, n)
                : shell_sort(work, n, algorithm == 2);
            for (i = 0; i < n; ++i) {
                if (work[i] != reference[i]) {
                    fputs("Validation failed\n", stderr);
                    free(original); free(work); free(reference); return 1;
                }
            }
            printf("%-21s: %" PRIu64 " comparisons; Correct=PASS\n", names[algorithm], comparisons);
            if (algorithm != 0) {
                int gaps[sizeof(int) * CHAR_BIT], count = make_gaps(n, algorithm == 2, gaps);
                printf("Gaps: ");
                for (i = 0; i < count; ++i) printf("%d%s", gaps[i], i + 1 == count ? "\n" : ", ");
            }
        }
    }
    puts("\nAll 18 sorting and value preservation checks: PASS");
    free(original); free(work); free(reference);
    return 0;
}
