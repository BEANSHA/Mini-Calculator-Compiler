/* lexer.c - Lexical analyzer (hand written, no Flex) */
#include "lexer.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Must be in the same order as the TokenType enum in lexer.h */
static const char *const TOKEN_NAMES[] = {
    "IDENTIFIER", "NUMBER", "PLUS", "MINUS", "MULTIPLY", "DIVIDE", "ASSIGN",
    "LPAREN", "RPAREN", "SEMICOLON", "PRINT", "EOF", "INVALID"
};

const char *token_type_name(TokenType type) {
    return TOKEN_NAMES[type];
}

void lexer_init(Lexer *lexer, const char *source) {
    lexer->source = source;
    lexer->pos = 0;
    lexer->line = 1;
}

/* Build a token from `len` characters of `text`. Long text is truncated. */
static Token make_token(TokenType type, const char *text, size_t len, int line) {
    Token token;
    memset(&token, 0, sizeof token);
    if (len >= MAX_LEXEME) {
        len = MAX_LEXEME - 1;
    }
    memcpy(token.lexeme, text, len);
    token.lexeme[len] = '\0';
    token.type = type;
    token.line = line;
    token.error_message = NULL;
    return token;
}

static Token make_invalid(const char *text, size_t len, int line, const char *message) {
    Token token = make_token(TOKEN_INVALID, text, len, line);
    token.error_message = message;
    return token;
}

/* Skip spaces, tabs, '\r' and newlines (counting lines). */
static void skip_whitespace(Lexer *lexer) {
    while (lexer->source[lexer->pos] != '\0') {
        char c = lexer->source[lexer->pos];
        if (c == '\n') {
            lexer->line++;
            lexer->pos++;
        } else if (isspace((unsigned char)c)) {
            lexer->pos++;
        } else {
            break;
        }
    }
}

/* identifier: letter or '_' followed by letters, digits or '_'. "print" is a keyword. */
static Token scan_word(Lexer *lexer) {
    const char *start = lexer->source + lexer->pos;
    size_t len = 0;
    while (isalnum((unsigned char)start[len]) || start[len] == '_') {
        len++;
    }
    lexer->pos += len;

    if (len >= MAX_LEXEME) {
        return make_invalid(start, len, lexer->line, "identifier too long (maximum 63 characters)");
    }
    if (len == 5 && strncmp(start, "print", 5) == 0) {
        return make_token(TOKEN_PRINT, start, len, lexer->line);
    }
    return make_token(TOKEN_IDENTIFIER, start, len, lexer->line);
}

/* number: digits, optionally followed by '.' and at least one digit. */
static Token scan_number(Lexer *lexer) {
    const char *start = lexer->source + lexer->pos;
    size_t len = 0;
    while (isdigit((unsigned char)start[len])) {
        len++;
    }
    if (start[len] == '.') {
        len++;
        if (!isdigit((unsigned char)start[len])) {
            lexer->pos += len;
            return make_invalid(start, len, lexer->line,
                                "malformed number (a digit is required after '.')");
        }
        while (isdigit((unsigned char)start[len])) {
            len++;
        }
    }
    lexer->pos += len;

    if (len >= MAX_LEXEME) {
        return make_invalid(start, len, lexer->line, "number too long (maximum 63 characters)");
    }
    return make_token(TOKEN_NUMBER, start, len, lexer->line);
}

Token lexer_next(Lexer *lexer) {
    char c;
    const char *here;
    int line;

    skip_whitespace(lexer);
    c = lexer->source[lexer->pos];
    here = lexer->source + lexer->pos;
    line = lexer->line;

    if (c == '\0') {
        return make_token(TOKEN_EOF, "EOF", 3, line);
    }
    if (isalpha((unsigned char)c) || c == '_') {
        return scan_word(lexer);
    }
    if (isdigit((unsigned char)c)) {
        return scan_number(lexer);
    }

    lexer->pos++; /* every remaining case is a single character */
    switch (c) {
        case '+': return make_token(TOKEN_PLUS, here, 1, line);
        case '-': return make_token(TOKEN_MINUS, here, 1, line);
        case '*': return make_token(TOKEN_MULTIPLY, here, 1, line);
        case '/': return make_token(TOKEN_DIVIDE, here, 1, line);
        case '=': return make_token(TOKEN_ASSIGN, here, 1, line);
        case '(': return make_token(TOKEN_LPAREN, here, 1, line);
        case ')': return make_token(TOKEN_RPAREN, here, 1, line);
        case ';': return make_token(TOKEN_SEMICOLON, here, 1, line);
        default:  return make_invalid(here, 1, line, "invalid character");
    }
}

int lexer_tokenize(const char *source, Token **tokens, int *count) {
    Lexer lexer;
    int capacity = 64;
    int used = 0;
    int errors = 0;
    Token *array = malloc((size_t)capacity * sizeof(Token));

    if (array == NULL) {
        return -1;
    }
    lexer_init(&lexer, source);

    for (;;) {
        Token token;
        if (used == capacity) {
            Token *bigger;
            capacity *= 2;
            bigger = realloc(array, (size_t)capacity * sizeof(Token));
            if (bigger == NULL) {
                free(array);
                return -1;
            }
            array = bigger;
        }
        token = lexer_next(&lexer);
        array[used++] = token;
        if (token.type == TOKEN_INVALID) {
            errors++;
        }
        if (token.type == TOKEN_EOF) {
            break;
        }
    }

    *tokens = array;
    *count = used;
    return errors;
}

void token_describe(const Token *token, char *buffer, size_t size) {
    switch (token->type) {
        case TOKEN_EOF:        snprintf(buffer, size, "end of file"); break;
        case TOKEN_IDENTIFIER: snprintf(buffer, size, "identifier '%s'", token->lexeme); break;
        case TOKEN_NUMBER:     snprintf(buffer, size, "number '%s'", token->lexeme); break;
        case TOKEN_PRINT:      snprintf(buffer, size, "keyword 'print'"); break;
        case TOKEN_INVALID:    snprintf(buffer, size, "invalid token '%s'", token->lexeme); break;
        default:               snprintf(buffer, size, "'%s'", token->lexeme); break;
    }
}
