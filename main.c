#include <stdio.h>
#include <stdlib.h>
#include "tokenize.h"
#include "ast.h"

TokenVector tokenVector;

int main() {
    FILE *file = fopen("C:/code/junk/sampleExpression.c", "rb");
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

    AstNode *head = NULL;
    bool result = ast(&tokenVector, 0, &head);
    if(result)
    {
        astPrettyPrintTree(head, 0);
    }
    else
    {
        puts("Failed to parse expression.");
    }
    astFreeTree(head);

    tokenVectorDispose(&tokenVector);
    free(fileBuffer);
}
