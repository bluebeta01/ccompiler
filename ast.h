#ifndef CCOMPILER_AST_H
#define CCOMPILER_AST_H
#include "tokenize.h"

typedef enum
{
    ASTOPTYPE_INVALID,
    ASTOPTYPE_MULTIPLY,
    ASTOPTYPE_DIVIDE,
    ASTOPTYPE_ADD,
    ASTOPTYPE_SUBTRACT,
    ASTOPTYPE_DOT,
    ASTOPTYPE_COMMA,
    ASTOPTYPE_CALL,
    ASTOPTYPE_REFERENCE,
    ASTOPTYPE_DEREFERENCE,
    ASTOPTYPE_EQUALS
} AstOperatorType;

typedef struct AstNode AstNode;
struct AstNode
{
    AstNode *left;
    AstNode *right;
    Token *tokenValue;
    AstOperatorType operator;
    bool isSubtree;
};

extern void ast_node_free_tree(AstNode *head);
extern bool ast(TokenVector *tv, int tvOffset, AstNode **tree, int *expressionEndIndex);
extern void ast_node_pretty_print(AstNode *head);
extern void ast_tree_to_list(AstNode *ast, AstNode **head, AstNode **tail);

#endif //CCOMPILER_AST_H
