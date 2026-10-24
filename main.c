/* main.c - Driver: runs the compiler front-end phases in order.
 *
 *   Source -> [Lexer] -> tokens -> [Parser] -> AST -> [Semantic analyzer] -> result
 *
 * A phase runs only if the previous phase succeeded.
 * This is a FRONT-END only: it does not generate code and does not run the program.
 */
#include <stdio.h>
#include <stdlib.h>

#include "ast.h"
#include "lexer.h"
#include "parser.h"
#include "semantic.h"

/* Read a whole stream (file or stdin) into a malloc'ed, '\0'-terminated string. */
static char *read_stream(FILE *fp) {
    size_t capacity = 1024;
    size_t length = 0;
    size_t got;
    char *buffer = malloc(capacity);

    if (buffer == NULL) {
        return NULL;
    }
    while ((got = fread(buffer + length, 1, capacity - length - 1, fp)) > 0) {
        length += got;
        if (length == capacity - 1) {
            char *bigger;
            capacity *= 2;
            bigger = realloc(buffer, capacity);
            if (bigger == NULL) {
                free(buffer);
                return NULL;
            }
            buffer = bigger;
        }
    }
    buffer[length] = '\0';
    return buffer;
}

static void print_heading(const char *title) {
    printf("\n==================================================\n");
    printf(" %s\n", title);
    printf("==================================================\n");
}

/* Show the source program with line numbers. */
static void print_source(const char *source) {
    int line = 1;
    const char *p = source;

    if (*p == '\0') {
        printf("(empty program)\n");
        return;
    }
    printf("%3d | ", line);
    for (; *p != '\0'; p++) {
        if (*p == '\r') {
            continue;
        }
        putchar(*p);
        if (*p == '\n' && p[1] != '\0') {
            printf("%3d | ", ++line);
        }
    }
    if (p[-1] != '\n') {
        putchar('\n');
    }
}

static void print_tokens(const Token *tokens, int count) {
    int i;
    printf("%-6s %-12s %s\n", "Line", "Token", "Lexeme");
    printf("------ ------------ ----------------\n");
    for (i = 0; i < count; i++) {
        printf("%-6d %-12s %s\n", tokens[i].line, token_type_name(tokens[i].type), tokens[i].lexeme);
    }
}

static int fail(const char *phase) {
    printf("\nCOMPILATION FAILED: %s.\n", phase);
    return 1;
}

int main(int argc, char *argv[]) {
    FILE *input = stdin;
    char *source;
    Token *tokens = NULL;
    int token_count = 0;
    int lexical_errors;
    Parser parser;
    AstNode *ast;
    int semantic_errors;
    int i;

    /* ---- read the source program ---- */
    if (argc > 2) {
        fprintf(stderr, "Usage: %s [source_file]\n", argv[0]);
        return 2;
    }
    if (argc == 2) {
        input = fopen(argv[1], "r");
        if (input == NULL) {
            fprintf(stderr, "Error: cannot open file '%s'\n", argv[1]);
            return 2;
        }
    } else {
        printf("Enter your program. Finish with Ctrl+D (Linux/macOS) or Ctrl+Z then Enter (Windows):\n");
    }
    source = read_stream(input);
    if (input != stdin) {
        fclose(input);
    }
    if (source == NULL) {
        fprintf(stderr, "Error: out of memory while reading the input\n");
        return 2;
    }

    print_heading("SOURCE PROGRAM");
    print_source(source);

    /* ---- Phase 1: lexical analysis ---- */
    print_heading("PHASE 1: LEXICAL ANALYSIS (token stream)");
    lexical_errors = lexer_tokenize(source, &tokens, &token_count);
    if (lexical_errors < 0) {
        fprintf(stderr, "Error: out of memory during lexical analysis\n");
        free(source);
        return 2;
    }
    print_tokens(tokens, token_count);

    if (lexical_errors > 0) {
        printf("\n");
        for (i = 0; i < token_count; i++) {
            if (tokens[i].type == TOKEN_INVALID) {
                printf("Lexical error (line %d): %s '%s'\n",
                       tokens[i].line, tokens[i].error_message, tokens[i].lexeme);
            }
        }
        printf("%d lexical error(s) found. Later phases are skipped.\n", lexical_errors);
        free(tokens);
        free(source);
        return fail("lexical errors");
    }
    printf("\nLexical analysis succeeded (%d tokens including EOF).\n", token_count);

    /* ---- Phase 2: syntax analysis ---- */
    print_heading("PHASE 2: SYNTAX ANALYSIS (recursive-descent parser)");
    parser_init(&parser, tokens, token_count);
    ast = parse_program(&parser);
    if (ast == NULL) {
        printf("Syntax error (line %d): %s\n", parser.error_line, parser.error_message);
        printf("No AST is built for a program with syntax errors. Semantic analysis is skipped.\n");
        free(tokens);
        free(source);
        return fail("syntax error");
    }
    printf("Syntax analysis succeeded: the program matches the grammar.\n");

    /* ---- AST ---- */
    print_heading("ABSTRACT SYNTAX TREE (AST)");
    ast_print(ast);

    /* ---- Phase 3: semantic analysis ---- */
    print_heading("PHASE 3: SEMANTIC ANALYSIS");
    semantic_errors = semantic_analyze(ast);

    ast_free(ast);
    free(tokens);
    free(source);

    if (semantic_errors > 0) {
        printf("%d semantic error(s) found.\n", semantic_errors);
        return fail("semantic errors");
    }
    printf("Semantic analysis succeeded.\n");

    print_heading("RESULT");
    printf("COMPILATION SUCCESSFUL: lexical, syntax and semantic checks all passed.\n");
    printf("(This is a compiler front-end only: no code is generated and the program is not executed.)\n");
    return 0;
}
