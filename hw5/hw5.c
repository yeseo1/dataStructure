#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <limits.h>

#define DATA_COUNT 100
#define SEARCH_COUNT 50
#define VALUE_COUNT 1001

typedef struct Node {
    int value;
    struct Node *left;
    struct Node *right;
} Node;

typedef struct SearchResult {
    int found;
    long comparisons;
} SearchResult;

int random_value(void)
{
    unsigned long range = (unsigned long)RAND_MAX + 1UL;
    unsigned long limit = range - range % VALUE_COUNT;
    unsigned long value;
    do {
        value = (unsigned long)rand();
    } while (value >= limit);
    return (int)(value % VALUE_COUNT);
}

void generate_data(int array[])
{
    int used[VALUE_COUNT] = {0};
    int count = 0;
    while (count < DATA_COUNT) {
        int value = random_value();
        if (!used[value]) {
            used[value] = 1;
            array[count++] = value;
        }
    }
}

int insert_bst(Node **tree, int value, long *comparisons)
{
    Node **link = tree;
    Node *node;
    while (*link != NULL) {
        (*comparisons)++;
        if (value < (*link)->value)
            link = &(*link)->left;
        else
            link = &(*link)->right;
    }
    node = (Node *)malloc(sizeof(Node));
    if (node == NULL)
        return 0;
    node->value = value;
    node->left = NULL;
    node->right = NULL;
    *link = node;
    return 1;
}

SearchResult sequential_search(const int array[], int count, int key)
{
    SearchResult result = {0, 0};
    int i;
    for (i = 0; i < count; i++) {
        result.comparisons++;
        if (key == array[i]) {
            result.found = 1;
            break;
        }
    }
    return result;
}

SearchResult bst_search(const Node *tree, int key)
{
    SearchResult result = {0, 0};
    while (tree != NULL) {
        result.comparisons++;
        if (key == tree->value) {
            result.found = 1;
            break;
        }
        result.comparisons++;
        if (key < tree->value)
            tree = tree->left;
        else
            tree = tree->right;
    }
    return result;
}

void destroy_bst(Node *tree)
{
    Node *stack[DATA_COUNT];
    int top = 0;
    if (tree != NULL)
        stack[top++] = tree;
    while (top > 0) {
        Node *node = stack[--top];
        if (node->left != NULL)
            stack[top++] = node->left;
        if (node->right != NULL)
            stack[top++] = node->right;
        free(node);
    }
}

int bst_height(const Node *tree)
{
    const Node *stack[DATA_COUNT];
    int depths[DATA_COUNT];
    int top = 0;
    int height = -1;
    if (tree != NULL) {
        stack[top] = tree;
        depths[top++] = 0;
    }
    while (top > 0) {
        const Node *node = stack[--top];
        int depth = depths[top];
        if (depth > height)
            height = depth;
        if (node->left != NULL) {
            stack[top] = node->left;
            depths[top++] = depth + 1;
        }
        if (node->right != NULL) {
            stack[top] = node->right;
            depths[top++] = depth + 1;
        }
    }
    return height;
}

void print_values(const int values[], int count)
{
    int i;
    for (i = 0; i < count; i++)
        printf("%4d%s", values[i], (i + 1) % 10 == 0 || i + 1 == count ? "\n" : " ");
}

int main(int argc, char *argv[])
{
    int array[DATA_COUNT];
    int keys[SEARCH_COUNT];
    Node *tree = NULL;
    unsigned int seed = (unsigned int)time(NULL);
    long build_total = 0;
    long sequential_total = 0;
    long bst_total = 0;
    long sequential_found = 0;
    long bst_found = 0;
    int found_count = 0;
    int i;

    if (argc > 2) {
        fprintf(stderr, "Usage: %s [seed]\n", argv[0]);
        return 1;
    }
    if (argc == 2) {
        char *end;
        unsigned long value;
        errno = 0;
        value = strtoul(argv[1], &end, 10);
        if (argv[1][0] == '-' || end == argv[1] || *end != '\0' ||
            errno == ERANGE || value > UINT_MAX) {
            fprintf(stderr, "Error: seed must be an unsigned integer.\n");
            return 1;
        }
        seed = (unsigned int)value;
    }
    srand(seed);
    generate_data(array);
    for (i = 0; i < DATA_COUNT; i++) {
        if (!insert_bst(&tree, array[i], &build_total)) {
            fprintf(stderr, "Error: memory allocation failed.\n");
            destroy_bst(tree);
            return 1;
        }
    }
    for (i = 0; i < SEARCH_COUNT; i++)
        keys[i] = random_value();

    printf("Seed: %u\n", seed);
    puts("\nGenerated data (in generation order):");
    print_values(array, DATA_COUNT);
    printf("\nBST build comparisons: %ld\n", build_total);
    printf("BST height (root depth = 0): %d\n", bst_height(tree));
    puts("\nSearch keys:");
    print_values(keys, SEARCH_COUNT);
    puts("\n No  Key  Sequential result  Comparisons  BST result  Comparisons");
    for (i = 0; i < SEARCH_COUNT; i++) {
        SearchResult seq = sequential_search(array, DATA_COUNT, keys[i]);
        SearchResult bst = bst_search(tree, keys[i]);
        if (seq.found != bst.found) {
            fprintf(stderr, "Error: search results disagree.\n");
            destroy_bst(tree);
            return 1;
        }
        sequential_total += seq.comparisons;
        bst_total += bst.comparisons;
        if (seq.found) {
            found_count++;
            sequential_found += seq.comparisons;
            bst_found += bst.comparisons;
        }
        printf("%3d %4d  %-17s %11ld  %-10s %11ld\n", i + 1, keys[i],
               seq.found ? "Found" : "Not found", seq.comparisons,
               bst.found ? "Found" : "Not found", bst.comparisons);
    }
    printf("\nNumber of searches: %d\n", SEARCH_COUNT);
    printf("Found: %d, Not found: %d\n", found_count, SEARCH_COUNT - found_count);
    printf("Sequential total: %ld\n", sequential_total);
    printf("Sequential average: %.2f\n", (double)sequential_total / SEARCH_COUNT);
    printf("BST search total: %ld\n", bst_total);
    printf("BST search average: %.2f\n", (double)bst_total / SEARCH_COUNT);
    printf("BST build comparisons: %ld\n", build_total);
    printf("BST build + search: %ld\n", build_total + bst_total);
    printf("Sequential minus BST (including build): %ld\n",
           sequential_total - build_total - bst_total);
    if (found_count > 0) {
        printf("Found averages: sequential=%.2f, BST=%.2f\n",
               (double)sequential_found / found_count, (double)bst_found / found_count);
    }
    if (found_count < SEARCH_COUNT) {
        printf("Not-found averages: sequential=%.2f, BST=%.2f\n",
               (double)(sequential_total - sequential_found) / (SEARCH_COUNT - found_count),
               (double)(bst_total - bst_found) / (SEARCH_COUNT - found_count));
    }
    destroy_bst(tree);
    return 0;
}
