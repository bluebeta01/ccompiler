#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tokenize.h"

TokenVector tokenVector;

typedef enum
{
    OPERATOR_INVALID,
    OPERATOR_MULTIPLY,
    OPERATOR_DIVIDE,
    OPERATOR_ADD,
    OPERATOR_SUBTRACT,
    OPERATOR_DOT,
    OPERATOR_COMMA,
    OPERATOR_CALL
} OperatorType;

OperatorType operatorTypeFromStr(const char *str, int strLength)
{
    if(!strncmp(str, "*", strLength)) return OPERATOR_MULTIPLY;
    if(!strncmp(str, "/", strLength)) return OPERATOR_DIVIDE;
    if(!strncmp(str, "+", strLength)) return OPERATOR_ADD;
    if(!strncmp(str, "-", strLength)) return OPERATOR_SUBTRACT;
    if(!strncmp(str, ".", strLength)) return OPERATOR_DOT;
    if(!strncmp(str, ",", strLength)) return OPERATOR_COMMA;
    return OPERATOR_INVALID;
}

typedef struct AstNode AstNode;
struct AstNode
{
    AstNode *left;
    AstNode *right;
    Token *tokenValue;
    OperatorType operator;
};

int findClosingParen(TokenVector *tv, int tvOffset)
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

int findCommaBefore(TokenVector *tv, int tvOffset, int beforeIndex)
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
                AstNode *funcParamsTree = NULL;
                result = ast(tv, tvOffset + 2, &funcParamsTree);
                if(!result)
                {
                    *tree = rootNode;
                    return result;
                }
                int funcCallEndIndex = findClosingParen(tv, tvOffset + 1);
                if(funcParamsTree == NULL || funcCallEndIndex == -1)
                {
                    puts("Could not parse function call.");
                    *tree = rootNode;
                    return false;
                }
                if(funcParamsTree->operator != OPERATOR_COMMA)
                {
                    AstNode *commaNode = calloc(1, sizeof(AstNode));
                    commaNode->operator = OPERATOR_COMMA;
                    commaNode->left = funcParamsTree;
                    funcParamsTree = commaNode;
                }
                AstNode *funcCallNode = calloc(1, sizeof(AstNode));
                funcCallNode->operator = OPERATOR_CALL;
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
        OperatorType currentTokenOpType = operatorTypeFromStr(currentToken->tokenStr, currentToken->tokenStrLength);
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
                    AstNode *funcParamsTree = NULL;
                    result = ast(tv, tvOffset + 2, &funcParamsTree);
                    if(!result)
                    {
                        *tree = rootNode;
                        return result;
                    }
                    int funcCallEndIndex = findClosingParen(tv, tvOffset + 1);
                    if(funcParamsTree == NULL || funcCallEndIndex == -1)
                    {
                        puts("Could not parse function call.");
                        *tree = rootNode;
                        return false;
                    }
                    if(funcParamsTree->operator != OPERATOR_COMMA)
                    {
                        AstNode *commaNode = calloc(1, sizeof(AstNode));
                        commaNode->operator = OPERATOR_COMMA;
                        commaNode->left = funcParamsTree;
                        funcParamsTree = commaNode;
                    }
                    AstNode *funcCallNode = calloc(1, sizeof(AstNode));
                    funcCallNode->operator = OPERATOR_CALL;
                    funcCallNode->left = funcParamsTree;
                    subTree = funcCallNode;
                    tvOffset = funcCallEndIndex;
                }
            }
        }

        if(currentTokenOpType == OPERATOR_INVALID)
        {
            puts("Operator type was invalid.");
            *tree = rootNode;
            return false;
        }
        AstNode *nextNode = calloc(1, sizeof(AstNode));
        nextNode->operator = currentTokenOpType;
        if(currentTokenOpType == OPERATOR_ADD || currentTokenOpType == OPERATOR_SUBTRACT || firstNode)
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
        if(currentTokenOpType == OPERATOR_MULTIPLY || currentTokenOpType == OPERATOR_DIVIDE)
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
        if(currentTokenOpType == OPERATOR_DOT)
        {
            nextNode->left = prevNode->right;
            nextNode->right = calloc(1, sizeof(AstNode));
            nextNode->right->tokenValue = currentToken;
            prevNode->right = nextNode;
            continue;
        }
        if(currentTokenOpType == OPERATOR_COMMA)
        {
            AstNode *commaSub = NULL;
            result = ast(tv, tvOffset, &commaSub);
            if(!result)
            {
                free(nextNode);
                astFreeTree(commaSub);
                *tree = rootNode;
                return result;
            }
            nextNode->left = rootNode;
            nextNode->right = commaSub;
            *tree = nextNode;
            return true;
        }
    }
    *tree = rootNode;
    return true;
}

void prettyPrint(AstNode *head, int tickCount)
{
    if(!head) return;
    for(int i = 0; i < tickCount; i++)
        putc('-', stdout);
    if(head->operator == OPERATOR_ADD)
        puts("+");
    if(head->operator == OPERATOR_SUBTRACT)
        puts("s");
    if(head->operator == OPERATOR_MULTIPLY)
        puts("*");
    if(head->operator == OPERATOR_DIVIDE)
        puts("/");
    if(head->operator == OPERATOR_DOT)
        puts("dot");
    if(head->operator == OPERATOR_COMMA)
        puts("comma");
    if(head->operator == OPERATOR_CALL)
        puts("call");
    if(head->operator == OPERATOR_INVALID)
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
    if(head->left)
        prettyPrint(head->left, tickCount + 1);
    if(head->right)
        prettyPrint(head->right, tickCount + 1);
}

int main() {
    FILE *file = fopen("C:/code/junk/sampleExpression.c", "r");
    if(!file)
    {
        puts("Failed to open file!");
        return 1;
    }
    fseek(file, 0, SEEK_END);
    long fileLength = ftell(file);
    fseek(file, 0, SEEK_SET);
    if(fileLength == 0)
    {
        fclose(file);
        puts("File is length 0");
        return 1;
    }
    char *fileBuffer = (char*)malloc(fileLength + 1);
    fread(fileBuffer, 1, fileLength, file);
    fileBuffer[fileLength] = 0;
    fclose(file);

    tokenVectorCreate(&tokenVector);
    tokenize(&tokenVector, fileBuffer, fileLength);
    for(int i = 0; i < tokenVector.length; i++)
    {
        const Token *token = &tokenVector.tokens[i];
        for(int j = 0; j < token->tokenStrLength; j++)
        {
            putc(token->tokenStr[j], stdout);
        }
        putc('\n', stdout);
    }

    AstNode *head = NULL;
    bool result = ast(&tokenVector, 0, &head);
    if(result)
    {
        prettyPrint(head, 0);
    }
    else
    {
        puts("Failed to parse expression.");
    }
    astFreeTree(head);

    tokenVectorDispose(&tokenVector);
    free(fileBuffer);
}
