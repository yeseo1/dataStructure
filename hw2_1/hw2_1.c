#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define MAX 10000

char tree[MAX];
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

void makeTree(char *str, int index)
{
    if (index >= MAX)
        return;

    if (str[pos] < 'A' || str[pos] > 'Z')
        return;

    tree[index] = str[pos++];

    if (str[pos] == '(') {
        pos++;

        if (str[pos] != ',' && str[pos] != ')')
            makeTree(str, index * 2);

        if (str[pos] == ',') {
            pos++;

            if (str[pos] != ')')
                makeTree(str, index * 2 + 1);
        }

        if (str[pos] == ')')
            pos++;
    }
}

void printTree(int index, int level)
{
    int i;

    if (index >= MAX || tree[index] == '\0')
        return;

    if (level == 0) {
        printf("%c\n", tree[index]);
    }
    else {
        for (i = 0; i < level - 1; i++)
            printf("    ");

        printf("+---%c\n", tree[index]);
    }

    printTree(index * 2, level + 1);
    printTree(index * 2 + 1, level + 1);
}

int countNode(int index)
{
    if (index >= MAX || tree[index] == '\0')
        return 0;

    return 1
        + countNode(index * 2)
        + countNode(index * 2 + 1);
}

int countLeaf(int index)
{
    int left;
    int right;

    if (index >= MAX || tree[index] == '\0')
        return 0;

    left =
        (index * 2 < MAX &&
         tree[index * 2] != '\0');

    right =
        (index * 2 + 1 < MAX &&
         tree[index * 2 + 1] != '\0');

    if (!left && !right)
        return 1;

    return countLeaf(index * 2)
        + countLeaf(index * 2 + 1);
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

int getDegree(int index)
{
    int degree = 0;
    int leftDegree;
    int rightDegree;
    int max;

    if (index >= MAX || tree[index] == '\0')
        return 0;

    if (index * 2 < MAX &&
        tree[index * 2] != '\0')
        degree++;

    if (index * 2 + 1 < MAX &&
        tree[index * 2 + 1] != '\0')
        degree++;

    leftDegree = getDegree(index * 2);
    rightDegree = getDegree(index * 2 + 1);

    max = degree;

    if (leftDegree > max)
        max = leftDegree;

    if (rightDegree > max)
        max = rightDegree;

    return max;
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

int isComplete(void)
{
    int nodeCount;
    int maxIndex;

    nodeCount = countNode(1);
    maxIndex = getMaxIndex();

    if (nodeCount == maxIndex)
        return 1;

    return 0;
}

int isPerfect(int index)
{
    int left;
    int right;

    if (index >= MAX || tree[index] == '\0')
        return 1;

    left =
        (index * 2 < MAX &&
         tree[index * 2] != '\0');

    right =
        (index * 2 + 1 < MAX &&
         tree[index * 2 + 1] != '\0');

    if (!left && !right)
        return 1;

    if (!left || !right)
        return 0;

    return isPerfect(index * 2)
        && isPerfect(index * 2 + 1);
}

int sameLeafLevel(int index, int level, int height)
{
    int left;
    int right;

    if (index >= MAX || tree[index] == '\0')
        return 1;

    left =
        (index * 2 < MAX &&
         tree[index * 2] != '\0');

    right =
        (index * 2 + 1 < MAX &&
         tree[index * 2 + 1] != '\0');

    if (!left && !right) {
        if (level == height)
            return 1;
        else
            return 0;
    }

    return sameLeafLevel(index * 2,
                         level + 1,
                         height)
        &&
           sameLeafLevel(index * 2 + 1,
                         level + 1,
                         height);
}

int isOneDirection(int index, int direction)
{
    int left;
    int right;

    if (index >= MAX || tree[index] == '\0')
        return 1;

    left =
        (index * 2 < MAX &&
         tree[index * 2] != '\0');

    right =
        (index * 2 + 1 < MAX &&
         tree[index * 2 + 1] != '\0');

    if (!left && !right)
        return 1;

    if (left && right)
        return 0;

    if (direction == 0)
        return !right && isOneDirection(index * 2, direction);

    return !left && isOneDirection(index * 2 + 1, direction);
}

int isSkewed(int index)
{
    return isOneDirection(index, 0)
        || isOneDirection(index, 1);
}

int main(void)
{
    char input[MAX];
    char str[MAX];

    int nodeCount;
    int leafCount;
    int nonLeafCount;
    int height;
    int degree;
    int perfect;

    memset(tree, 0, sizeof(tree));

    printf("이진트리 입력 : ");
    fgets(input, sizeof(input), stdin);

    removeSpace(input, str);

    pos = 0;
    makeTree(str, 1);

    printf("\n[이진트리 출력]\n");
    printTree(1, 0);

    nodeCount = countNode(1);
    leafCount = countLeaf(1);
    nonLeafCount = nodeCount - leafCount;
    height = getHeight(str);
    degree = getDegree(1);

    printf("\n[트리 정보]\n");
    printf("전체 노드 수 : %d\n", nodeCount);
    printf("단말 노드 수 : %d\n", leafCount);
    printf("비단말 노드 수 : %d\n", nonLeafCount);
    printf("트리의 높이 : %d\n", height);
    printf("트리의 차수 : %d\n", degree);

    perfect =
        isPerfect(1) &&
        sameLeafLevel(1, 0, height);

    printf("\n[트리 형태]\n");

    if (isComplete())
        printf("완전 이진트리 : 예\n");
    else
        printf("완전 이진트리 : 아니오\n");

    if (perfect)
        printf("포화 이진트리 : 예\n");
    else
        printf("포화 이진트리 : 아니오\n");

    if (isSkewed(1))
        printf("편향 이진트리 : 예\n");
    else
        printf("편향 이진트리 : 아니오\n");

    return 0;
}
