/* parser.h - Syntax analyzer interface (Phase 2: tokens -> AST) */
#ifndef PARSER_H
#define PARSER_H

#include "ast.h"
#include "lexer.h"

typedef struct {
    const Token *tokens;      /* token array ending with TOKEN_EOF */
    int count;
    int pos;                  /* index of the current token        */
    int depth;                /* current nesting depth             */
    int had_error;            /* 1 after the first syntax error    */
    int error_line;
    char error_message[256];
} Parser;

void parser_init(Parser *parser, const Token *tokens, int count);

/* Parses the whole program.
   Returns the AST root, or NULL after a syntax error (details are stored in
   parser->error_line and parser->error_message). The caller frees the AST. */
AstNode *parse_program(Parser *parser);

#endif
