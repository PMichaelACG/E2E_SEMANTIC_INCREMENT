/* Build: bash build.sh
 * Run:   ./build/quectoc-ast examples/sample.qc > build/ast.dot
 * View:  dot -Tsvg build/ast.dot -o build/ast.svg
 * Input may be omitted or '-' for stdin. Only DOT is written to stdout.
 * Exit codes: 0 success, 1 lexical/syntax error, 2 I/O/resource/usage error.
 */
#include "parse_context.h"
#include "ast_dot.h"
#include "semantic.h"
#include <stdio.h>
#include <string.h>

ParseContext parse_context = {0};
extern FILE *yyin;
int yyparse(void);
int yylex_destroy(void);

int main(int argc, char **argv)
{
    int semantic_mode = argc > 1 && strcmp(argv[1], "--semantic") == 0;
    int file_arg = semantic_mode ? 2 : 1;
    if (argc > file_arg + 1) {
        fprintf(stderr, "usage: %s [--semantic] [source.qc|-]\n", argv[0]);
        return 2;
    }
    yyin = stdin;
    if (argc == file_arg + 1 && strcmp(argv[file_arg], "-") != 0) {
        yyin = fopen(argv[file_arg], "rb");
        if (yyin == NULL) {
            perror(argv[file_arg]);
            return 2;
        }
    }

    int parsed = yyparse();
    int status = parsed == 2 || parse_context.out_of_memory ? 2 :
                 parsed != 0 || parse_context.had_error ? 1 : 0;
    if (ferror(yyin)) {
        fprintf(stderr, "input read failed\n");
        status = 2;
    }
    if (yyin != stdin && fclose(yyin) != 0) {
        perror("closing input");
        status = 2;
    }
    SemanticResult semantic = {0};
    if (status == 0 && semantic_mode)
        status = semantic_analyze(parse_context.root, &semantic, stderr);
    /* Emit a semantic graph only after successful checking. */
    if (status == 0 &&
        ((semantic_mode ? semantic_write_dot(stdout, parse_context.root) :
                          ast_write_dot(stdout, parse_context.root)) != 0 ||
         fflush(stdout) != 0)) {
        fprintf(stderr, "DOT output failed\n");
        status = 2;
    }
    if (parse_context.out_of_memory)
        fprintf(stderr, "out of memory while creating an AST node\n");
    yylex_destroy();
    semantic_destroy(&semantic);
    ast_destroy(&parse_context.tree);
    return status;
}
