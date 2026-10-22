#include "semantic.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static AstNode *node(Ast *t,const char *k,const char *v) {
    AstNode *n=ast_new(t,k,v); assert(n); return n;
}
static void edge(AstNode *p,const char *r,AstNode *c) { assert(ast_add_child(p,r,c)); }
int main(void) {
    /* API fixture: source grammar deliberately forbids string arithmetic. */
    Ast t={0}; SemanticResult r={0}; FILE *log=tmpfile(); assert(log);
    AstNode *p=node(&t,"Program",NULL), *print=node(&t,"Print",NULL);
    AstNode *plus=node(&t,"Binary","+");
    edge(p,"statement",print);edge(print,"value",plus);
    edge(plus,"left",node(&t,"String","hi"));
    edge(plus,"right",node(&t,"Integer","1"));
    assert(semantic_analyze(p,&r,log)==1 && r.errors==1);
    assert(plus->inferred_type==TYPE_ERROR);
    semantic_destroy(&r);ast_destroy(&t);fclose(log);
    /* Binding identity and initializer-before-insertion under shadowing. */
    t=(Ast){0};r=(SemanticResult){0};log=tmpfile();assert(log);
    p=node(&t,"Program",NULL);
    AstNode *outer=node(&t,"Declaration","let");edge(p,"statement",outer);
    edge(outer,"name",node(&t,"Identifier","x"));edge(outer,"initializer",node(&t,"Integer","1"));
    AstNode *block=node(&t,"Block",NULL);edge(p,"statement",block);
    AstNode *inner=node(&t,"Declaration","let");edge(block,"statement",inner);
    edge(inner,"name",node(&t,"Identifier","x"));
    AstNode *init=node(&t,"Identifier","x");edge(inner,"initializer",init);
    print=node(&t,"Print",NULL);edge(block,"statement",print);
    AstNode *use=node(&t,"Identifier","x");edge(print,"value",use);
    AstNode *after=node(&t,"Print",NULL);edge(p,"statement",after);
    AstNode *use_after=node(&t,"Identifier","x");edge(after,"value",use_after);
    assert(semantic_analyze(p,&r,log)==0 && r.count==2);
    assert(init->binding==outer && use->binding==inner && use_after->binding==outer);
    assert(outer->scope_id==0 && inner->scope_id==1);
    assert(r.symbols[1].declaration==inner);
    assert(semantic_analyze(NULL,&r,log)==2);
    semantic_destroy(&r);ast_destroy(&t);fclose(log);
    puts("PASS semantic API type and binding assertions");
}
