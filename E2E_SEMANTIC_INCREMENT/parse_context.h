#ifndef QUECTOC_PARSE_CONTEXT_H
#define QUECTOC_PARSE_CONTEXT_H
#include "ast.h"
/* One input per process. The tree owns every allocated node. */
typedef struct {
    Ast tree;
    AstNode *root;
    int had_error;
    int out_of_memory;
} ParseContext;
extern ParseContext parse_context;
#endif
