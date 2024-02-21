#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tokenize.h"
#include "ast.h"

TokenVector tokenVector;

typedef enum
{
    INST_TYPE_INVALID,
    INST_TYPE_MOV_IMMEDIATE,
    INST_TYPE_MOV_INDIRECT,
    INST_TYPE_ADD_INDIRECT,
    INST_TYPE_ADD_IMMEDIATE,
    INST_TYPE_SUB_INDIRECT,
    INST_TYPE_MUL_INDIRECT,
    INST_TYPE_PUSH,
    INST_TYPE_POP,
} InstType;

typedef struct Inst Inst;
struct Inst
{
    InstType instType;
    Inst *prev;
    Inst *next;
};

struct InstSingleToken
{
    Inst inst;
    Token *token;
};

struct InstTwoRegisters
{
    Inst inst;
    int srcRegister;
    int dstRegister;
};

struct InstSingleRegister
{
    Inst inst;
    int registerNumber;
};

typedef struct InstSingleRegister InstPush;
typedef struct InstSingleRegister InstPop;
typedef struct InstSingleToken InstMovImmediate;
typedef struct InstTwoRegisters InstMovIndirect;
typedef struct InstTwoRegisters InstAddIndirect;
typedef struct InstTwoRegisters InstSubIndirect;
typedef struct InstTwoRegisters InstMulIndirect;
typedef struct InstSingleToken InstAddImmediate;


void parseExpressionRecursive(AstNode *ast, int *registerCount, Inst **lastInst)
{
    if(ast->left == NULL) return;
    if(ast->left->tokenValue == NULL)
        parseExpressionRecursive(ast->left, registerCount, lastInst);
    if(ast->left->tokenValue != NULL)
    {
        InstMovImmediate *instMovImm = calloc(1, sizeof(InstMovImmediate));
        instMovImm->token = ast->left->tokenValue;
        instMovImm->inst.instType = INST_TYPE_MOV_IMMEDIATE;
        if(*registerCount >= 3)
        {
            InstPush *instPush = calloc(1, sizeof(InstPush));
            instPush->registerNumber = 0;
            instPush->inst.instType = INST_TYPE_PUSH;
            instMovImm->inst.next = (Inst*)instPush;
        }
        else
        {
            InstMovIndirect *movIndirect = calloc(1, sizeof(InstMovIndirect));
            movIndirect->dstRegister = *registerCount + 1;
            movIndirect->srcRegister = 0;
            movIndirect->inst.instType = INST_TYPE_MOV_INDIRECT;
            instMovImm->inst.next = (Inst*)movIndirect;
        }
        (*registerCount)++;
        (*lastInst)->next = (Inst*)instMovImm;
        *lastInst = ((Inst*)instMovImm)->next;
    }
    if(ast->right && ast->right->tokenValue == NULL)
        parseExpressionRecursive(ast->right, registerCount, lastInst);
    if(ast->right && ast->right->tokenValue != NULL)
    {
        InstMovImmediate *instMovImm = calloc(1, sizeof(InstMovImmediate));
        instMovImm->token = ast->right->tokenValue;
        instMovImm->inst.instType = INST_TYPE_MOV_IMMEDIATE;
        if(*registerCount >= 3)
        {
            InstPush *instPush = calloc(1, sizeof(InstPush));
            instPush->registerNumber = 0;
            instPush->inst.instType = INST_TYPE_PUSH;
            instMovImm->inst.next = (Inst*)instPush;
        }
        else
        {
            InstMovIndirect *movIndirect = calloc(1, sizeof(InstMovIndirect));
            movIndirect->dstRegister = *registerCount + 1;
            movIndirect->srcRegister = 0;
            movIndirect->inst.instType = INST_TYPE_MOV_INDIRECT;
            instMovImm->inst.next = (Inst*)movIndirect;
        }
        (*registerCount)++;
        (*lastInst)->next = (Inst*)instMovImm;
        *lastInst = ((Inst*)instMovImm)->next;
    }
    if(ast->operator == ASTOPTYPE_ADD || ast->operator == ASTOPTYPE_SUBTRACT || ast->operator == ASTOPTYPE_MULTIPLY)
    {
        int srcRegister = *registerCount;
        int dstRegister = *registerCount - 1;
        if(*registerCount > 3)
        {
            srcRegister = 4;
            InstPop *instPop = calloc(1, sizeof(InstPop));
            instPop->registerNumber = 4;
            instPop->inst.instType = INST_TYPE_POP;
            (*lastInst)->next = (Inst*)instPop;
            *lastInst = (Inst*)instPop;
            srcRegister = 4;
        }
        if(*registerCount > 4)
        {
            dstRegister = 5;
            InstPop *instPop = calloc(1, sizeof(InstPop));
            instPop->registerNumber = 5;
            instPop->inst.instType = INST_TYPE_POP;
            (*lastInst)->next = (Inst*)instPop;
            *lastInst = (Inst*)instPop;
            dstRegister = 5;
        }
        struct InstTwoRegisters *addIndirect = calloc(1, sizeof(struct InstTwoRegisters));
        addIndirect->srcRegister = srcRegister;
        addIndirect->dstRegister = dstRegister;
        addIndirect->inst.instType = INST_TYPE_ADD_INDIRECT;
        if(ast->operator == ASTOPTYPE_SUBTRACT)
            addIndirect->inst.instType = INST_TYPE_SUB_INDIRECT;
        if(ast->operator == ASTOPTYPE_MULTIPLY)
            addIndirect->inst.instType = INST_TYPE_MUL_INDIRECT;
        (*lastInst)->next = (Inst*)addIndirect;
        *lastInst = (Inst*)addIndirect;
        *registerCount -= 1;
        if(*registerCount > 3)
        {
            InstPush *instPush = calloc(1, sizeof(InstPush));
            instPush->registerNumber = dstRegister;
            instPush->inst.instType = INST_TYPE_PUSH;
            (*lastInst)->next = (Inst*)instPush;
            *lastInst = (Inst*)instPush;
        }
    }
}

Inst *parseExpression(AstNode *ast)
{
    Inst head = {0};
    Inst *headPtr = &head;
    int regCount = 0;
    parseExpressionRecursive(ast, &regCount, &headPtr);
    return head.next;
}

void instChainPrettyPrint(Inst *chain)
{
    do
    {
        switch(chain->instType)
        {
            case INST_TYPE_PUSH:
            {
                InstPush *inst = (InstPush*)chain;
                printf("push r%i\n", inst->registerNumber);
                break;
            }
            case INST_TYPE_POP:
            {
                InstPop *inst = (InstPop*)chain;
                printf("pop r%i\n", inst->registerNumber);
                break;
            }
            case INST_TYPE_MOV_IMMEDIATE:
            {
                InstMovImmediate *inst = (InstMovImmediate*)chain;
                if(inst->token->tokenStrLength != 0)
                {
                    char *tokenBuffer = calloc(inst->token->tokenStrLength + 1, 1);
                    memcpy(tokenBuffer, inst->token->tokenStr, inst->token->tokenStrLength);
                    printf("movi #%s\n", tokenBuffer);
                    free(tokenBuffer);
                }
                break;
            }
            case INST_TYPE_MOV_INDIRECT:
            {
                InstMovIndirect *inst = (InstMovIndirect *)chain;
                printf("mov r%i, r%i\n", inst->dstRegister, inst->srcRegister);
                break;
            }
            case INST_TYPE_ADD_INDIRECT:
            {
                InstAddIndirect *inst = (InstAddIndirect *)chain;
                printf("add r%i, r%i\n", inst->dstRegister, inst->srcRegister);
                break;
            }
            case INST_TYPE_SUB_INDIRECT:
            {
                InstSubIndirect *inst = (InstSubIndirect *)chain;
                printf("sub r%i, r%i\n", inst->dstRegister, inst->srcRegister);
                break;
            }
            case INST_TYPE_MUL_INDIRECT:
            {
                InstMulIndirect *inst = (InstMulIndirect *)chain;
                printf("mul r%i, r%i\n", inst->dstRegister, inst->srcRegister);
                break;
            }
            default:
            {
                puts("Cannot print instruction. Instruction type not implemented.");
                break;
            }

        }
        chain = chain->next;
    }
    while(chain != NULL);
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
    Inst *instChain = NULL;
    if(result)
    {
        astPrettyPrintTree(head, 0);
        instChain = parseExpression(head);
    }
    else
    {
        puts("Failed to parse expression.");
    }
    instChainPrettyPrint(instChain);
    astFreeTree(head);

    tokenVectorDispose(&tokenVector);
    free(fileBuffer);
}
