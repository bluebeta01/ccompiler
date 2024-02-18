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
    OPERATOR_DOT
} OperatorType;

OperatorType operatorTypeFromStr(const char *str, int strLength)
{
    if(!strncmp(str, "*", strLength)) return OPERATOR_MULTIPLY;
    if(!strncmp(str, "/", strLength)) return OPERATOR_DIVIDE;
    if(!strncmp(str, "+", strLength)) return OPERATOR_ADD;
    if(!strncmp(str, "-", strLength)) return OPERATOR_SUBTRACT;
    if(!strncmp(str, ".", strLength)) return OPERATOR_DOT;
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

void astFreeTree(AstNode *head)
{
    if(head->left)
        astFreeTree(head->left);
    if(head->right)
        astFreeTree(head->right);
    free(head);
}

AstNode *ast(TokenVector *tv, int tvOffset)
{
    AstNode *rootNode = NULL;
    AstNode *prevNode = NULL;
    rootNode = calloc(1, sizeof(AstNode));
    rootNode->tokenValue = &tv->tokens[tvOffset];
    prevNode = rootNode;

    while(tvOffset < tv->length - 1)
    {
        if(tvOffset + 1 >= tv->length )
        {
            puts("Unexpected end of expression.");
            return rootNode;
        }
        tvOffset += 1;
        Token *currentToken = &tv->tokens[tvOffset];
        if(!strncmp(currentToken->tokenStr, ")", currentToken->tokenStrLength) || !strncmp(currentToken->tokenStr, ";", currentToken->tokenStrLength))
            return rootNode;
        OperatorType currentTokenOpType = operatorTypeFromStr(currentToken->tokenStr, currentToken->tokenStrLength);
        tvOffset += 1;
        if(tvOffset + 1 >= tv->length )
        {
            puts("Unexpected end of expression.");
            return rootNode;
        }
        currentToken = &tv->tokens[tvOffset];

        AstNode *subTree = NULL;
        if(!strncmp(currentToken->tokenStr, "(", currentToken->tokenStrLength))
        {
            int closingParenIndex = findClosingParen(tv, tvOffset);
            if(closingParenIndex == -1)
            {
                puts("Invalid expression. Could not find the closing paren.");
                return rootNode;
            }
            subTree = ast(tv, tvOffset + 1);
            tvOffset = closingParenIndex;
        }

        if(currentTokenOpType == OPERATOR_INVALID)
        {
            puts("Operator type was invalid.");
            return rootNode;
        }
        AstNode *nextNode = calloc(1, sizeof(AstNode));
        nextNode->operator = currentTokenOpType;
        if(currentTokenOpType == OPERATOR_ADD || currentTokenOpType == OPERATOR_SUBTRACT || rootNode->tokenValue != NULL)
        {
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
    }
    return rootNode;
}

void prettyPrint(AstNode *head, int tickCount)
{
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

    AstNode *head = ast(&tokenVector, 0);
    prettyPrint(head, 0);
    astFreeTree(head);

    tokenVectorDispose(&tokenVector);
    free(fileBuffer);
}
