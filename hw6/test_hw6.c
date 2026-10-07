/* 별도 검증 프로그램: hw6.c와 함께 링크하지 않고 이 파일만 컴파일한다. */
#define main assignment_main
#include "hw6.c"
#undef main
#include <assert.h>

int validate(Node *root, int low, int high, int avl, int *count)
{
    int l, r;
    if (!root) return 0;
    assert(root->value > low && root->value < high);
    (*count)++;
    l = validate(root->left, low, root->value, avl, count);
    r = validate(root->right, root->value, high, avl, count);
    assert(root->height == 1 + (l > r ? l : r));
    if (avl) assert(abs(l - r) <= 1);
    return root->height;
}
int main(void)
{
    int patterns[4][3] = {{30,20,10},{10,20,30},{30,10,20},{10,30,20}};
    int p, i, s;
    for (p = 0; p < 4; p++) {
        Node *t = NULL;
        long c = 0;
        int added = 0, n = 0;
        for (i = 0; i < 3; i++) t = insert(t, patterns[p][i], 1, &c, &added);
        assert(t->value == 20 && t->left->value == 10 && t->right->value == 30);
        validate(t, -1, 1001, 1, &n);
        assert(n == 3 && c == 6);
        t = insert(t, 20, 1, &c, &added);
        assert(added == 0 && c == 7);
        destroy(t);
    }
    for (s = 0; s < 5; s++) {
        Node *b = NULL, *a = NULL;
        int array[100], len = 0, seen[1001] = {0};
        long bc = 0, ac = 0;
        srand((unsigned)s);
        for (i = 0; i < 100; i++) {
            int v = random_value(), ba = 0, aa = 0, bn = 0, an = 0;
            long bp = bc, ap = ac;
            Result sr = sequential_search(array, len, v);
            Result br = tree_search(b, v), ar = tree_search(a, v);
            assert(sr.found == seen[v]);
            if (!seen[v]) {
                assert(sr.comparisons == len);
                array[len++] = v;
            }
            b = insert(b, v, 0, &bc, &ba);
            a = insert(a, v, 1, &ac, &aa);
            assert(ba == !seen[v] && aa == ba);
            assert(bc - bp == br.comparisons && ac - ap == ar.comparisons);
            seen[v] = 1;
            validate(b, -1, 1001, 0, &bn);
            validate(a, -1, 1001, 1, &an);
            assert(bn == len && an == len);
        }
        for (i = 0; i <= 1000; i++) {
            Result sr = sequential_search(array, len, i);
            Result br = tree_search(b, i), ar = tree_search(a, i);
            assert(sr.found == seen[i] && br.found == seen[i] && ar.found == seen[i]);
            assert(br.comparisons <= 2 * height(b) && ar.comparisons <= 2 * height(a));
        }
        destroy(b); destroy(a);
    }
    {
        Node *b = NULL, *a = NULL;
        long bc = 0, ac = 0;
        int added, n = 0;
        assert(height(b) == 0);
        for (i = 0; i < 100; i++) {
            b = insert(b, i, 0, &bc, &added);
            a = insert(a, i, 1, &ac, &added);
        }
        assert(height(b) == 100 && bc == 9900);
        validate(a, -1, 1001, 1, &n);
        assert(n == 100 && height(a) == 7);
        printf("Sorted: BST build=%ld height=%d; AVL build=%ld height=%d\n",
               bc, height(b), ac, height(a));
        destroy(b); destroy(a);
    }
    puts("PASS: four rotations, duplicates, 500 insertion checks, 5005 searches, sorted input");
    return 0;
}
