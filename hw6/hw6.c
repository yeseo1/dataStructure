#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <limits.h>

#define ATTEMPTS 100
#define SEARCHES 50
#define RANGE 1001

typedef struct Node {
    int value;
    int height;
    struct Node *left, *right;
} Node;

typedef struct Result {
    int found;
    long comparisons;
} Result;

int height(const Node *node) { return node ? node->height : 0; }
void update_height(Node *node)
{
    int l = height(node->left), r = height(node->right);
    node->height = 1 + (l > r ? l : r);
}
int balance(const Node *node)
{
    return height(node->left) - height(node->right);
}
Node *rotate_right(Node *root)
{
    Node *next = root->left;
    root->left = next->right;
    next->right = root;
    update_height(root);
    update_height(next);
    return next;
}
Node *rotate_left(Node *root)
{
    Node *next = root->right;
    root->right = next->left;
    next->left = root;
    update_height(root);
    update_height(next);
    return next;
}

/* BST와 AVL이 같은 삽입 비교 규칙을 사용한다.
   ==와 <를 실제 실행할 때마다 센다. 균형 판단은 높이로만 한다. */
Node *insert(Node *root, int value, int avl, long *comparisons, int *added)
{
    if (root == NULL) {
        Node *node = (Node *)malloc(sizeof(Node));
        if (node == NULL) {
            *added = -1;
            return NULL;
        }
        node->value = value;
        node->height = 1;
        node->left = node->right = NULL;
        *added = 1;
        return node;
    }
    (*comparisons)++;
    if (value == root->value) {
        *added = 0;
        return root;
    }
    (*comparisons)++;
    if (value < root->value)
        root->left = insert(root->left, value, avl, comparisons, added);
    else
        root->right = insert(root->right, value, avl, comparisons, added);
    if (*added != 1)
        return root;
    update_height(root);
    if (avl) {
        int factor = balance(root);
        if (factor > 1) {
            if (balance(root->left) < 0)
                root->left = rotate_left(root->left); /* LR */
            return rotate_right(root); /* LL 또는 LR의 두 번째 회전 */
        }
        if (factor < -1) {
            if (balance(root->right) > 0)
                root->right = rotate_right(root->right); /* RL */
            return rotate_left(root); /* RR 또는 RL의 두 번째 회전 */
        }
    }
    return root;
}
Result sequential_search(const int *array, int length, int key)
{
    Result result = {0, 0};
    int i;
    for (i = 0; i < length; i++) {
        result.comparisons++;
        if (key == array[i]) {
            result.found = 1;
            break;
        }
    }
    return result;
}
Result tree_search(const Node *root, int key)
{
    Result result = {0, 0};
    while (root != NULL) {
        result.comparisons++;
        if (key == root->value) {
            result.found = 1;
            break;
        }
        result.comparisons++;
        root = key < root->value ? root->left : root->right;
    }
    return result;
}
int random_value(void)
{
    unsigned long range = (unsigned long)RAND_MAX + 1UL;
    unsigned long limit = range - range % RANGE;
    unsigned long value;
    do { value = (unsigned long)rand(); } while (value >= limit);
    return (int)(value % RANGE);
}
void destroy(Node *root)
{
    if (root == NULL) return;
    destroy(root->left);
    destroy(root->right);
    free(root);
}
void print_values(const int *values, int count)
{
    int i;
    for (i = 0; i < count; i++)
        printf("%4d%s", values[i], (i + 1) % 10 == 0 || i + 1 == count ? "\n" : " ");
}
int main(int argc, char **argv)
{
    unsigned int seed = (unsigned int)time(NULL);
    int generated[ATTEMPTS], array[ATTEMPTS], keys[SEARCHES];
    int length = 0, i, successes = 0;
    long construction[3] = {0, 0, 0}, total[3] = {0, 0, 0};
    Node *bst = NULL, *avl = NULL;
    const char *names[] = {"Array", "BST", "AVL"};
    if (argc > 2) {
        fprintf(stderr, "Usage: %s [seed]\n", argv[0]);
        return 1;
    }
    if (argc == 2) {
        char *end;
        unsigned long value;
        errno = 0;
        value = strtoul(argv[1], &end, 10);
        if (argv[1][0] == '-' || end == argv[1] || *end || errno == ERANGE || value > UINT_MAX) {
            fputs("Error: invalid seed.\n", stderr);
            return 1;
        }
        seed = (unsigned int)value;
    }
    srand(seed);
    for (i = 0; i < ATTEMPTS; i++) {
        int b_added = 0, a_added = 0;
        Result duplicate;
        generated[i] = random_value();
        duplicate = sequential_search(array, length, generated[i]);
        construction[0] += duplicate.comparisons;
        if (!duplicate.found) array[length++] = generated[i];
        /* 배열의 검색 결과와 관계없이 각 트리에서 독립적으로 중복을 검사한다. */
        bst = insert(bst, generated[i], 0, &construction[1], &b_added);
        avl = insert(avl, generated[i], 1, &construction[2], &a_added);
        if (b_added < 0 || a_added < 0 || b_added != !duplicate.found || a_added != b_added) {
            fputs("Error: allocation failure or inconsistent insertion.\n", stderr);
            destroy(bst); destroy(avl);
            return 1;
        }
    }
    for (i = 0; i < SEARCHES; i++) keys[i] = random_value();
    printf("Seed: %u\n\nGenerated integers (100 attempts):\n", seed);
    print_values(generated, ATTEMPTS);
    printf("\nStored values: %d\nSkipped duplicates: %d\n", length, ATTEMPTS - length);
    puts("Stored array (first occurrence order):");
    print_values(array, length);
    puts("\nConstruction comparisons:");
    for (i = 0; i < 3; i++) printf("%s: %ld\n", names[i], construction[i]);
    printf("\nArray length: %d\nBST height: %d\nAVL height: %d\n", length, height(bst), height(avl));
    puts("\nSearch keys (50):");
    print_values(keys, SEARCHES);
    puts("\n No  Key  Array result  Comparisons  BST result  Comparisons  AVL result  Comparisons");
    for (i = 0; i < SEARCHES; i++) {
        Result results[3];
        int j;
        results[0] = sequential_search(array, length, keys[i]);
        results[1] = tree_search(bst, keys[i]);
        results[2] = tree_search(avl, keys[i]);
        if (results[0].found != results[1].found || results[0].found != results[2].found) {
            fputs("Error: inconsistent search results.\n", stderr);
            destroy(bst); destroy(avl);
            return 1;
        }
        successes += results[0].found;
        printf("%3d %4d", i + 1, keys[i]);
        for (j = 0; j < 3; j++) {
            total[j] += results[j].comparisons;
            printf("  %-10s %11ld", results[j].found ? "Found" : "Not found", results[j].comparisons);
        }
        putchar('\n');
    }
    printf("\nSearches: %d\nFound: %d\nNot found: %d\n", SEARCHES, successes, SEARCHES - successes);
    for (i = 0; i < 3; i++) {
        printf("%s search total: %ld\n", names[i], total[i]);
        printf("%s search average: %.2f\n", names[i], (double)total[i] / SEARCHES);
        printf("%s construction + search: %ld\n", names[i], construction[i] + total[i]);
    }
    destroy(bst); destroy(avl);
    return 0;
}
