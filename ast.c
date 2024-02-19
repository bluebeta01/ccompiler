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


static int findClosingParen(TokenVector *tv, int tvOffset)
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

static int findCommaBefore(TokenVector *tv, int tvOffset, int beforeIndex)
{
    int counter = tvOffset;
    while(counter <= tv->length && counter < beforeIndex)
    {
        Token *token = &tv->tokens[counter];
        if(!strncmp(token->tokenStr, ",", token->tokenStrLength))
            return counter;
        counter++;
    }
    return -1;
}

void astPrettyPrintTree(AstNode *head, int tickCount)
{
    if(!head) return;
    if(head->left)
        astPrettyPrintTree(head->left, tickCount + 1);
    if(head->right)
        astPrettyPrintTree(head->right, tickCount + 1);
    for(int i = 0; i < tickCount; i++)
        putc('-', stdout);
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
}

void astFreeTree(AstNode *head)
{
    if(!head)return;
    if(head->left)
        astFreeTree(head->left);
    if(head->right)
        astFreeTree(head->right);
    free(head);
}

bool ast(TokenVector *tv, int tvOffset, AstNode **tree)
{
    AstNode *rootNode = NULL;
    AstNode *prevNode = NULL;
    Token *firstToken = &tv->tokens[tvOffset];
    bool firstNode = true;
    bool result = false;

    if(!strncmp(firstToken->tokenStr, "(", firstToken->tokenStrLength))
    {
        int closingParenIndex = findClosingParen(tv, tvOffset);
        if(closingParenIndex == -1)
        {
            puts("Invalid expression. Could not find the closing paren.");
            *tree = rootNode;
            return false;
        }
        result = ast(tv, tvOffset + 1, &rootNode);
        if(!result)
        {
            *tree = rootNode;
            return result;
        }
        tvOffset = closingParenIndex;
    }
    else
    {
        if(tvOffset + 2 <= tv->length)
        {
            Token* nextToken = &tv->tokens[tvOffset + 1];
            if(!strncmp(nextToken->tokenStr, "(", nextToken->tokenStrLength))
            {
                int funcCallEndIndex = findClosingParen(tv, tvOffset + 1);
                if(funcCallEndIndex == -1)
                {
                    puts("Could not parse function call.");
                    *tree = rootNode;
                    return false;
                }
                AstNode *funcParamsTree = NULL;
                if(funcCallEndIndex > tvOffset + 2)
                {
                    result = ast(tv, tvOffset + 2, &funcParamsTree);
                    if(!result)
                    {
                        *tree = rootNode;
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
                funcCallNode->left = funcParamsTree;
                rootNode = funcCallNode;
                tvOffset = funcCallEndIndex;
            }
        }
    }
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
        if(!strncmp(currentToken->tokenStr, "(", currentToken->tokenStrLength))
        {
            int closingParenIndex = findClosingParen(tv, tvOffset);
            if(closingParenIndex == -1)
            {
                puts("Invalid expression. Could not find the closing paren.");
                *tree = rootNode;
                return false;
            }
            result = ast(tv, tvOffset + 1, &subTree);
            if(!result)
            {
                *tree = rootNode;
                return result;
            }
            tvOffset = closingParenIndex;
        }
        else
        {
            if(tvOffset + 2 <= tv->length)
            {
                Token* nextToken = &tv->tokens[tvOffset + 1];
                if(!strncmp(nextToken->tokenStr, "(", nextToken->tokenStrLength))
                {
                    int funcCallEndIndex = findClosingParen(tv, tvOffset + 1);
                    if(funcCallEndIndex == -1)
                    {
                        puts("Could not parse function call.");
                        *tree = rootNode;
                        return false;
                    }
                    AstNode *funcParamsTree = NULL;
                    if(funcCallEndIndex > tvOffset + 2)
                    {
                        result = ast(tv, tvOffset + 2, &funcParamsTree);
                        if(!result)
                        {
                            *tree = rootNode;
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
                    funcCallNode->left = funcParamsTree;
                    subTree = funcCallNode;
                    tvOffset = funcCallEndIndex;
                }
            }
        }

        if(currentTokenOpType == ASTOPTYPE_INVALID)
        {
            puts("Operator type was invalid.");
            *tree = rootNode;
            return false;
        }
        AstNode *nextNode = calloc(1, sizeof(AstNode));
        nextNode->operator = currentTokenOpType;
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
            nextNode->left = rootNode->right;
            rootNode->right = nextNode;
            prevNode = nextNode;
            continue;
        }
        if(currentTokenOpType == ASTOPTYPE_DOT)
        {
            nextNode->left = prevNode->right;
            nextNode->right = calloc(1, sizeof(AstNode));
            nextNode->right->tokenValue = currentToken;
            prevNode->right = nextNode;
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
                    astFreeTree(commaSub);
                    *tree = rootNode;
                    return result;
                }
                nextNode->right = commaSub;
            }
            else
            {
                nextNode->right = subTree;
            }
            nextNode->left = rootNode;
            *tree = nextNode;
            return true;
        }
    }
    *tree = rootNode;
    return true;
}
