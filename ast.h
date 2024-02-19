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
    ASTOPTYPE_CALL
} AstOperatorType;

typedef struct AstNode AstNode;
struct AstNode
{
    AstNode *left;
    AstNode *right;
    Token *tokenValue;
    AstOperatorType operator;
};

extern void astFreeTree(AstNode *head);
extern bool ast(TokenVector *tv, int tvOffset, AstNode **tree);
extern void astPrettyPrintTree(AstNode *head, int tickCount);

#endif //CCOMPILER_AST_H