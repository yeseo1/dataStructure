#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_NODES 100
#define INPUT_SIZE 1024

typedef struct Node {
    char data;
    struct Node *left;
    struct Node *right;
} Node;

typedef struct BinaryTree {
    Node *root;
    int size;
    int count;
} BinaryTree;

typedef struct ChildInfo {
    int found;  /* 경로가 존재하면 1, 없으면 0 */
    char left;  /* 자식이 없으면 '\0' */
    char right;
} ChildInfo;

int valid_value(char value)
{
    return value >= 'A' && value <= 'Z';
}

BinaryTree *create_btree(int size)
{
    BinaryTree *tree;
    if (size <= 0)
        return NULL;
    tree = (BinaryTree *)malloc(sizeof(BinaryTree));
    if (tree != NULL) {
        tree->root = NULL;
        tree->size = size;
        tree->count = 0;
    }
    return tree;
}

Node *create_node(char value)
{
    Node *node = (Node *)malloc(sizeof(Node));
    if (node != NULL) {
        node->data = value;
        node->left = NULL;
        node->right = NULL;
    }
    return node;
}

int valid_path(const char *path)
{
    size_t i;
    size_t length = strlen(path);
    if (length < 2 || length % 2 != 0)
        return 0;
    for (i = 0; i < length; i += 2) {
        if (path[i] != '/' || !valid_value(path[i + 1]))
            return 0;
    }
    return 1;
}

Node **find_link(BinaryTree *tree, const char *path, Node **parent)
{
    Node **link;
    size_t i;
    *parent = NULL;
    if (!valid_path(path))
        return NULL;
    link = &tree->root;
    if (*link == NULL || (*link)->data != path[1])
        return NULL;
    for (i = 2; path[i] != '\0'; i += 2) {
        Node *current = *link;
        *parent = current;
        if (current->left != NULL && current->left->data == path[i + 1])
            link = &current->left;
        else if (current->right != NULL && current->right->data == path[i + 1])
            link = &current->right;
        else
            return NULL;
    }
    return link;
}

BinaryTree *insert_root(BinaryTree *tree, char value)
{
    Node *node;
    if (!valid_value(value))
        puts("오류: 데이터는 영문 대문자 한 글자여야 합니다.");
    else if (tree->root != NULL)
        puts("오류: 루트는 빈 트리에만 추가할 수 있습니다.");
    else if (tree->count >= tree->size)
        puts("오류: 최대 노드 수에 도달했습니다.");
    else {
        node = create_node(value);
        if (node == NULL)
            puts("오류: 메모리를 할당할 수 없습니다.");
        else {
            tree->root = node;
            tree->count++;
            puts("루트를 추가했습니다.");
        }
    }
    return tree;
}

BinaryTree *insert_child(BinaryTree *tree, const char *parent,
                         char child, char value)
{
    Node *ancestor;
    Node **link = find_link(tree, parent, &ancestor);
    Node *node;
    if (link == NULL)
        puts("오류: 부모 경로가 올바르지 않거나 존재하지 않습니다.");
    else if (child != 'L' && child != 'R')
        puts("오류: 자식 위치는 L 또는 R이어야 합니다.");
    else if (!valid_value(value))
        puts("오류: 데이터는 영문 대문자 한 글자여야 합니다.");
    else if (((*link)->left != NULL && (*link)->left->data == value) ||
             ((*link)->right != NULL && (*link)->right->data == value))
        puts("오류: 같은 부모 아래에 같은 데이터의 자식을 추가할 수 없습니다.");
    else if ((child == 'L' && (*link)->left != NULL) ||
             (child == 'R' && (*link)->right != NULL))
        puts("오류: 지정한 자식 위치에 이미 노드가 있습니다.");
    else if (tree->count >= tree->size)
        puts("오류: 최대 노드 수에 도달했습니다.");
    else {
        node = create_node(value);
        if (node == NULL)
            puts("오류: 메모리를 할당할 수 없습니다.");
        else {
            if (child == 'L')
                (*link)->left = node;
            else
                (*link)->right = node;
            tree->count++;
            puts("자식을 추가했습니다.");
        }
    }
    return tree;
}

BinaryTree *delete_node(BinaryTree *tree, const char *leaf)
{
    Node *parent;
    Node **link = find_link(tree, leaf, &parent);
    if (link == NULL)
        puts("오류: 삭제할 경로가 올바르지 않거나 존재하지 않습니다.");
    else if ((*link)->left != NULL || (*link)->right != NULL)
        puts("오류: 단말 노드만 삭제할 수 있습니다.");
    else {
        free(*link);
        *link = NULL;
        tree->count--;
        puts("노드를 삭제했습니다.");
    }
    return tree;
}

BinaryTree *update_value(BinaryTree *tree, const char *path, char value)
{
    Node *parent;
    Node **link = find_link(tree, path, &parent);
    Node *sibling = NULL;
    if (link == NULL) {
        puts("오류: 수정할 경로가 올바르지 않거나 존재하지 않습니다.");
        return tree;
    }
    if (parent != NULL)
        sibling = (parent->left == *link) ? parent->right : parent->left;
    if (!valid_value(value))
        puts("오류: 데이터는 영문 대문자 한 글자여야 합니다.");
    else if (sibling != NULL && sibling->data == value)
        puts("오류: 형제 노드와 같은 데이터로 수정할 수 없습니다.");
    else {
        (*link)->data = value;
        puts("데이터를 수정했습니다. 이후에는 변경된 경로를 사용하세요.");
    }
    return tree;
}

ChildInfo read_child(BinaryTree *tree, const char *parent)
{
    Node *ancestor;
    Node **link = find_link(tree, parent, &ancestor);
    ChildInfo result = {0, '\0', '\0'};
    if (link == NULL) {
        puts("오류: 조회할 경로가 올바르지 않거나 존재하지 않습니다.");
        return result;
    }
    result.found = 1;
    if ((*link)->left != NULL)
        result.left = (*link)->left->data;
    if ((*link)->right != NULL)
        result.right = (*link)->right->data;
    return result;
}

void print_nodes(Node *node, int level)
{
    int i;
    if (node == NULL)
        return;
    if (level > 0) {
        for (i = 1; i < level; i++)
            printf("    ");
        printf("+---");
    }
    printf("%c\n", node->data);
    print_nodes(node->left, level + 1);
    print_nodes(node->right, level + 1);
}

void print_btree(BinaryTree *tree)
{
    if (tree->root == NULL)
        puts("트리가 비어 있습니다.");
    else
        print_nodes(tree->root, 0);
}

void destroy_nodes(Node *node)
{
    if (node == NULL)
        return;
    destroy_nodes(node->left);
    destroy_nodes(node->right);
    free(node);
}

void destroy_btree(BinaryTree *tree)
{
    if (tree != NULL) {
        destroy_nodes(tree->root);
        free(tree);
    }
}

int command_is(const char *text, const char *full, const char *short_name)
{
    return strcmp(text, full) == 0 || strcmp(text, short_name) == 0;
}

int data_token(const char *text)
{
    return strlen(text) == 1 && valid_value(text[0]);
}

void print_menu(void)
{
    puts("\nI[nsert] / 데이터                 : 빈 트리에 루트 추가");
    puts("I[nsert] 부모경로 L[eft]/R[ight] 데이터 : 빈 자식 위치에 추가");
    puts("D[elete] 단말경로                 : 단말 삭제");
    puts("U[pdate] 노드경로 데이터          : 데이터 변경");
    puts("R[ead] 노드경로                   : 자식 조회");
    puts("P[rint]                           : 전체 트리 출력");
    puts("Q[uit]                            : 종료");
}

int main(void)
{
    BinaryTree *tree = create_btree(MAX_NODES);
    char input[INPUT_SIZE];
    if (tree == NULL) {
        puts("오류: 트리를 생성할 수 없습니다.");
        return 1;
    }
    print_menu();
    for (;;) {
        char *args[5];
        char *token;
        int count = 0;
        int format_error = 0;
        printf("\n명령> ");
        fflush(stdout);
        if (fgets(input, sizeof(input), stdin) == NULL)
            break;
        if (strchr(input, '\n') == NULL && !feof(stdin)) {
            int ch;
            while ((ch = getchar()) != '\n' && ch != EOF) { }
            puts("오류: 명령이 너무 깁니다.");
            continue;
        }
        token = strtok(input, " \t\r\n");
        while (token != NULL && count < 5) {
            args[count++] = token;
            token = strtok(NULL, " \t\r\n");
        }
        if (count == 0) {
            puts("오류: 명령을 입력하세요.");
            continue;
        }
        if (command_is(args[0], "Insert", "I")) {
            if (count == 3 && strcmp(args[1], "/") == 0 && data_token(args[2]))
                tree = insert_root(tree, args[2][0]);
            else if (count == 4 && data_token(args[3])) {
                char child = '\0';
                if (command_is(args[2], "Left", "L"))
                    child = 'L';
                else if (command_is(args[2], "Right", "R"))
                    child = 'R';
                tree = insert_child(tree, args[1], child, args[3][0]);
            }
            else
                format_error = 1;
        }
        else if (command_is(args[0], "Delete", "D")) {
            if (count == 2)
                tree = delete_node(tree, args[1]);
            else
                format_error = 1;
        }
        else if (command_is(args[0], "Update", "U")) {
            if (count == 3 && data_token(args[2]))
                tree = update_value(tree, args[1], args[2][0]);
            else
                format_error = 1;
        }
        else if (command_is(args[0], "Read", "R")) {
            if (count == 2) {
                ChildInfo info = read_child(tree, args[1]);
                if (info.found) {
                    if (!info.left && !info.right)
                        puts("자식이 없습니다.");
                    else {
                        if (info.left)
                            printf("%c(L)", info.left);
                        if (info.left && info.right)
                            printf(", ");
                        if (info.right)
                            printf("%c(R)", info.right);
                        printf("\n");
                    }
                }
            }
            else
                format_error = 1;
        }
        else if (command_is(args[0], "Print", "P")) {
            if (count == 1)
                print_btree(tree);
            else
                format_error = 1;
        }
        else if (command_is(args[0], "Quit", "Q")) {
            if (count == 1)
                break;
            format_error = 1;
        }
        else
            puts("오류: 알 수 없는 명령입니다.");
        if (format_error)
            puts("오류: 인자 개수 또는 데이터 형식이 올바르지 않습니다.");
    }
    destroy_btree(tree);
    return 0;
}
