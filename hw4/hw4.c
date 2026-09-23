#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_INPUT 4096

typedef struct Node {
    char data;
    struct Node *left;
    struct Node *right;
} Node;

typedef struct Frame {
    Node *node;
    int state;
} Frame;

int is_data(char ch)
{
    return ch >= 'A' && ch <= 'Z';
}

Node *new_node(char data)
{
    Node *node = (Node *)malloc(sizeof(Node));
    if (node != NULL) {
        node->data = data;
        node->left = NULL;
        node->right = NULL;
    }
    return node;
}

void destroy_tree(Node *tree)
{
    Node *stack[MAX_INPUT];
    int top = 0;
    if (tree != NULL)
        stack[top++] = tree;
    while (top > 0) {
        Node *node = stack[--top];
        if (node->right != NULL)
            stack[top++] = node->right;
        if (node->left != NULL)
            stack[top++] = node->left;
        free(node);
    }
}

Node *parse_tree(const char *text)
{
    Frame stack[MAX_INPUT];
    Node *root;
    int top = 0;
    size_t pos = 0;
    if (!is_data(text[pos])) {
        puts("오류: 트리는 영문 대문자 노드로 시작해야 합니다.");
        return NULL;
    }
    root = new_node(text[pos++]);
    if (root == NULL) {
        puts("오류: 메모리를 할당할 수 없습니다.");
        return NULL;
    }
    stack[top++] = (Frame){root, 0};
    while (top > 0) {
        Frame *frame = &stack[top - 1];
        char ch = text[pos];
        if (frame->state == 0) {
            if (ch == '(') {
                pos++;
                frame->state = 1;
            }
            else
                top--;
        }
        else if (frame->state == 1 || frame->state == 3) {
            int side = frame->state;
            if (side == 1 && ch == ',') {
                pos++;
                frame->state = 3;
            }
            else if (side == 3 && ch == ')' && frame->node->left != NULL) {
                pos++;
                top--;
            }
            else if (is_data(ch)) {
                Node *child = new_node(ch);
                if (child == NULL) {
                    puts("오류: 메모리를 할당할 수 없습니다.");
                    destroy_tree(root);
                    return NULL;
                }
                if (side == 1) {
                    frame->node->left = child;
                    frame->state = 2;
                }
                else {
                    frame->node->right = child;
                    frame->state = 4;
                }
                pos++;
                stack[top++] = (Frame){child, 0};
            }
            else
                goto invalid;
        }
        else if (frame->state == 2 && ch == ',') {
            pos++;
            frame->state = 3;
        }
        else if (ch == ')') {
            pos++;
            top--;
        }
        else
            goto invalid;
    }
    if (text[pos] != '\0')
        goto invalid;
    return root;

invalid:
    printf("오류: 괄호 표기법이 올바르지 않습니다. (공백 제외 %zu번째 위치)\n",
           pos + 1);
    destroy_tree(root);
    return NULL;
}

void print_tree(Node *tree)
{
    Frame stack[MAX_INPUT];
    int top = 0;
    if (tree != NULL)
        stack[top++] = (Frame){tree, 0};
    while (top > 0) {
        Frame item = stack[--top];
        int depth = item.state;
        int i;
        if (depth > 0) {
            for (i = 1; i < depth; i++)
                printf("    ");
            printf("+---");
        }
        printf("%c\n", item.node->data);
        if (item.node->right != NULL)
            stack[top++] = (Frame){item.node->right, depth + 1};
        if (item.node->left != NULL)
            stack[top++] = (Frame){item.node->left, depth + 1};
    }
}

void visit(Node *node, int *first)
{
    if (!*first)
        printf(" ");
    printf("%c", node->data);
    *first = 0;
}

void preorder(Node *tree)
{
    Node *stack[MAX_INPUT];
    int top = 0;
    int first = 1;
    if (tree != NULL)
        stack[top++] = tree;
    while (top > 0) {
        Node *node = stack[--top];
        visit(node, &first);
        if (node->right != NULL)
            stack[top++] = node->right;
        if (node->left != NULL)
            stack[top++] = node->left;
    }
    printf("\n");
}

void inorder(Node *tree)
{
    Node *stack[MAX_INPUT];
    Node *current = tree;
    int top = 0;
    int first = 1;
    while (current != NULL || top > 0) {
        while (current != NULL) {
            stack[top++] = current;
            current = current->left;
        }
        current = stack[--top];
        visit(current, &first);
        current = current->right;
    }
    printf("\n");
}

void postorder(Node *tree)
{
    Node *stack[MAX_INPUT];
    Node *current = tree;
    Node *last_visited = NULL;
    int top = 0;
    int first = 1;
    while (current != NULL || top > 0) {
        if (current != NULL) {
            stack[top++] = current;
            current = current->left;
        }
        else {
            Node *node = stack[top - 1];
            if (node->right != NULL && last_visited != node->right)
                current = node->right;
            else {
                visit(node, &first);
                last_visited = node;
                top--;
            }
        }
    }
    printf("\n");
}

int main(void)
{
    char input[MAX_INPUT];
    char text[MAX_INPUT];
    size_t i;
    size_t length = 0;
    Node *tree;
    printf("이진트리 입력: ");
    fflush(stdout);
    if (fgets(input, sizeof(input), stdin) == NULL) {
        puts("오류: 입력된 트리가 없습니다.");
        return 1;
    }
    if (strchr(input, '\n') == NULL && !feof(stdin)) {
        puts("오류: 입력이 너무 깁니다. 한 줄에 최대 4094자를 입력하세요.");
        return 1;
    }
    for (i = 0; input[i] != '\0'; i++) {
        if (!isspace((unsigned char)input[i]))
            text[length++] = input[i];
    }
    text[length] = '\0';
    tree = parse_tree(text);
    if (tree == NULL)
        return 1;
    printf("\n입력한 트리: %s\n", text);
    puts("\n[트리 구조]");
    print_tree(tree);
    printf("\nPreorder  : ");
    preorder(tree);
    printf("Inorder   : ");
    inorder(tree);
    printf("Postorder : ");
    postorder(tree);
    destroy_tree(tree);
    return 0;
}
