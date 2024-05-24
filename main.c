#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "tokenize.h"
#include "ast.h"
#include "vec.h"

#define ISA_SP_REGISTER 8
#define ISA_BP_REGISTER 5

TokenVector tokenVector;

struct CodeVariable
{
    char *identifierName;
    int identifierNameLength;
    int width;
    int registerNumber;
    int bpRelativeAddress;
    int scope;
    bool inRegister;
    bool isSigned;
    bool isPointer;
};
typedef struct CodeVariable CodeVariable;
listDeclare(CodeVariable , CodeVariableList);
listDefine(CodeVariable , CodeVariableList);

bool r1Used = false;
bool r2Used = false;
bool r3Used = false;
bool r4Used = false;

//Returns an integer representing the next available register, or 0 if none.
//Reserves the register and requires caller to call freeRegister when no longer in use.
int useRegister()
{
    if(!r1Used) return 1;
    if(!r2Used) return 2;
    if(!r3Used) return 3;
    if(!r4Used) return 4;
    return 0;
}
void freeRegister(int registerNumber)
{
    switch (registerNumber)
    {
        case 1:
        {
            r1Used = false;
            return;
        }
        case 2:
        {
            r2Used = false;
            return;
        }
        case 3:
        {
            r3Used = false;
            return;
        }
        case 4:
        {
            r4Used = false;
            return;
        }
        default:
            return;
    }
}

int stackSize = 0;

enum InstructionType
{
    IT_ADD,
    IT_SUB,
    IT_SUBI,
    IT_ADC,
    IT_ADDI,
    IT_MOV,
    IT_MOVI,
    IT_LHI,
    IT_ORI,
    IT_PUSH,
    IT_LDR,
};

struct Instruction
{
    enum InstructionType type;
};
typedef struct Instruction Instruction;
struct InstructionRegReg
{
    struct Instruction instruction;
    int srcReg;
    int dstReg;
};
struct InstructionImm
{
    struct Instruction instruction;
    long long iValue;
};
typedef struct InstructionRegReg MovInstruction;
typedef struct InstructionRegReg AddInstruction;
typedef struct InstructionRegReg AdcInstruction;
typedef struct InstructionRegReg SubInstruction;
typedef struct InstructionRegReg PushInstruction;
typedef struct InstructionRegReg LdrInstruction;
typedef struct InstructionImm AddiInstruction;
typedef struct InstructionImm SubiInstruction;
typedef struct InstructionImm MoviInstruction;
typedef struct InstructionImm LhiInstruction;
typedef struct InstructionImm OriInstruction;
listDeclare(Instruction*, InstructionPtrList);
listDefine(Instruction*, InstructionPtrList);

InstructionPtrList instructions;

typedef struct
{
    bool isRegister;
    bool isBpRelative;
    bool isIntegerLiteral;
    bool isSigned;
    int width;
    int registerNumber;
    int bpRelativeAddress;
    long long integerLiteral;
} AstNodeValue;

/*
Moves the specified zero-based word index of the provided value into the register number specified.
If the wordIndex is greater than the width of the value, 0xFFFF is loaded into the register if the value is signed,
otherwise 0 is loaded.
*/
void moveValueToRegister(AstNodeValue *value, int wordIndex, int registerNumber)
{
    if(wordIndex >= value->width && value->isSigned)
    {
        LhiInstruction *lhi = malloc(sizeof(LhiInstruction));
        lhi->instruction.type = IT_LHI;
        lhi->iValue = 0xFF;
        listPushInstructionPtrList(&instructions, (Instruction*)lhi);

        OriInstruction *ori = malloc(sizeof(OriInstruction));
        ori->instruction.type = IT_ORI;
        ori->iValue = 0xFF;
        listPushInstructionPtrList(&instructions, (Instruction*)ori);

        MovInstruction *mov = malloc(sizeof(MovInstruction));
        mov->instruction.type = IT_MOV;
        mov->srcReg = 0;
        mov->dstReg = registerNumber;
        listPushInstructionPtrList(&instructions, (Instruction*)mov);

        return;
    }
    if(wordIndex >= value->width && !value->isSigned)
    {
        MoviInstruction *movi = malloc(sizeof(MoviInstruction));
        movi->instruction.type = IT_MOVI;
        movi->iValue = 0;
        listPushInstructionPtrList(&instructions, (Instruction*)movi);

        MovInstruction *mov = malloc(sizeof(MovInstruction));
        mov->instruction.type = IT_MOV;
        mov->srcReg = 0;
        mov->dstReg = registerNumber;
        listPushInstructionPtrList(&instructions, (Instruction*)mov);

        return;
    }
    if(value->isRegister)
    {
        if(value->registerNumber == registerNumber) return;
        MovInstruction *mov = malloc(sizeof(MovInstruction));
        mov->instruction.type = IT_MOV;
        mov->srcReg = value->registerNumber;
        mov->dstReg = registerNumber;
        listPushInstructionPtrList(&instructions, (Instruction*)mov);
        return;
    }
    if(value->isBpRelative)
    {
        MovInstruction *mov = malloc(sizeof(MovInstruction));
        mov->instruction.type = IT_MOV;
        mov->srcReg = ISA_BP_REGISTER;
        mov->dstReg = 0;
        listPushInstructionPtrList(&instructions, (Instruction*)mov);

        SubiInstruction *subi = malloc(sizeof(SubiInstruction));
        subi->instruction.type = IT_SUBI;
        subi->iValue = value->bpRelativeAddress + value->width - 1 - wordIndex;
        listPushInstructionPtrList(&instructions, (Instruction*)subi);

        LdrInstruction *ldr = malloc(sizeof(LdrInstruction));
        ldr->instruction.type = IT_LDR;
        ldr->srcReg = 0;
        ldr->dstReg = registerNumber;
        listPushInstructionPtrList(&instructions, (Instruction*)ldr);

        return;
    }
    if(value->isIntegerLiteral)
    {
        long long hiValue = (value->integerLiteral >> (16 * wordIndex + 8)) & 0xFF;
        long long loValue = (value->integerLiteral >> (16 * wordIndex)) & 0xFF;

        LhiInstruction *lhi = malloc(sizeof(LhiInstruction));
        lhi->instruction.type = IT_MOVI;
        lhi->iValue = hiValue;
        listPushInstructionPtrList(&instructions, (Instruction*)lhi);

        OriInstruction *ori = malloc(sizeof(OriInstruction));
        ori->instruction.type = IT_ORI;
        ori->iValue = loValue;
        listPushInstructionPtrList(&instructions, (Instruction*)ori);

        MovInstruction *mov = malloc(sizeof(MovInstruction));
        mov->instruction.type = IT_MOV;
        mov->srcReg = 0;
        mov->dstReg = registerNumber;
        listPushInstructionPtrList(&instructions, (Instruction*)mov);

        return;
    }
}

AstNodeValue compileAdd(AstNodeValue *leftValue, AstNodeValue *rightValue, int dstReg)
{
    int resultWidth = leftValue->width;
    if(rightValue->width > resultWidth)
        resultWidth = rightValue->width;

    for(int i = 0; i < resultWidth; i++)
    {
        moveValueToRegister(leftValue, i, 1);
        moveValueToRegister(rightValue, i, 2);
    }
}

AstNodeValue compileExpression(AstNode *ast, bool left)
{
    AstNodeValue leftValue;
    AstNodeValue rightValue;
    if(ast->left)
        leftValue = compileExpression(ast->left, true);
    if(ast->right)
        rightValue = compileExpression(ast->right, false);

    if(ast->operator == ASTOPTYPE_INVALID)
    {
        if(ast->tokenValue->tokenType == TT_INT_LITERAL)
        {
            char *endPtr = ast->tokenValue->tokenStr;
            AstNodeValue value = {0};
            value.isIntegerLiteral = true;
            value.integerLiteral = strtoll(ast->tokenValue->tokenStr, &endPtr, 10);
            value.width = 1;
            return value;
        }
        puts("Non operator type was countered while compiling expression that cannot be handled.");
        return (AstNodeValue){0};
    }

    if(ast->operator == ASTOPTYPE_ADD)
    {
        return compileAdd(&leftValue, &rightValue);
    }

    puts("An unhandled operator type was encountered while compiling expression.");
    return (AstNodeValue){0};
}

//Parses a variable definition and returns the token vector index of the last token in the definition + 1
//Returns -1 on failure
int parseDefinition(int start)
{
    bool pointer = false;
    Token *identifierToken = NULL;
    int tokenIndex = 0;
    for(int i = start; i < tokenVector.length; i++)
    {
        tokenIndex = i;
        identifierToken = tokenVectorAt(&tokenVector, i);
        if(!identifierToken) return -1;
        if (!strncmp("(", identifierToken->tokenStr, identifierToken->tokenStrLength))
            continue;
        if (!strncmp("*", identifierToken->tokenStr, identifierToken->tokenStrLength))
        {
            pointer = true;
            continue;
        }
        break;
    }
    if(!identifierToken) return -1;
    CodeVariable cv = {0};
    cv.identifierName = identifierToken->tokenStr;
    cv.identifierNameLength = identifierToken->tokenStrLength;
    cv.width = 1;
    int reg = useRegister();
    if(reg)
    {
        cv.registerNumber = reg;
        cv.inRegister = true;
    }
    else
    {
        stackSize += cv.width;
        cv.bpRelativeAddress = -stackSize - 1;
        //TODO: Emit instructions to increase stack size
    }
    tokenIndex++;
    Token *nextToken = tokenVectorAt(&tokenVector, tokenIndex);
    if(nextToken == NULL)
        return tokenIndex;
    if(strncmp("=", nextToken->tokenStr, nextToken->tokenStrLength))
        return tokenIndex;

    //TODO: Parse expression and emit instructions
}

void parseFunction()
{
    for(int i = 0; i < tokenVector.length; i++)
    {
        Token *iToken = &tokenVector.tokens[i];
        if(iToken->tokenType == TT_TYPE_SPECIFIER)
        {
            parseDefinition(i);
        }
    }
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

    instructions = listInitInstructionPtrList(10);
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
