/* lexer.h - Lexical analyzer interface (Phase 1: source text -> tokens) */
#ifndef LEXER_H
#define LEXER_H

#include <stddef.h>

/* Longest lexeme we store, including the terminating '\0'.
   So identifiers and numbers can be at most 63 characters long. */
#define MAX_LEXEME 64

/* All token types of the calculator language.
   NOTE: token_type_name() in lexer.c must list the names in this same order. */
typedef enum {
    TOKEN_IDENTIFIER,
    TOKEN_NUMBER,
    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_MULTIPLY,
    TOKEN_DIVIDE,
    TOKEN_ASSIGN,
    TOKEN_LPAREN,
    TOKEN_RPAREN,
    TOKEN_SEMICOLON,
    TOKEN_PRINT,
    TOKEN_EOF,
    TOKEN_INVALID
} TokenType;

/* One token: its type, the matched text (lexeme) and the line it was found on.
   error_message is NULL for good tokens; for TOKEN_INVALID it explains why. */
typedef struct {
    TokenType type;
    char lexeme[MAX_LEXEME];
    int line;
    const char *error_message;
} Token;

/* Lexer state: the source text, the current position and the current line. */
typedef struct {
    const char *source;
    size_t pos;
    int line;
} Lexer;

void lexer_init(Lexer *lexer, const char *source);

/* Returns the next token. After the end of input it keeps returning TOKEN_EOF. */
Token lexer_next(Lexer *lexer);

/* Scans the whole source. On success *tokens points to a malloc'ed array that
   always ends with TOKEN_EOF (the caller must free it) and *count is its size.
   Returns the number of lexical errors (TOKEN_INVALID tokens),
   or -1 if memory allocation failed. */
int lexer_tokenize(const char *source, Token **tokens, int *count);

/* Name of a token type, e.g. "PLUS". */
const char *token_type_name(TokenType type);

/* Human-readable description used in error messages, e.g. "identifier 'x'". */
void token_describe(const Token *token, char *buffer, size_t size);

#endif
