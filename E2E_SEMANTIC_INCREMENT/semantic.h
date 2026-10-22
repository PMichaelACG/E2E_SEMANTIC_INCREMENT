#ifndef QUECTOC_SEMANTIC_H
#define QUECTOC_SEMANTIC_H
#include "ast.h"
#include <stdio.h>

typedef struct {
    const char *name;
    Type type;
    AstNode *declaration;
    size_t scope_id;
} Symbol;

typedef struct {
    Symbol *symbols;             /* registry, including closed scopes */
    size_t count, capacity, errors;
} SemanticResult;

/* Initialize result to {0}. Return 0 success, 1 source error, 2 internal/resource. */
int semantic_analyze(AstNode *root, SemanticResult *result, FILE *diagnostics);
void semantic_destroy(SemanticResult *result);
const char *semantic_type_name(Type type);
int semantic_write_dot(FILE *out, const AstNode *root);
#endif
