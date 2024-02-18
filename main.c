#include <stdio.h>
#include <stdlib.h>
#include "tokenize.h"

TokenVector tokenVector;

typedef enum
{
    OPERATOR_MULTIPLY,
    OPERATOR_DIVIDE,
    OPERATOR_ADD,
    OPERATOR_SUBTRACT
} OperatorType;

typedef struct
{
    Token *left;
    Token *right;
    OperatorType operator;
} AstNode;

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
    fileBuffer[fileLength - 1] = 0;
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

    tokenVectorDispose(&tokenVector);
    free(fileBuffer);
}
