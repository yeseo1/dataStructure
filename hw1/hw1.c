#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define MAX 1000

int main(void)
{
    char input[MAX];
    char str[MAX];

    int i, j = 0;
    int len;

    int nodeCount = 0;
    int leafCount = 0;
    int nonLeafCount = 0;

    int level = 0;
    int maxLevel = 0;

    int degreeStack[MAX];
    int degreeTop = -1;
    int maxDegree = 0;

    char nodeStack[MAX];
    int nodeTop = -1;

    char lastNode = '\0';

    char parentC = '\0';
    char childrenC[MAX];
    int childCCount = 0;

    printf("트리 입력: ");
    fgets(input, sizeof(input), stdin);

    for (i = 0; input[i] != '\0'; i++) {
        if (!isspace((unsigned char)input[i])) {
            str[j++] = input[i];
        }
    }

    str[j] = '\0';
    len = j;

    if (len == 0 || str[0] != 'A') {
        printf("오류: 루트 노드는 A여야 합니다.\n");
        return 1;
    }

    int balance = 0;

    for (i = 0; i < len; i++) {

        if (str[i] == '(') {
            balance++;
        }
        else if (str[i] == ')') {
            balance--;

            if (balance < 0) {
                printf("오류: 괄호 형식이 잘못되었습니다.\n");
                return 1;
            }
        }
        else if (str[i] == ',') {
        }
        else if (str[i] >= 'A' && str[i] <= 'Z') {
        }
        else {
            printf("오류: 잘못된 문자가 있습니다.\n");
            return 1;
        }
    }

    if (balance != 0) {
        printf("오류: 괄호의 개수가 맞지 않습니다.\n");
        return 1;
    }

    for (i = 0; i < len; i++) {

        char ch = str[i];

        if (ch >= 'A' && ch <= 'Z') {

            nodeCount++;

            if (level == 0) {
                printf("%c\n", ch);
            }
            else {
                int k;

                for (k = 1; k < level; k++)
                    printf("    ");

                printf("+---%c\n", ch);
            }

            if (degreeTop >= 0) {
                degreeStack[degreeTop]++;
            }

            if (ch == 'C') {
                if (nodeTop >= 0)
                    parentC = nodeStack[nodeTop];
            }

            if (nodeTop >= 0 && nodeStack[nodeTop] == 'C') {
                childrenC[childCCount++] = ch;
            }

          
            if (i + 1 >= len || str[i + 1] != '(') {
                leafCount++;
            }

            lastNode = ch;
        }

        else if (ch == '(') {

            level++;

            if (level > maxLevel)
                maxLevel = level;

            nodeStack[++nodeTop] = lastNode;

            degreeStack[++degreeTop] = 0;
        }

        else if (ch == ')') {

            if (degreeTop >= 0) {

                if (degreeStack[degreeTop] > maxDegree)
                    maxDegree = degreeStack[degreeTop];

                degreeTop--;
            }

            if (nodeTop >= 0)
                nodeTop--;

            level--;
        }
    }

    nonLeafCount = nodeCount - leafCount;

    printf("\n");
    printf("전체 노드 수 : %d\n", nodeCount);
    printf("단말 노드 수 : %d\n", leafCount);
    printf("비단말 노드 수 : %d\n", nonLeafCount);

    printf("트리의 높이 : %d\n", maxLevel);
    printf("트리의 차수 : %d\n", maxDegree);

    if (parentC != '\0')
        printf("노드 C의 부모 : %c\n", parentC);
    else
        printf("노드 C의 부모 : 없음\n");

    printf("노드 C의 자식 : ");

    if (childCCount == 0) {
        printf("없음");
    }
    else {
        for (i = 0; i < childCCount; i++) {
            printf("%c", childrenC[i]);

            if (i < childCCount - 1)
                printf(", ");
        }
    }

    printf("\n");

    return 0;
}
