/* ast.c - AST node creation, printing and cleanup */
#include "ast.h"

#include <stdio.h>
#include <stdlib.h>

static AstNode *alloc_node(AstType type, int line) {
    AstNode *node = calloc(1, sizeof(AstNode)); /* calloc sets all pointers to NULL */
    if (node != NULL) {
        node->type = type;
        node->line = line;
    }
    return node;
}

static void copy_name(AstNode *node, const char *text) {
    snprintf(node->name, MAX_LEXEME, "%s", text);
}

AstNode *ast_new_program(int line) {
    return alloc_node(AST_PROGRAM, line);
}

AstNode *ast_new_number(const char *text, int line) {
    AstNode *node = alloc_node(AST_NUMBER, line);
    if (node != NULL) {
        copy_name(node, text);
    }
    return node;
}

AstNode *ast_new_variable(const char *name, int line) {
    AstNode *node = alloc_node(AST_VARIABLE, line);
    if (node != NULL) {
        copy_name(node, name);
    }
    return node;
}

AstNode *ast_new_binary(char op, AstNode *left, AstNode *right, int line) {
    AstNode *node = alloc_node(AST_BINARY, line);
    if (node == NULL) {
        ast_free(left);
        ast_free(right);
        return NULL;
    }
    node->op = op;
    node->left = left;
    node->right = right;
    return node;
}

AstNode *ast_new_unary(char op, AstNode *operand, int line) {
    AstNode *node = alloc_node(AST_UNARY, line);
    if (node == NULL) {
        ast_free(operand);
        return NULL;
    }
    node->op = op;
    node->left = operand;
    return node;
}

AstNode *ast_new_assign(const char *name, AstNode *expression, int line) {
    AstNode *node = alloc_node(AST_ASSIGN, line);
    if (node == NULL) {
        ast_free(expression);
        return NULL;
    }
    copy_name(node, name);
    node->left = expression;
    return node;
}

AstNode *ast_new_print(AstNode *expression, int line) {
    AstNode *node = alloc_node(AST_PRINT, line);
    if (node == NULL) {
        ast_free(expression);
        return NULL;
    }
    node->left = expression;
    return node;
}

/* ---------- printing ---------- */

static void print_indent(int level) {
    int i;
    for (i = 0; i < level * 2; i++) {
        putchar(' ');
    }
}

static void print_node(const AstNode *node, int level) {
    print_indent(level);
    switch (node->type) {
        case AST_NUMBER:
            printf("Number: %s\n", node->name);
            break;
        case AST_VARIABLE:
            printf("Variable: %s\n", node->name);
            break;
        case AST_UNARY:
            printf("UnaryOp: %c\n", node->op);
            print_node(node->left, level + 1);
            break;
        case AST_BINARY:
            printf("BinaryOp: %c\n", node->op);
            print_node(node->left, level + 1);
            print_node(node->right, level + 1);
            break;
        case AST_ASSIGN:
            printf("Assignment: %s\n", node->name);
            print_node(node->left, level + 1);
            break;
        case AST_PRINT:
            printf("Print\n");
            print_node(node->left, level + 1);
            break;
        case AST_PROGRAM:
            printf("Program\n");
            break;
    }
}

void ast_print(const AstNode *program) {
    const AstNode *statement;
    print_node(program, 0);
    for (statement = program->left; statement != NULL; statement = statement->next) {
        print_node(statement, 1);
    }
}

/* ---------- cleanup ---------- */

/* Children are freed recursively; the statement list is freed with a loop so a
   very long program cannot overflow the stack. */
void ast_free(AstNode *node) {
    while (node != NULL) {
        AstNode *next = node->next;
        ast_free(node->left);
        ast_free(node->right);
        free(node);
        node = next;
    }
}
