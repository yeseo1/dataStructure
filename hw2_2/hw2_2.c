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

int pos = 0;

void removeSpace(char *src, char *dest)
{
    int i, j = 0;

    for (i = 0; src[i] != '\0'; i++) {
        if (!isspace((unsigned char)src[i]))
            dest[j++] = src[i];
    }

    dest[j] = '\0';
}

Node *makeTree(char *str)
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
            node->left = makeTree(str);

        if (str[pos] == ',') {
            pos++;

            if (str[pos] != ')')
                node->right = makeTree(str);
        }

        if (str[pos] == ')')
            pos++;
    }

    return node;
}

void printTree(Node *root, int level)
{
    int i;

    if (root == NULL)
        return;

    if (level == 0) {
        printf("%c\n", root->data);
    }
    else {
        for (i = 0; i < level - 1; i++)
            printf("    ");

        printf("+---%c\n", root->data);
    }

    printTree(root->left, level + 1);
    printTree(root->right, level + 1);
}

int countNode(Node *root)
{
    if (root == NULL)
        return 0;

    return 1
        + countNode(root->left)
        + countNode(root->right);
}

int countLeaf(Node *root)
{
    if (root == NULL)
        return 0;

    if (root->left == NULL &&
        root->right == NULL)
        return 1;

    return countLeaf(root->left)
        + countLeaf(root->right);
}

int getHeight(char *str)
{
    int i;
    int top = 0;
    int height = 0;

    for (i = 0; str[i] != '\0'; i++) {
        if (str[i] == '(') {
            top++;

            if (top > height)
                height = top;
        }
        else if (str[i] == ')') {
            top--;
        }
    }

    return height;
}

int getDegree(Node *root)
{
    int degree = 0;
    int leftDegree;
    int rightDegree;
    int max;

    if (root == NULL)
        return 0;

    if (root->left != NULL)
        degree++;

    if (root->right != NULL)
        degree++;

    leftDegree = getDegree(root->left);
    rightDegree = getDegree(root->right);

    max = degree;

    if (leftDegree > max)
        max = leftDegree;

    if (rightDegree > max)
        max = rightDegree;

    return max;
}

int checkComplete(Node *root, int index, int nodeCount)
{
    if (root == NULL)
        return 1;

    if (index >= nodeCount)
        return 0;

    return checkComplete(root->left,
                         index * 2 + 1,
                         nodeCount)
        &&
           checkComplete(root->right,
                         index * 2 + 2,
                         nodeCount);
}

int isComplete(Node *root)
{
    int nodeCount;

    nodeCount = countNode(root);

    return checkComplete(root, 0, nodeCount);
}

int hasTwoOrZeroChildren(Node *root)
{
    if (root == NULL)
        return 1;

    if (root->left == NULL &&
        root->right == NULL)
        return 1;

    if (root->left == NULL ||
        root->right == NULL)
        return 0;

    return hasTwoOrZeroChildren(root->left)
        &&
           hasTwoOrZeroChildren(root->right);
}

int sameLeafLevel(Node *root, int level, int height)
{
    if (root == NULL)
        return 1;

    if (root->left == NULL &&
        root->right == NULL) {

        if (level == height)
            return 1;
        else
            return 0;
    }

    return sameLeafLevel(root->left,
                         level + 1,
                         height)
        &&
           sameLeafLevel(root->right,
                         level + 1,
                         height);
}

int isPerfect(Node *root, int height)
{
    return hasTwoOrZeroChildren(root)
        &&
           sameLeafLevel(root, 0, height);
}

int isOneDirection(Node *root, int direction)
{
    if (root == NULL)
        return 1;

    if (root->left == NULL &&
        root->right == NULL)
        return 1;

    if (root->left != NULL &&
        root->right != NULL)
        return 0;

    if (direction == 0)
        return root->right == NULL
            && isOneDirection(root->left, direction);

    return root->left == NULL
        && isOneDirection(root->right, direction);
}

int isSkewed(Node *root)
{
    return isOneDirection(root, 0)
        || isOneDirection(root, 1);
}

void freeTree(Node *root)
{
    if (root == NULL)
        return;

    freeTree(root->left);
    freeTree(root->right);

    free(root);
}

int main(void)
{
    char input[MAX];
    char str[MAX];

    Node *root;

    int nodeCount;
    int leafCount;
    int nonLeafCount;
    int height;
    int degree;

    printf("이진트리 입력 : ");
    fgets(input, sizeof(input), stdin);

    removeSpace(input, str);

    pos = 0;
    root = makeTree(str);

    printf("\n[이진트리 출력]\n");
    printTree(root, 0);

    nodeCount = countNode(root);
    leafCount = countLeaf(root);
    nonLeafCount = nodeCount - leafCount;
    height = getHeight(str);
    degree = getDegree(root);

    printf("\n[트리 정보]\n");
    printf("전체 노드 수 : %d\n", nodeCount);
    printf("단말 노드 수 : %d\n", leafCount);
    printf("비단말 노드 수 : %d\n", nonLeafCount);
    printf("트리의 높이 : %d\n", height);
    printf("트리의 차수 : %d\n", degree);

    printf("\n[트리 형태]\n");

    if (isComplete(root))
        printf("완전 이진트리 : 예\n");
    else
        printf("완전 이진트리 : 아니오\n");

    if (isPerfect(root, height))
        printf("포화 이진트리 : 예\n");
    else
        printf("포화 이진트리 : 아니오\n");

    if (isSkewed(root))
        printf("편향 이진트리 : 예\n");
    else
        printf("편향 이진트리 : 아니오\n");

    freeTree(root);

    return 0;
}
