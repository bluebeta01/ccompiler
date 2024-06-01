#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "ast.h"

static AstOperatorType operatorTypeFromStr(const char *str, int strLength)
{
    if (!strncmp(str, "*", strLength)) return ASTOPTYPE_MULTIPLY;
    if (!strncmp(str, "/", strLength)) return ASTOPTYPE_DIVIDE;
    if (!strncmp(str, "+", strLength)) return ASTOPTYPE_ADD;
    if (!strncmp(str, "-", strLength)) return ASTOPTYPE_SUBTRACT;
    if (!strncmp(str, ".", strLength)) return ASTOPTYPE_DOT;
    if (!strncmp(str, ",", strLength)) return ASTOPTYPE_COMMA;
    if (!strncmp(str, "=", strLength)) return ASTOPTYPE_EQUALS;
    return ASTOPTYPE_INVALID;
}

static int operatorPrecedence(AstOperatorType type)
{
    switch (type)
    {
        case ASTOPTYPE_ADD:
        case ASTOPTYPE_SUBTRACT:
            return 4;
        case ASTOPTYPE_MULTIPLY:
        case ASTOPTYPE_DIVIDE:
            return 3;
        case ASTOPTYPE_DOT:
        case ASTOPTYPE_CALL:
            return 1;
        case ASTOPTYPE_REFERENCE:
        case ASTOPTYPE_DEREFERENCE:
            return 2;
        case ASTOPTYPE_COMMA:
            return 15;
        case ASTOPTYPE_EQUALS:
            return 14;

        default:
            return 0;
    }
}


static int find_closing_paren(TokenVector *tv, int tvOffset)
{
    int counter = 0;
    for (int i = tvOffset; i < tv->length; i++)
    {
        Token *token = &tv->tokens[i];
        if (!strncmp(token->tokenStr, "(", token->tokenStrLength))
        {
            counter++;
            continue;
        }
        if (!strncmp(token->tokenStr, ")", token->tokenStrLength))
        {
            counter--;
            if (counter == 0)
                return i;
            continue;
        }
    }
    return -1;
}

void ast_tree_to_list(AstNode *ast, AstNode **head, AstNode **tail)
{
    if (ast->left)
        ast_tree_to_list(ast->left, head, tail);
    if (ast->right)
        ast_tree_to_list(ast->right, head, tail);

    if (!(*tail) && ast->tokenValue)
    {
        ast->left = NULL;
        ast->right = NULL;
        *tail = ast;
        *head = ast;
        return;
    }

    if (*tail)
    {
        (*tail)->right = ast;
        ast->left = *tail;
        ast->right = NULL;
        *tail = ast;
    }
}

void ast_node_pretty_print(AstNode *head)
{
    if (!head) return;
    if (head->left)
        ast_node_pretty_print(head->left);
    if (head->right)
        ast_node_pretty_print(head->right);

    if (head->operator == ASTOPTYPE_ADD)
        puts("+");
    if (head->operator == ASTOPTYPE_SUBTRACT)
        puts("s");
    if (head->operator == ASTOPTYPE_MULTIPLY)
        puts("*");
    if (head->operator == ASTOPTYPE_DIVIDE)
        puts("/");
    if (head->operator == ASTOPTYPE_DOT)
        puts("dot");
    if (head->operator == ASTOPTYPE_COMMA)
        puts("comma");
    if (head->operator == ASTOPTYPE_EQUALS)
        puts("=");
    if (head->operator == ASTOPTYPE_CALL)
    {
        fputs("call ", stdout);
        if (head->tokenValue)
        {
            for (int i = 0; i < head->tokenValue->tokenStrLength; i++)
            {
                fputc(head->tokenValue->tokenStr[i], stdout);
            }
        }
        fputc('\n', stdout);
    }
    if (head->operator == ASTOPTYPE_REFERENCE)
        puts("REF");
    if (head->operator == ASTOPTYPE_DEREFERENCE)
        puts("DEREF");
    if (head->operator == ASTOPTYPE_INVALID)
    {
        if (head->tokenValue == NULL)
        {
            puts("Node had operator invalid and no token value.");
            return;
        }
        for (int i = 0; i < head->tokenValue->tokenStrLength; i++)
        {
            putc(head->tokenValue->tokenStr[i], stdout);
        }
        puts("");
    }
}

void ast_node_free_tree(AstNode *head)
{
    if (!head)return;
    if(head->left)
        ast_node_free_tree(head->left);
    if(head->right)
        ast_node_free_tree(head->right);
    free(head);
}

static bool ast_check_subtree(TokenVector *tv, int tvOffset)
{
    Token *currentToken = &tv->tokens[tvOffset];
    if (!strncmp(currentToken->tokenStr, "(", currentToken->tokenStrLength))
        return true;
    if (tvOffset + 2 <= tv->length)
    {
        Token *nextToken = &tv->tokens[tvOffset + 1];
        if (!strncmp(nextToken->tokenStr, "(", nextToken->tokenStrLength))
            return true;
    }
    return false;
}

static bool ast_check_token(TokenVector *tv, int *tvOffset, AstNode **rootNode, AstNode **tree, int *endIndex)
{
    Token *firstToken = &tv->tokens[*tvOffset];
    if (!strncmp(firstToken->tokenStr, "(", firstToken->tokenStrLength))
    {
        int closingParenIndex = find_closing_paren(tv, *tvOffset);
        if (closingParenIndex == -1)
        {
            puts("Invalid expression. Could not find the closing paren.");
            *tree = *rootNode;
            return false;
        }
        bool result = ast(tv, (*tvOffset) + 1, rootNode, endIndex);
        if (!result)
        {
            *tree = *rootNode;
            return result;
        }
        *tvOffset = closingParenIndex;

        return true;
    }
    if (!strncmp(firstToken->tokenStr, "&", firstToken->tokenStrLength))
    {
        *rootNode = (AstNode *) calloc(1, sizeof(AstNode));
        (*rootNode)->operator = ASTOPTYPE_REFERENCE;
        (*rootNode)->left = calloc(1, sizeof(AstNode));
        (*rootNode)->left->tokenValue = &tv->tokens[(*tvOffset) + 1];
        *tree = *rootNode;
        *tvOffset += 1;

        return true;
    }
    if (!strncmp(firstToken->tokenStr, "*", firstToken->tokenStrLength))
    {
        *tvOffset += 1;
        AstNode *subTree = NULL;
        bool result = ast_check_token(tv, tvOffset, &subTree, tree, endIndex);
        if (!result) return result;

        *rootNode = (AstNode *) calloc(1, sizeof(AstNode));
        (*rootNode)->operator = ASTOPTYPE_DEREFERENCE;
        *tree = *rootNode;

        if (!subTree)
        {
            (*rootNode)->left = calloc(1, sizeof(AstNode));
            (*rootNode)->left->tokenValue = &tv->tokens[*tvOffset];
        } else
        {
            (*rootNode)->left = subTree;
        }

        return true;
    }
    if ((*tvOffset) + 2 <= tv->length)
    {
        Token *nextToken = &tv->tokens[(*tvOffset) + 1];
        if (!strncmp(nextToken->tokenStr, "(", nextToken->tokenStrLength))
        {
            int funcCallEndIndex = find_closing_paren(tv, (*tvOffset) + 1);
            if (funcCallEndIndex == -1)
            {
                puts("Could not parse function call.");
                *tree = *rootNode;
                return false;
            }
            AstNode *funcParamsTree = NULL;
            if (funcCallEndIndex > (*tvOffset) + 2)
            {
                bool result = ast(tv, (*tvOffset) + 2, &funcParamsTree, endIndex);
                if (!result)
                {
                    *tree = *rootNode;
                    return result;
                }
            }
            if (funcParamsTree && funcParamsTree->operator != ASTOPTYPE_COMMA)
            {
                AstNode *commaNode = calloc(1, sizeof(AstNode));
                commaNode->operator = ASTOPTYPE_COMMA;
                commaNode->left = funcParamsTree;
                funcParamsTree = commaNode;
            }
            AstNode *funcCallNode = calloc(1, sizeof(AstNode));
            funcCallNode->operator = ASTOPTYPE_CALL;
            funcCallNode->tokenValue = firstToken;
            funcCallNode->left = funcParamsTree;
            *rootNode = funcCallNode;
            *tvOffset = funcCallEndIndex;
        }
        return true;
    }

    return true;
}

bool ast(TokenVector *tv, int tvOffset, AstNode **tree, int *expressionEndIndex)
{
    AstNode *rootNode = NULL;
    Token *firstToken = &tv->tokens[tvOffset];
    bool result = false;

    result = ast_check_token(tv, &tvOffset, &rootNode, tree, expressionEndIndex);
    if (!result) return result;

    if (rootNode == NULL)
    {
        rootNode = calloc(1, sizeof(AstNode));
        rootNode->tokenValue = firstToken;
    } else
        rootNode->isSubtree = true;

    while (tvOffset < tv->length - 1)
    {
        if (tvOffset + 1 >= tv->length)
        {
            puts("Unexpected end of expression.");
            *tree = rootNode;
            return false;
        }
        tvOffset += 1;
        Token *currentToken = &tv->tokens[tvOffset];
        if (!strncmp(currentToken->tokenStr, ")", currentToken->tokenStrLength) ||
            !strncmp(currentToken->tokenStr, ";", currentToken->tokenStrLength))
        {
            *expressionEndIndex = tvOffset;
            *tree = rootNode;
            return true;
        }
        AstOperatorType currentTokenOpType = operatorTypeFromStr(currentToken->tokenStr, currentToken->tokenStrLength);
        tvOffset += 1;
        if (tvOffset + 1 >= tv->length)
        {
            puts("Unexpected end of expression.");
            *tree = rootNode;
            return false;
        }
        currentToken = &tv->tokens[tvOffset];

        AstNode *subTree = NULL;
        if (ast_check_subtree(tv, tvOffset))
        {
            result = ast_check_token(tv, &tvOffset, &subTree, tree, expressionEndIndex);
            if (!result) return result;
        }

        if (currentTokenOpType == ASTOPTYPE_INVALID)
        {
            puts("Operator type was invalid.");
            *tree = rootNode;
            return false;
        }
        AstNode *nextNode = calloc(1, sizeof(AstNode));
        nextNode->operator = currentTokenOpType;
        if (!strncmp(currentToken->tokenStr, "&", currentToken->tokenStrLength))
        {
            subTree = calloc(1, sizeof(AstNode));
            subTree->operator = ASTOPTYPE_REFERENCE;
            subTree->left = calloc(1, sizeof(AstNode));
            subTree->left->tokenValue = &tv->tokens[tvOffset + 1];
            tvOffset++;
        }
        if (!strncmp(currentToken->tokenStr, "*", currentToken->tokenStrLength))
        {
            subTree = calloc(1, sizeof(AstNode));
            subTree->operator = ASTOPTYPE_DEREFERENCE;
            subTree->left = calloc(1, sizeof(AstNode));
            subTree->left->tokenValue = &tv->tokens[tvOffset + 1];
            tvOffset++;
        }
        if (subTree == NULL)
        {
            nextNode->right = calloc(1, sizeof(AstNode));
            nextNode->right->tokenValue = currentToken;
        } else
        {
            nextNode->right = subTree;
        }

        AstNode *replacingNode = rootNode;
        AstNode *replacingNodeParent = NULL;
        //We're going to search the tree until we find the right place to insert this node based on operator
        //precedence.
        while (replacingNode)
        {
            if (replacingNode->isSubtree ||
                operatorPrecedence(currentTokenOpType) >= operatorPrecedence(replacingNode->operator))
                break;
            replacingNodeParent = replacingNode;
            replacingNode = replacingNode->right;
        }
        if (replacingNode == NULL)
        {
            //This shouldn't happen with a well-formed expression
            free(nextNode);
            *tree = rootNode;
            return false;
        }

        if (!replacingNodeParent)
        {
            nextNode->left = rootNode;
            rootNode = nextNode;
            continue;
        }

        nextNode->left = replacingNode;
        replacingNodeParent->right = nextNode;
    }
    *tree = rootNode;
    return false;
}
