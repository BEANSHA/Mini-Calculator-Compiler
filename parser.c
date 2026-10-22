/* parser.c - Hand-written recursive-descent parser. One function per grammar rule.
 *
 * Grammar (the ExpressionTail / TermTail rules are written as while-loops):
 *   Program        -> { Statement }
 *   Statement      -> Assignment | PrintStatement
 *   Assignment     -> IDENTIFIER '=' Expression ';'
 *   PrintStatement -> 'print' '(' Expression ')' ';'
 *   Expression     -> Term   { ('+' | '-') Term }
 *   Term           -> Unary  { ('*' | '/') Unary }
 *   Unary          -> ('+' | '-') Unary | Factor
 *   Factor         -> NUMBER | IDENTIFIER | '(' Expression ')'
 */
#include "parser.h"

#include <stdarg.h>
#include <stdio.h>

#define MAX_NESTING_DEPTH 200

static AstNode *parse_statement(Parser *p);
static AstNode *parse_assignment(Parser *p);
static AstNode *parse_print_statement(Parser *p);
static AstNode *parse_expression(Parser *p);
static AstNode *parse_term(Parser *p);
static AstNode *parse_unary(Parser *p);
static AstNode *parse_factor(Parser *p);

/* ---------- small helpers ---------- */

static const Token *current(const Parser *p) {
    return &p->tokens[p->pos];
}

/* Move to the next token (never past the final EOF token). */
static void advance(Parser *p) {
    if (p->tokens[p->pos].type != TOKEN_EOF) {
        p->pos++;
    }
}

static int check(const Parser *p, TokenType type) {
    return current(p)->type == type;
}

static int previous_line(const Parser *p) {
    return p->pos > 0 ? p->tokens[p->pos - 1].line : current(p)->line;
}

/* Remember the FIRST error only; later errors would just be noise. */
static void set_error(Parser *p, int line, const char *format, ...) {
    va_list args;
    if (p->had_error) {
        return;
    }
    p->had_error = 1;
    p->error_line = line;
    va_start(args, format);
    vsnprintf(p->error_message, sizeof p->error_message, format, args);
    va_end(args);
}

/* If a constructor returned NULL, memory ran out. */
static AstNode *check_alloc(Parser *p, AstNode *node) {
    if (node == NULL) {
        set_error(p, current(p)->line, "out of memory");
    }
    return node;
}

/* Guard against absurdly deep nesting such as ((((...)))) so we never overflow the stack. */
static int enter_nested(Parser *p) {
    if (p->depth >= MAX_NESTING_DEPTH) {
        set_error(p, current(p)->line,
                  "expression is nested too deeply (limit is %d levels)", MAX_NESTING_DEPTH);
        return 0;
    }
    p->depth++;
    return 1;
}

/* Expect ')' that closes a '(' opened on open_line. */
static int expect_rparen(Parser *p, int open_line) {
    char found[96];
    if (check(p, TOKEN_RPAREN)) {
        advance(p);
        return 1;
    }
    token_describe(current(p), found, sizeof found);
    set_error(p, current(p)->line,
              "unmatched '(' opened on line %d: expected ')' but found %s", open_line, found);
    return 0;
}

/* Expect the ';' that ends a statement. */
static int expect_semicolon(Parser *p) {
    char found[96];
    if (check(p, TOKEN_SEMICOLON)) {
        advance(p);
        return 1;
    }
    if (check(p, TOKEN_RPAREN)) {
        set_error(p, current(p)->line, "unmatched ')': there is no matching '('");
        return 0;
    }
    token_describe(current(p), found, sizeof found);
    /* Report the line of the statement that is missing its ';' */
    set_error(p, previous_line(p), "missing ';' at the end of the statement (found %s instead)", found);
    return 0;
}

/* ---------- grammar rules ---------- */

void parser_init(Parser *parser, const Token *tokens, int count) {
    parser->tokens = tokens;
    parser->count = count;
    parser->pos = 0;
    parser->depth = 0;
    parser->had_error = 0;
    parser->error_line = 0;
    parser->error_message[0] = '\0';
}

/* Program -> { Statement } */
AstNode *parse_program(Parser *p) {
    AstNode *program = check_alloc(p, ast_new_program(1));
    AstNode *last = NULL;

    if (program == NULL) {
        return NULL;
    }
    while (!check(p, TOKEN_EOF)) {
        AstNode *statement = parse_statement(p);
        if (statement == NULL) {
            ast_free(program); /* never return a half-built tree */
            return NULL;
        }
        if (last == NULL) {
            program->left = statement;
        } else {
            last->next = statement;
        }
        last = statement;
    }
    return program;
}

/* Statement -> Assignment | PrintStatement */
static AstNode *parse_statement(Parser *p) {
    char found[96];
    if (check(p, TOKEN_IDENTIFIER)) {
        return parse_assignment(p);
    }
    if (check(p, TOKEN_PRINT)) {
        return parse_print_statement(p);
    }
    token_describe(current(p), found, sizeof found);
    set_error(p, current(p)->line,
              "invalid statement: a statement must start with a variable name or 'print', found %s",
              found);
    return NULL;
}

/* Assignment -> IDENTIFIER '=' Expression ';' */
static AstNode *parse_assignment(Parser *p) {
    Token id = *current(p); /* copy: we need the name after advancing */
    AstNode *expression;
    char found[96];

    advance(p);
    if (!check(p, TOKEN_ASSIGN)) {
        token_describe(current(p), found, sizeof found);
        set_error(p, current(p)->line, "expected '=' after variable '%s' but found %s", id.lexeme, found);
        return NULL;
    }
    advance(p);

    expression = parse_expression(p);
    if (expression == NULL) {
        return NULL;
    }
    if (!expect_semicolon(p)) {
        ast_free(expression);
        return NULL;
    }
    return check_alloc(p, ast_new_assign(id.lexeme, expression, id.line));
}

/* PrintStatement -> 'print' '(' Expression ')' ';' */
static AstNode *parse_print_statement(Parser *p) {
    int line = current(p)->line;
    int open_line;
    AstNode *expression;
    char found[96];

    advance(p); /* skip 'print' */
    if (!check(p, TOKEN_LPAREN)) {
        token_describe(current(p), found, sizeof found);
        set_error(p, current(p)->line, "expected '(' after 'print' but found %s", found);
        return NULL;
    }
    open_line = current(p)->line;
    advance(p);

    expression = parse_expression(p);
    if (expression == NULL) {
        return NULL;
    }
    if (!expect_rparen(p, open_line) || !expect_semicolon(p)) {
        ast_free(expression);
        return NULL;
    }
    return check_alloc(p, ast_new_print(expression, line));
}

/* Expression -> Term { ('+' | '-') Term }
   The loop keeps adding to the LEFT, so 10 - 4 - 3 becomes (10 - 4) - 3. */
static AstNode *parse_expression(Parser *p) {
    AstNode *left = parse_term(p);
    if (left == NULL) {
        return NULL;
    }
    while (check(p, TOKEN_PLUS) || check(p, TOKEN_MINUS)) {
        char op = current(p)->lexeme[0];
        int line = current(p)->line;
        AstNode *right;
        advance(p);
        right = parse_term(p);
        if (right == NULL) {
            ast_free(left);
            return NULL;
        }
        left = check_alloc(p, ast_new_binary(op, left, right, line));
        if (left == NULL) {
            return NULL;
        }
    }
    return left;
}

/* Term -> Unary { ('*' | '/') Unary }
   Term is called from Expression, so '*' and '/' end up BELOW '+' and '-' in the tree. */
static AstNode *parse_term(Parser *p) {
    AstNode *left = parse_unary(p);
    if (left == NULL) {
        return NULL;
    }
    while (check(p, TOKEN_MULTIPLY) || check(p, TOKEN_DIVIDE)) {
        char op = current(p)->lexeme[0];
        int line = current(p)->line;
        AstNode *right;
        advance(p);
        right = parse_unary(p);
        if (right == NULL) {
            ast_free(left);
            return NULL;
        }
        left = check_alloc(p, ast_new_binary(op, left, right, line));
        if (left == NULL) {
            return NULL;
        }
    }
    return left;
}

/* Unary -> ('+' | '-') Unary | Factor */
static AstNode *parse_unary(Parser *p) {
    if (check(p, TOKEN_PLUS) || check(p, TOKEN_MINUS)) {
        char op = current(p)->lexeme[0];
        int line = current(p)->line;
        AstNode *operand;
        advance(p);
        if (!enter_nested(p)) {
            return NULL;
        }
        operand = parse_unary(p);
        p->depth--;
        if (operand == NULL) {
            return NULL;
        }
        return check_alloc(p, ast_new_unary(op, operand, line));
    }
    return parse_factor(p);
}

/* Factor -> NUMBER | IDENTIFIER | '(' Expression ')'
   Parentheses only control the grouping, so they do not create AST nodes. */
static AstNode *parse_factor(Parser *p) {
    const Token *token = current(p);
    char found[96];

    if (token->type == TOKEN_NUMBER) {
        AstNode *node = check_alloc(p, ast_new_number(token->lexeme, token->line));
        if (node != NULL) {
            advance(p);
        }
        return node;
    }
    if (token->type == TOKEN_IDENTIFIER) {
        AstNode *node = check_alloc(p, ast_new_variable(token->lexeme, token->line));
        if (node != NULL) {
            advance(p);
        }
        return node;
    }
    if (token->type == TOKEN_LPAREN) {
        int open_line = token->line;
        AstNode *inner;
        advance(p);
        if (!enter_nested(p)) {
            return NULL;
        }
        inner = parse_expression(p);
        p->depth--;
        if (inner == NULL) {
            return NULL;
        }
        if (!expect_rparen(p, open_line)) {
            ast_free(inner);
            return NULL;
        }
        return inner;
    }

    token_describe(token, found, sizeof found);
    set_error(p, token->line, "expected a number, a variable or '(' but found %s", found);
    return NULL;
}
