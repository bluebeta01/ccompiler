#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "ast.h"

static AstOperatorType operatorTypeFromStr(const char *str, int strLength)
{
    if(!strncmp(str, "*", strLength)) return ASTOPTYPE_MULTIPLY;
    if(!strncmp(str, "/", strLength)) return ASTOPTYPE_DIVIDE;
    if(!strncmp(str, "+", strLength)) return ASTOPTYPE_ADD;
    if(!strncmp(str, "-", strLength)) return ASTOPTYPE_SUBTRACT;
    if(!strncmp(str, ".", strLength)) return ASTOPTYPE_DOT;
    if(!strncmp(str, ",", strLength)) return ASTOPTYPE_COMMA;
    return ASTOPTYPE_INVALID;
}


static int find_closing_paren(TokenVector *tv, int tvOffset)
{
    int counter = 0;
    for(int i = tvOffset; i < tv->length; i++)
    {
        Token *token = &tv->tokens[i];
        if(!strncmp(token->tokenStr, "(", token->tokenStrLength))
        {
            counter++;
            continue;
        }
        if(!strncmp(token->tokenStr, ")", token->tokenStrLength))
        {
            counter--;
            if(counter == 0)
                return i;
            continue;
        }
    }
    return -1;
}

void ast_tree_to_list(AstNode *ast, AstNode **head, AstNode **tail)
{
    if(ast->left)
        ast_tree_to_list(ast->left, head, tail);
    if(ast->right)
        ast_tree_to_list(ast->right, head, tail);

    if(!(*tail) && ast->tokenValue)
    {
        ast->left = NULL;
        ast->right = NULL;
        *tail = ast;
        *head = ast;
        return;
    }

    if(*tail)
    {
        (*tail)->right = ast;
        ast->left = *tail;
        ast->right = NULL;
        *tail = ast;
    }
}

void ast_node_pretty_print(AstNode *head)
{
    if(!head) return;
    while(head)
    {
        if(head->operator == ASTOPTYPE_ADD)
            puts("+");
        if(head->operator == ASTOPTYPE_SUBTRACT)
            puts("s");
        if(head->operator == ASTOPTYPE_MULTIPLY)
            puts("*");
        if(head->operator == ASTOPTYPE_DIVIDE)
            puts("/");
        if(head->operator == ASTOPTYPE_DOT)
            puts("dot");
        if(head->operator == ASTOPTYPE_COMMA)
            puts("comma");
        if(head->operator == ASTOPTYPE_CALL)
            puts("call");
        if(head->operator == ASTOPTYPE_REFERENCE)
            puts("REF");
        if(head->operator == ASTOPTYPE_DEREFERENCE)
            puts("DEREF");
        if(head->operator == ASTOPTYPE_INVALID)
        {
            if(head->tokenValue == NULL)
            {
                puts("Node had operator invalid and no token value.");
                return;
            }
            for(int i = 0; i < head->tokenValue->tokenStrLength; i++)
            {
                putc(head->tokenValue->tokenStr[i], stdout);
            }
            puts("");
        }
        head = head->right;
    }
}

void ast_node_free_tree(AstNode *head)
{
    if(!head)return;
    while(head != NULL)
    {
        AstNode *next = head->right;
        free(head);
        head = next;
    }
}

static bool ast_check_subtree(TokenVector *tv, int tvOffset)
{
    Token *currentToken = &tv->tokens[tvOffset];
    if(!strncmp(currentToken->tokenStr, "(", currentToken->tokenStrLength))
        return true;
    if(tvOffset + 2 <= tv->length)
    {
        Token *nextToken = &tv->tokens[tvOffset + 1];
        if (!strncmp(nextToken->tokenStr, "(", nextToken->tokenStrLength))
            return true;
    }
    return false;
}

static bool ast_check_token(TokenVector *tv, int *tvOffset, AstNode **rootNode, AstNode **tree)
{
    Token *firstToken = &tv->tokens[*tvOffset];
    if(!strncmp(firstToken->tokenStr, "(", firstToken->tokenStrLength))
    {
        int closingParenIndex = find_closing_paren(tv, *tvOffset);
        if(closingParenIndex == -1)
        {
            puts("Invalid expression. Could not find the closing paren.");
            *tree = *rootNode;
            return false;
        }
        bool result = ast(tv, (*tvOffset) + 1, rootNode);
        if(!result)
        {
            *tree = *rootNode;
            return result;
        }
        *tvOffset = closingParenIndex;

        return true;
    }
    if(!strncmp(firstToken->tokenStr, "&", firstToken->tokenStrLength))
    {
        *rootNode = (AstNode*)calloc(1, sizeof(AstNode));
        (*rootNode)->operator = ASTOPTYPE_REFERENCE;
        (*rootNode)->left = calloc(1, sizeof(AstNode));
        (*rootNode)->left->tokenValue = &tv->tokens[(*tvOffset) + 1];
        *tree = *rootNode;
        *tvOffset += 1;

        return true;
    }
    if(!strncmp(firstToken->tokenStr, "*", firstToken->tokenStrLength))
    {
        *tvOffset += 1;
        AstNode *subTree = NULL;
        bool result = ast_check_token(tv, tvOffset, &subTree, tree);
        if(!result) return result;

        *rootNode = (AstNode*)calloc(1, sizeof(AstNode));
        (*rootNode)->operator = ASTOPTYPE_DEREFERENCE;
        *tree = *rootNode;

        if(!subTree)
        {
            (*rootNode)->left = calloc(1, sizeof(AstNode));
            (*rootNode)->left->tokenValue = &tv->tokens[*tvOffset];
        }
        else
        {
            (*rootNode)->left = subTree;
        }

        return true;
    }
    if((*tvOffset) + 2 <= tv->length)
    {
        Token* nextToken = &tv->tokens[(*tvOffset) + 1];
        if(!strncmp(nextToken->tokenStr, "(", nextToken->tokenStrLength))
        {
            int funcCallEndIndex = find_closing_paren(tv, (*tvOffset) + 1);
            if(funcCallEndIndex == -1)
            {
                puts("Could not parse function call.");
                *tree = *rootNode;
                return false;
            }
            AstNode *funcParamsTree = NULL;
            if(funcCallEndIndex > (*tvOffset) + 2)
            {
                bool result = ast(tv, (*tvOffset) + 2, &funcParamsTree);
                if(!result)
                {
                    *tree = *rootNode;
                    return result;
                }
            }
            if(funcParamsTree && funcParamsTree->operator != ASTOPTYPE_COMMA)
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

bool ast(TokenVector *tv, int tvOffset, AstNode **tree)
{
    AstNode *rootNode = NULL;
    AstNode *prevNode = NULL;
    Token *firstToken = &tv->tokens[tvOffset];
    bool firstNode = true;
    bool result = false;

    result = ast_check_token(tv, &tvOffset, &rootNode, tree);
    if(!result) return result;

    if(rootNode == NULL)
    {
        rootNode = calloc(1, sizeof(AstNode));
        rootNode->tokenValue = firstToken;
    }
    prevNode = rootNode;

    while(tvOffset < tv->length - 1)
    {
        if(tvOffset + 1 >= tv->length )
        {
            puts("Unexpected end of expression.");
            *tree = rootNode;
            return false;
        }
        tvOffset += 1;
        Token *currentToken = &tv->tokens[tvOffset];
        if(!strncmp(currentToken->tokenStr, ")", currentToken->tokenStrLength) ||
           !strncmp(currentToken->tokenStr, ";", currentToken->tokenStrLength))
        {
            *tree = rootNode;
            return true;
        }
        AstOperatorType currentTokenOpType = operatorTypeFromStr(currentToken->tokenStr, currentToken->tokenStrLength);
        tvOffset += 1;
        if(tvOffset + 1 >= tv->length )
        {
            puts("Unexpected end of expression.");
            *tree = rootNode;
            return false;
        }
        currentToken = &tv->tokens[tvOffset];

        AstNode *subTree = NULL;
        if(ast_check_subtree(tv, tvOffset))
        {
            result = ast_check_token(tv, &tvOffset, &subTree, tree);
            if(!result) return result;
        }

        if(currentTokenOpType == ASTOPTYPE_INVALID)
        {
            puts("Operator type was invalid.");
            *tree = rootNode;
            return false;
        }
        AstNode *nextNode = calloc(1, sizeof(AstNode));
        nextNode->operator = currentTokenOpType;
        if(!strncmp(currentToken->tokenStr, "&", currentToken->tokenStrLength))
        {
            subTree = calloc(1, sizeof(AstNode));
            subTree->operator = ASTOPTYPE_REFERENCE;
            subTree->left = calloc(1, sizeof(AstNode));
            subTree->left->tokenValue = &tv->tokens[tvOffset + 1];
            tvOffset++;
        }
        if(!strncmp(currentToken->tokenStr, "*", currentToken->tokenStrLength))
        {
            subTree = calloc(1, sizeof(AstNode));
            subTree->operator = ASTOPTYPE_DEREFERENCE;
            subTree->left = calloc(1, sizeof(AstNode));
            subTree->left->tokenValue = &tv->tokens[tvOffset + 1];
            tvOffset++;
        }
        if(currentTokenOpType == ASTOPTYPE_ADD || currentTokenOpType == ASTOPTYPE_SUBTRACT || firstNode)
        {
            firstNode = false;
            nextNode->left = rootNode;
            if(subTree == NULL)
            {
                nextNode->right = calloc(1, sizeof(AstNode));
                nextNode->right->tokenValue = currentToken;
            }
            else
            {
                nextNode->right = subTree;
            }
            rootNode = nextNode;
            prevNode = nextNode;
            continue;
        }
        if(currentTokenOpType == ASTOPTYPE_MULTIPLY || currentTokenOpType == ASTOPTYPE_DIVIDE)
        {
            if(subTree == NULL)
            {
                nextNode->right = calloc(1, sizeof(AstNode));
                nextNode->right->tokenValue = currentToken;
            }
            else
            {
                nextNode->right = subTree;
            }
            if(rootNode->operator == ASTOPTYPE_ADD || rootNode->operator == ASTOPTYPE_SUBTRACT)
            {
                nextNode->left = rootNode->right;
                rootNode->right = nextNode;
                prevNode = nextNode;
            }
            else
            {
                nextNode->left = rootNode;
                rootNode = nextNode;
            }
            continue;
        }
        if(currentTokenOpType == ASTOPTYPE_DOT)
        {
            nextNode->left = rootNode;
            nextNode->right = calloc(1, sizeof(AstNode));
            nextNode->right->tokenValue = currentToken;
            rootNode = nextNode;
            continue;
        }
        if(currentTokenOpType == ASTOPTYPE_COMMA)
        {
            AstNode *commaSub = NULL;
            if(subTree == NULL)
            {
                result = ast(tv, tvOffset, &commaSub);
                if(!result)
                {
                    free(nextNode);
                    ast_node_free_tree(commaSub);
                    *tree = rootNode;
                    return result;
                }
                nextNode->left = commaSub;
            }
            else
            {
                nextNode->left = subTree;
            }
            nextNode->right = rootNode;
            *tree = nextNode;
            return true;
        }
    }
    *tree = rootNode;
    return true;
}
