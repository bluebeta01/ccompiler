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
    OPERATOR_SUBTRACT
} OperatorType;

OperatorType operatorTypeFromStr(const char *str, int strLength)
{
    if(!strncmp(str, "*", strLength)) return OPERATOR_MULTIPLY;
    if(!strncmp(str, "/", strLength)) return OPERATOR_DIVIDE;
    if(!strncmp(str, "+", strLength)) return OPERATOR_ADD;
    if(!strncmp(str, "-", strLength)) return OPERATOR_SUBTRACT;
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

AstNode *headNode = NULL;

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

AstNode *ast(TokenVector *tv, int tvOffset)
{
    AstNode *rootNode = NULL;
    rootNode = calloc(1, sizeof(AstNode));
    rootNode->tokenValue = &tv->tokens[tvOffset];

    while(tvOffset < tv->length - 1)
    {
        if(tvOffset + 2 >= tv->length ) {
            puts("Unexpected end of expression.");
            return rootNode;
        }
        tvOffset += 1;
        Token *currentToken = &tv->tokens[tvOffset];
        OperatorType currentTokenOpType = operatorTypeFromStr(currentToken->tokenStr, currentToken->tokenStrLength);
        tvOffset += 1;
        currentToken = &tv->tokens[tvOffset];

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
            nextNode->right = calloc(1, sizeof(AstNode));
            nextNode->right->tokenValue = currentToken;
            rootNode = nextNode;
        }
        else
        {
            if(currentTokenOpType == OPERATOR_MULTIPLY || currentTokenOpType == OPERATOR_DIVIDE)
            {
                nextNode->right = calloc(1, sizeof(AstNode));
                nextNode->right->tokenValue = currentToken;
                nextNode->left = rootNode->right;
                rootNode->right = nextNode;
            }
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

    tokenVectorDispose(&tokenVector);
    free(fileBuffer);
}
