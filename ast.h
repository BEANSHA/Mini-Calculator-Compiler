/* ast.h - Abstract Syntax Tree definition */
#ifndef AST_H
#define AST_H

#include "lexer.h" /* for MAX_LEXEME */

typedef enum {
    AST_PROGRAM,   /* root: left = first statement, statements linked by next */
    AST_ASSIGN,    /* name = left                                              */
    AST_PRINT,     /* print(left)                                              */
    AST_BINARY,    /* left op right      (op is + - * /)                       */
    AST_UNARY,     /* op left            (op is + or -)                        */
    AST_NUMBER,    /* name holds the number's text, e.g. "3.14"                */
    AST_VARIABLE   /* name holds the variable name                             */
} AstType;

/* One general-purpose node type keeps the code short and easy to explain. */
typedef struct AstNode {
    AstType type;
    int line;                 /* source line, used in semantic error messages */
    char name[MAX_LEXEME];    /* variable name or number text                 */
    char op;                  /* '+', '-', '*' or '/' for operator nodes      */
    struct AstNode *left;     /* first child                                  */
    struct AstNode *right;    /* second child (binary operations only)        */
    struct AstNode *next;     /* next statement (statements only)             */
} AstNode;

/* Constructors. They return NULL if memory runs out. Constructors that take
   child nodes take ownership of them (and free them if allocation fails). */
AstNode *ast_new_program(int line);
AstNode *ast_new_number(const char *text, int line);
AstNode *ast_new_variable(const char *name, int line);
AstNode *ast_new_binary(char op, AstNode *left, AstNode *right, int line);
AstNode *ast_new_unary(char op, AstNode *operand, int line);
AstNode *ast_new_assign(const char *name, AstNode *expression, int line);
AstNode *ast_new_print(AstNode *expression, int line);

/* Print the tree with indentation. */
void ast_print(const AstNode *program);

/* Free a node, its children and every statement linked through next. */
void ast_free(AstNode *node);

#endif
