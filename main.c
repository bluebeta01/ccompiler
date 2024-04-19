#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tokenize.h"
#include "ast.h"

TokenVector tokenVector;

struct IrVariable
{
    int width;
    struct IrVariable *prev;
    struct IrVariable *next;
};
typedef struct IrVariable IrVariable;

void parseFunction(AstNode *astTree)
{

}

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
        ast_node_pretty_print(head);
    }
    else
    {
        puts("Failed to parse expression.");
    }
    ast_node_free_tree(head);

    tokenVectorDispose(&tokenVector);
    free(fileBuffer);
}
