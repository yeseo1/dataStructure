#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX 10000

typedef struct Node {
    char data;
    struct Node *left;
    struct Node *right;
} Node;

char tree[MAX];
int pos;

void removeSpace(char *src, char *dest)
{
    int i, j = 0;

    for (i = 0; src[i] != '\0'; i++) {
        if (!isspace((unsigned char)src[i]))
            dest[j++] = src[i];
    }

    dest[j] = '\0';
}

void clearArray(void)
{
    memset(tree, 0, sizeof(tree));
}

void makeArrayTree(char *str, int index)
{
    if (index >= MAX)
        return;

    if (str[pos] < 'A' || str[pos] > 'Z')
        return;

    tree[index] = str[pos++];

    if (str[pos] == '(') {
        pos++;

        if (str[pos] != ',' && str[pos] != ')')
            makeArrayTree(str, index * 2);

        if (str[pos] == ',') {
            pos++;

            if (str[pos] != ')')
                makeArrayTree(str, index * 2 + 1);
        }

        if (str[pos] == ')')
            pos++;
    }
}

Node *makeLinkedTree(char *str)
{
    Node *node;

    if (str[pos] < 'A' || str[pos] > 'Z')
        return NULL;

    node = (Node *)malloc(sizeof(Node));

    if (node == NULL) {
        printf("메모리 할당 실패\n");
        exit(1);
    }

    node->data = str[pos++];
    node->left = NULL;
    node->right = NULL;

    if (str[pos] == '(') {
        pos++;

        if (str[pos] != ',' && str[pos] != ')')
            node->left = makeLinkedTree(str);

        if (str[pos] == ',') {
            pos++;

            if (str[pos] != ')')
                node->right = makeLinkedTree(str);
        }

        if (str[pos] == ')')
            pos++;
    }

    return node;
}

int countArrayNodes(void)
{
    int i;
    int count = 0;

    for (i = 1; i < MAX; i++) {
        if (tree[i] != '\0')
            count++;
    }

    return count;
}

int getMaxIndex(void)
{
    int i;

    for (i = MAX - 1; i >= 1; i--) {
        if (tree[i] != '\0')
            return i;
    }

    return 0;
}

int countLinkedNodes(Node *root)
{
    if (root == NULL)
        return 0;

    return 1
        + countLinkedNodes(root->left)
        + countLinkedNodes(root->right);
}

void freeTree(Node *root)
{
    if (root == NULL)
        return;

    freeTree(root->left);
    freeTree(root->right);

    free(root);
}

int findArrayNode(char target)
{
    int i;

    for (i = 1; i < MAX; i++) {
        if (tree[i] == target)
            return i;
    }

    return -1;
}

void printArrayRelation(char target)
{
    int index;
    int sibling;

    index = findArrayNode(target);

    printf("\n[배열 방식]\n");

    if (index == -1) {
        printf("노드가 없습니다.\n");
        return;
    }

    if (index == 1)
        printf("부모 : 없음\n");
    else
        printf("부모 : %c\n", tree[index / 2]);

    if (index * 2 < MAX &&
        tree[index * 2] != '\0')
        printf("왼쪽 자식 : %c\n",
               tree[index * 2]);
    else
        printf("왼쪽 자식 : 없음\n");

    if (index * 2 + 1 < MAX &&
        tree[index * 2 + 1] != '\0')
        printf("오른쪽 자식 : %c\n",
               tree[index * 2 + 1]);
    else
        printf("오른쪽 자식 : 없음\n");

    if (index == 1) {
        printf("형제 : 없음\n");
    }
    else {
        if (index % 2 == 0)
            sibling = index + 1;
        else
            sibling = index - 1;

        if (sibling < MAX &&
            tree[sibling] != '\0')
            printf("형제 : %c\n",
                   tree[sibling]);
        else
            printf("형제 : 없음\n");
    }
}

Node *findLinkedNode(Node *root, char target)
{
    Node *result;

    if (root == NULL)
        return NULL;

    if (root->data == target)
        return root;

    result = findLinkedNode(root->left, target);

    if (result != NULL)
        return result;

    return findLinkedNode(root->right, target);
}

Node *findParent(Node *root, char target)
{
    Node *result;

    if (root == NULL)
        return NULL;

    if ((root->left != NULL &&
         root->left->data == target) ||
        (root->right != NULL &&
         root->right->data == target))
        return root;

    result = findParent(root->left, target);

    if (result != NULL)
        return result;

    return findParent(root->right, target);
}

void printLinkedRelation(Node *root, char target)
{
    Node *node;
    Node *parent;
    Node *sibling = NULL;

    node = findLinkedNode(root, target);

    printf("\n[연결 방식]\n");

    if (node == NULL) {
        printf("노드가 없습니다.\n");
        return;
    }

    parent = findParent(root, target);

    if (parent == NULL)
        printf("부모 : 없음\n");
    else
        printf("부모 : %c\n",
               parent->data);

    if (node->left != NULL)
        printf("왼쪽 자식 : %c\n",
               node->left->data);
    else
        printf("왼쪽 자식 : 없음\n");

    if (node->right != NULL)
        printf("오른쪽 자식 : %c\n",
               node->right->data);
    else
        printf("오른쪽 자식 : 없음\n");

    if (parent != NULL) {
        if (parent->left == node)
            sibling = parent->right;
        else
            sibling = parent->left;
    }

    if (sibling != NULL)
        printf("형제 : %c\n",
               sibling->data);
    else
        printf("형제 : 없음\n");
}

void measureCase(char *name, char *str)
{
    Node *root;

    int nodeCount;
    int maxIndex;

    size_t arrayAllocated;
    size_t arrayRequired;
    size_t linkedMemory;

    clearArray();

    pos = 0;
    makeArrayTree(str, 1);

    nodeCount = countArrayNodes();
    maxIndex = getMaxIndex();

    arrayAllocated = sizeof(tree);

    arrayRequired =
        (size_t)(maxIndex + 1)
        * sizeof(char);

    pos = 0;
    root = makeLinkedTree(str);

    linkedMemory =
        (size_t)countLinkedNodes(root)
        * sizeof(Node);

    printf("\n%s\n", name);
    printf("트리 : %s\n", str);
    printf("노드 수 : %d\n", nodeCount);

    printf("배열 전체 할당량 : %zu byte\n",
           arrayAllocated);

    printf("배열 필요 범위 : %zu byte\n",
           arrayRequired);

    printf("배열 최대 인덱스 : %d\n",
           maxIndex);

    printf("연결 자료형 사용량 : %zu byte\n",
           linkedMemory);

    freeTree(root);
}

int main(void)
{
    char generalTree[] =
        "A(B(D,),C(,E))";

    char completeTree[] =
        "A(B(D,E),C(F,G))";

    char skewedTree[] =
        "A(,B(,C(,D)))";

    char input[MAX];
    char str[MAX];
    char target;

    Node *root;

    printf("sizeof(char) = %zu byte\n",
           sizeof(char));

    printf("sizeof(Node) = %zu byte\n",
           sizeof(Node));

    printf("==메모리 사용량 비교==\n");

    measureCase("일반 이진트리",
                generalTree);

    measureCase("완전 이진트리",
                completeTree);

    measureCase("편향 이진트리",
                skewedTree);

    printf("==부모, 자식, 형제 검색==\n");

    printf("이진트리 입력 : ");
    fgets(input, sizeof(input), stdin);

    removeSpace(input, str);

    clearArray();

    pos = 0;
    makeArrayTree(str, 1);

    pos = 0;
    root = makeLinkedTree(str);

    printf("찾을 노드 입력 : ");
    scanf(" %c", &target);

    printArrayRelation(target);
    printLinkedRelation(root, target);

    freeTree(root);

    return 0;
}
