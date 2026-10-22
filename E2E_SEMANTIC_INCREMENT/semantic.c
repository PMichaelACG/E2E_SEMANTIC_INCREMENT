#include "semantic.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef struct Scope {
    const struct Scope *parent;
    size_t id;
} Scope;
typedef struct {
    SemanticResult *result;
    FILE *diagnostics;
    size_t next_scope;
    int internal_error;
} Checker;

static bool is(const AstNode *n, const char *kind)
{ return n && strcmp(n->kind, kind) == 0; }

static AstNode *child(AstNode *n, const char *role)
{
    for (AstNode *c = n->first_child; c; c = c->next_sibling)
        if (strcmp(c->role, role) == 0) return c;
    return NULL;
}

static void report(Checker *c, AstNode *n, const char *category, const char *name)
{
    ++c->result->errors;
    n->inferred_type = TYPE_ERROR;
    fprintf(c->diagnostics, "SEMANTIC_%s %s\n", category, name ? name : "");
}

static const Symbol *lookup_current(const Checker *c, const Scope *scope,
                                    const char *name)
{
    for (size_t i = c->result->count; i > 0; --i) {
        const Symbol *s = &c->result->symbols[i - 1];
        if (s->scope_id == scope->id && strcmp(s->name, name) == 0)
            return s;
    }
    return NULL;
}

static const Symbol *lookup(const Checker *c, const Scope *scope, const char *name)
{
    for (const Scope *s = scope; s; s = s->parent) {
        const Symbol *found = lookup_current(c, s, name);
        if (found) return found;
    }
    return NULL;
}

static bool insert(Checker *c, const Scope *scope, AstNode *decl, AstNode *name, Type t)
{
    SemanticResult *r = c->result;
    if (r->count == r->capacity) {
        if (r->capacity > SIZE_MAX / 2 / sizeof(Symbol)) return false;
        size_t cap = r->capacity ? r->capacity * 2 : 8;
        Symbol *items = realloc(r->symbols, cap * sizeof *items);
        if (!items) return false;
        r->symbols = items;
        r->capacity = cap;
    }
    r->symbols[r->count++] = (Symbol){name->text, t, decl, scope->id};
    decl->scope_id = scope->id;
    name->binding = decl;
    name->inferred_type = decl->inferred_type = t;
    return true;
}

static bool comparison(const char *op)
{
    return strcmp(op, "==") == 0 || strcmp(op, "!=") == 0 ||
           strcmp(op, "<") == 0 || strcmp(op, "<=") == 0 ||
           strcmp(op, ">") == 0 || strcmp(op, ">=") == 0;
}

static Type expression(Checker *c, const Scope *scope, AstNode *n)
{
    if (!n) { c->internal_error = 1; return TYPE_ERROR; }
    Type t = TYPE_ERROR;
    if (is(n, "Integer")) t = TYPE_INT;
    else if (is(n, "String")) t = TYPE_STRING;
    else if (is(n, "Identifier")) {
        const Symbol *s = lookup(c, scope, n->text);
        if (!s) report(c, n, "UNDECLARED", n->text);
        else { n->binding = s->declaration; t = s->type; }
    } else if (is(n, "Unary")) {
        Type operand = expression(c, scope, child(n, "operand"));
        if (operand == TYPE_INT) t = TYPE_INT;
        else if (operand != TYPE_ERROR) report(c, n, "TYPE", n->text);
    } else if (is(n, "Binary")) {
        Type left = expression(c, scope, child(n, "left"));
        Type right = expression(c, scope, child(n, "right"));
        if (left == TYPE_INT && right == TYPE_INT)
            t = comparison(n->text) ? TYPE_BOOL : TYPE_INT;
        else if (left != TYPE_ERROR && right != TYPE_ERROR)
            report(c, n, "TYPE", n->text);
    } else c->internal_error = 1;
    n->inferred_type = t;
    return t;
}

static void statement(Checker *c, const Scope *scope, AstNode *n);
static void statements(Checker *c, const Scope *scope, AstNode *container)
{
    for (AstNode *n = container->first_child; n; n = n->next_sibling)
        statement(c, scope, n);
}

static void statement(Checker *c, const Scope *scope, AstNode *n)
{
    if (!n) { c->internal_error = 1; return; }
    if (is(n, "Block")) {
        Scope inner = {scope, c->next_scope++};
        n->scope_id = inner.id;
        statements(c, &inner, n);
    } else if (is(n, "Declaration")) {
        AstNode *name = child(n, "name");
        Type t = expression(c, scope, child(n, "initializer"));
        if (!name) { c->internal_error = 1; return; }
        if (lookup_current(c, scope, name->text))
            report(c, n, "DUPLICATE", name->text);
        else if (t == TYPE_INT) {
            if (!insert(c, scope, n, name, t)) c->internal_error = 1;
        } else if (t != TYPE_ERROR) report(c, n, "TYPE", name->text);
    } else if (is(n, "Assign")) {
        Type target = expression(c, scope, child(n, "target"));
        Type value = expression(c, scope, child(n, "value"));
        if (target != TYPE_ERROR && value != TYPE_ERROR &&
            (target != TYPE_INT || value != TYPE_INT))
            report(c, n, "TYPE", "assignment");
    } else if (is(n, "Print")) {
        Type t = expression(c, scope, child(n, "value"));
        if (t != TYPE_ERROR && t != TYPE_INT && t != TYPE_STRING)
            report(c, n, "TYPE", "print");
    } else if (is(n, "If") || is(n, "While")) {
        Type condition = expression(c, scope, child(n, "condition"));
        if (condition != TYPE_BOOL && condition != TYPE_ERROR)
            report(c, n, "TYPE", "condition");
        if (is(n, "While")) statement(c, scope, child(n, "body"));
        else {
            statement(c, scope, child(n, "then"));
            AstNode *otherwise = child(n, "else");
            if (otherwise) statement(c, scope, otherwise);
        }
    } else c->internal_error = 1;
}

int semantic_analyze(AstNode *root, SemanticResult *result, FILE *diagnostics)
{
    if (!is(root, "Program") || !result || !diagnostics) return 2;
    Checker checker = {result, diagnostics, 1, 0};
    Scope global = {NULL, 0};
    statements(&checker, &global, root);
    if (checker.internal_error || ferror(diagnostics)) return 2;
    return result->errors ? 1 : 0;
}

void semantic_destroy(SemanticResult *result)
{
    if (!result) return;
    free(result->symbols);
    *result = (SemanticResult){0};
}

const char *semantic_type_name(Type t)
{
    switch (t) {
    case TYPE_INT: return "Int";
    case TYPE_BOOL: return "Bool";
    case TYPE_STRING: return "String";
    case TYPE_ERROR: return "Error";
    default: return "Unknown";
    }
}
