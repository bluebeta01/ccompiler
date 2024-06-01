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
CodeVariableList codeVarList;

bool r3Used = false;
bool r4Used = false;

//Returns an integer representing the next available register, or 0 if none.
//Reserves the register and requires caller to call freeRegister when no longer in use.
int useRegister()
{
    if(!r3Used)
    {
        r3Used = true;
        return 3;
    }
    if(!r4Used)
    {
        r4Used = true;
        return 4;
    }
    return 0;
}
void freeRegister(int registerNumber)
{
    switch (registerNumber)
    {
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
    IT_MUL,
    IT_SUBI,
    IT_ADC,
    IT_ADDI,
    IT_MOV,
    IT_MOVI,
    IT_LHI,
    IT_ORI,
    IT_PUSH,
    IT_LDR,
    IT_STR,
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
typedef struct InstructionRegReg MulInstruction;
typedef struct InstructionRegReg PushInstruction;
typedef struct InstructionRegReg LdrInstruction;
typedef struct InstructionRegReg StrInstruction;
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
    int pointerCount;
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
    if(value->pointerCount > 0)
    {
        if(value->isBpRelative)
        {
            MovInstruction *mov = malloc(sizeof(MovInstruction));
            mov->instruction.type = IT_MOV;
            mov->srcReg = ISA_BP_REGISTER;
            mov->dstReg = 0;
            listPushInstructionPtrList(&instructions, (Instruction*)mov);

            SubiInstruction *subi = malloc(sizeof(SubiInstruction));
            subi->instruction.type = IT_SUBI;
            subi->iValue = value->bpRelativeAddress;
            listPushInstructionPtrList(&instructions, (Instruction*)subi);
        }
        if(value->isRegister)
        {
            MovInstruction *mov = malloc(sizeof(MovInstruction));
            mov->instruction.type = IT_MOV;
            mov->srcReg = value->registerNumber;
            mov->dstReg = 0;
            listPushInstructionPtrList(&instructions, (Instruction*)mov);
        }

        LdrInstruction *ldr = malloc(sizeof(LdrInstruction));
        ldr->instruction.type = IT_LDR;
        ldr->srcReg = 0;
        ldr->dstReg = 0;
        listPushInstructionPtrList(&instructions, (Instruction*)ldr);

        int remainingCount = value->pointerCount - 1;
        for(int i = 0; i < remainingCount; i++)
        {
            ldr = malloc(sizeof(LdrInstruction));
            ldr->instruction.type = IT_LDR;
            ldr->srcReg = 0;
            ldr->dstReg = 0;
            listPushInstructionPtrList(&instructions, (Instruction*)ldr);
        }

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

        if(hiValue != 0)
        {
            LhiInstruction *lhi = malloc(sizeof(LhiInstruction));
            lhi->instruction.type = IT_LHI;
            lhi->iValue = hiValue;
            listPushInstructionPtrList(&instructions, (Instruction*)lhi);

            OriInstruction *ori = malloc(sizeof(OriInstruction));
            ori->instruction.type = IT_ORI;
            ori->iValue = loValue;
            listPushInstructionPtrList(&instructions, (Instruction*)ori);
        }
        else
        {
            MoviInstruction *movi = malloc(sizeof(MoviInstruction));
            movi->instruction.type = IT_MOVI;
            movi->iValue = loValue;
            listPushInstructionPtrList(&instructions, (Instruction*)movi);
        }

        MovInstruction *mov = malloc(sizeof(MovInstruction));
        mov->instruction.type = IT_MOV;
        mov->srcReg = 0;
        mov->dstReg = registerNumber;
        listPushInstructionPtrList(&instructions, (Instruction*)mov);

        return;
    }
}

AstNodeValue compileDeref(AstNodeValue *leftValue)
{
    AstNodeValue v = *leftValue;
    v.pointerCount++;
    return v;
}

AstNodeValue compileAdd(AstNodeValue *leftValue, AstNodeValue *rightValue)
{
    moveValueToRegister(leftValue, 0, 1);
    moveValueToRegister(rightValue, 0, 2);
    AddInstruction *add = malloc(sizeof(AddInstruction));
    add->instruction.type = IT_ADD;
    add->dstReg = 1;
    add->srcReg = 2;
    listPushInstructionPtrList(&instructions, (Instruction*)add);
    PushInstruction *push = malloc(sizeof(PushInstruction));
    push->instruction.type = IT_PUSH;
    push->srcReg = 1;
    listPushInstructionPtrList(&instructions, (Instruction*)push);
    AstNodeValue value = {0};
    value.width = 1;
    value.isSigned = leftValue->isSigned;
    value.isBpRelative = true;
    value.bpRelativeAddress = stackSize;
    stackSize++;
    return value;
}

AstNodeValue compileSubtract(AstNodeValue *leftValue, AstNodeValue *rightValue)
{
    moveValueToRegister(leftValue, 0, 1);
    moveValueToRegister(rightValue, 0, 2);
    SubInstruction *sub = malloc(sizeof(SubInstruction));
    sub->instruction.type = IT_SUB;
    sub->dstReg = 1;
    sub->srcReg = 2;
    listPushInstructionPtrList(&instructions, (Instruction*)sub);
    PushInstruction *push = malloc(sizeof(PushInstruction));
    push->instruction.type = IT_PUSH;
    push->srcReg = 1;
    listPushInstructionPtrList(&instructions, (Instruction*)push);
    AstNodeValue value = {0};
    value.width = 1;
    value.isSigned = leftValue->isSigned;
    value.isBpRelative = true;
    value.bpRelativeAddress = stackSize;
    stackSize++;
    return value;
}

AstNodeValue compileMultiply(AstNodeValue *leftValue, AstNodeValue *rightValue)
{
    moveValueToRegister(leftValue, 0, 1);
    moveValueToRegister(rightValue, 0, 2);
    MulInstruction *mul = malloc(sizeof(MulInstruction));
    mul->instruction.type = IT_MUL;
    mul->dstReg = 1;
    mul->srcReg = 2;
    listPushInstructionPtrList(&instructions, (Instruction*)mul);
    PushInstruction *push = malloc(sizeof(PushInstruction));
    push->instruction.type = IT_PUSH;
    push->srcReg = 1;
    listPushInstructionPtrList(&instructions, (Instruction*)push);
    AstNodeValue value = {0};
    value.width = 1;
    value.isSigned = leftValue->isSigned;
    value.isBpRelative = true;
    value.bpRelativeAddress = stackSize;
    stackSize++;
    return value;
}

AstNodeValue compileAssignment(AstNodeValue *leftValue, AstNodeValue *rightValue)
{
    moveValueToRegister(rightValue, 0, 2);

    if(leftValue->pointerCount > 0)
    {
        if(leftValue->isRegister)
        {
            MovInstruction *mov = malloc(sizeof(MovInstruction));
            mov->instruction.type = IT_MOV;
            mov->srcReg = leftValue->registerNumber;
            mov->dstReg = 1;
            listPushInstructionPtrList(&instructions, (Instruction*)mov);
        }
        if(leftValue->isBpRelative)
        {
            MovInstruction *mov = malloc(sizeof(MovInstruction));
            mov->instruction.type = IT_MOV;
            mov->srcReg = ISA_BP_REGISTER;
            mov->dstReg = 0;
            listPushInstructionPtrList(&instructions, (Instruction*)mov);

            SubiInstruction *subi = malloc(sizeof(SubiInstruction));
            subi->instruction.type = IT_SUBI;
            subi->iValue = leftValue->bpRelativeAddress;
            listPushInstructionPtrList(&instructions, (Instruction*)subi);

            LdrInstruction *ldr = malloc(sizeof(LdrInstruction));
            ldr->instruction.type = IT_LDR;
            ldr->srcReg = 0;
            ldr->dstReg = 1;
            listPushInstructionPtrList(&instructions, (Instruction*)ldr);
        }
        int remainingCount = leftValue->pointerCount - 1;
        for(int i = 0; i < remainingCount; i++)
        {
            LdrInstruction *ldr = malloc(sizeof(LdrInstruction));
            ldr->instruction.type = IT_LDR;
            ldr->srcReg = 1;
            ldr->dstReg = 1;
            listPushInstructionPtrList(&instructions, (Instruction*)ldr);
        }

        StrInstruction *str = malloc(sizeof(StrInstruction));
        str->instruction.type = IT_STR;
        str->srcReg = 2;
        str->dstReg = 1;
        listPushInstructionPtrList(&instructions, (Instruction*)str);

        AstNodeValue v = *leftValue;
        return v;
    }
    if(leftValue->isRegister)
    {
        MovInstruction *mov = malloc(sizeof(MovInstruction));
        mov->instruction.type = IT_MOV;
        mov->srcReg = 2;
        mov->dstReg = leftValue->registerNumber;
        listPushInstructionPtrList(&instructions, (Instruction*)mov);

        AstNodeValue v = *leftValue;
        return v;
    }
    if(leftValue->isBpRelative)
    {
        MovInstruction *mov = malloc(sizeof(MovInstruction));
        mov->instruction.type = IT_MOV;
        mov->srcReg = ISA_BP_REGISTER;
        mov->dstReg = 0;
        listPushInstructionPtrList(&instructions, (Instruction*)mov);

        SubiInstruction *subi = malloc(sizeof(SubiInstruction));
        subi->instruction.type = IT_SUBI;
        subi->iValue = leftValue->bpRelativeAddress;
        listPushInstructionPtrList(&instructions, (Instruction*)subi);

        StrInstruction *str = malloc(sizeof(StrInstruction));
        str->instruction.type = IT_STR;
        str->srcReg = 2;
        str->dstReg = 0;
        listPushInstructionPtrList(&instructions, (Instruction*)str);
    }

    //This should never happen
    AstNodeValue v = *leftValue;
    return v;
}

AstNodeValue compileExpression(AstNode *ast)
{
    AstNodeValue leftValue;
    AstNodeValue rightValue;
    if(ast->left)
        leftValue = compileExpression(ast->left);
    if(ast->right)
        rightValue = compileExpression(ast->right);

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
        if(ast->tokenValue->tokenType == TT_IDENTIFIER)
        {
            CodeVariable *cv = NULL;
            for(int i = 0; i < codeVarList.length; i++)
            {
                cv = listAtCodeVariableList(&codeVarList, i);
                int maxLength = cv->identifierNameLength;
                if(ast->tokenValue->tokenStrLength > maxLength)
                    maxLength = ast->tokenValue->tokenStrLength;
                if(!strncmp(cv->identifierName, ast->tokenValue->tokenStr, maxLength))
                    break;
            }
            if(!cv)
            {
                puts("Could not find identifier");
                return (AstNodeValue){0};
            }
            AstNodeValue v = {0};
            v.isSigned = cv->isSigned;
            v.width = 1;
            v.isBpRelative = !cv->inRegister;
            v.isRegister = cv->inRegister;
            v.bpRelativeAddress = cv->bpRelativeAddress;
            v.registerNumber = cv->registerNumber;
            return v;
        }
        puts("Non operator type was countered while compiling expression that cannot be handled.");
        return (AstNodeValue){0};
    }

    switch(ast->operator)
    {
        case ASTOPTYPE_ADD:
        {
            return compileAdd(&leftValue, &rightValue);
        }
        case ASTOPTYPE_SUBTRACT:
        {
            return compileSubtract(&leftValue, &rightValue);
        }
        case ASTOPTYPE_MULTIPLY:
        {
            return compileMultiply(&leftValue, &rightValue);
        }
        case ASTOPTYPE_DEREFERENCE:
        {
            return compileDeref(&leftValue);
        }
        case ASTOPTYPE_EQUALS:
        {
            return compileAssignment(&leftValue, &rightValue);
        }
        default:
        {
            puts("An unhandled operator type was encountered while compiling expression.");
            return (AstNodeValue){0};
        }
    }
}

//Parses a variable definition and returns the token vector index of the last token in the definition
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
        if(identifierToken->tokenType != TT_IDENTIFIER)
            continue;
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
        cv.bpRelativeAddress = stackSize;
        stackSize += cv.width;
        PushInstruction *push = malloc(sizeof(PushInstruction));
        push->instruction.type = IT_PUSH;
        push->srcReg = 0;
        listPushInstructionPtrList(&instructions, (Instruction*)push);
    }
    listPushCodeVariableList(&codeVarList, cv);
    tokenIndex++;
    Token *nextToken = tokenVectorAt(&tokenVector, tokenIndex);
    if(nextToken == NULL)
        return tokenIndex;
    if(!strncmp(";", nextToken->tokenStr, nextToken->tokenStrLength))
        return tokenIndex;

    puts("Unexpected end of definition.");
    return tokenIndex;
}

int parseExpression(int start)
{
    AstNode *tree = NULL;
    int endIndex = 0;
    bool success = ast(&tokenVector, start, &tree, &endIndex);
    if(!success)
    {
        free(tree);
        puts("Failed to parse expression. Cannot compile.");
        return start + 1;
    }
    compileExpression(tree);
    ast_node_pretty_print(tree);
    ast_node_free_tree(tree);
    return endIndex;
}

void parseFunction()
{
    for(int i = 0; i < tokenVector.length; i++)
    {
        Token *iToken = &tokenVector.tokens[i];
        if(iToken->tokenType == TT_TYPE_SPECIFIER)
        {
            i = parseDefinition(i);
            continue;
        }
        if(iToken->tokenType == TT_IDENTIFIER || !strncmp("*", iToken->tokenStr, iToken->tokenStrLength))
        {
            i = parseExpression(i);
            continue;
        }
    }
}

void print_instructions()
{
    for(int i = 0; i < instructions.length; i++)
    {
        Instruction *ins = *listAtInstructionPtrList(&instructions, i);
        switch(ins->type)
        {
            case IT_ADD:
                printf("add r%d, r%d\n", ((AddInstruction*)ins)->dstReg, ((AddInstruction*)ins)->srcReg);
                break;
            case IT_SUB:
                printf("sub r%d, r%d\n", ((SubInstruction*)ins)->dstReg, ((SubInstruction*)ins)->srcReg);
                break;
            case IT_MUL:
                printf("mul r%d, r%d\n", ((MulInstruction*)ins)->dstReg, ((MulInstruction*)ins)->srcReg);
                break;
            case IT_MOV:
                printf("mov r%d, r%d\n", ((MovInstruction*)ins)->dstReg, ((MovInstruction*)ins)->srcReg);
                break;
            case IT_LDR:
                printf("ldr r%d, r%d\n", ((LdrInstruction *)ins)->dstReg, ((LdrInstruction*)ins)->srcReg);
                break;
            case IT_STR:
                printf("str r%d, r%d\n", ((StrInstruction *)ins)->dstReg, ((StrInstruction*)ins)->srcReg);
                break;
            case IT_PUSH:
                printf("push r%d\n", ((PushInstruction *)ins)->srcReg);
                break;
            case IT_SUBI:
                printf("subi #%lld\n", ((SubiInstruction*)ins)->iValue);
                break;
            case IT_MOVI:
                printf("movi #%lld\n", ((MoviInstruction*)ins)->iValue);
                break;
            case IT_LHI:
                printf("mhi #%lld\n", ((LhiInstruction*)ins)->iValue);
                break;
            case IT_ORI:
                printf("ori #%lld\n", ((OriInstruction*)ins)->iValue);
                break;
            default:
                puts("INVALID INSTRUCTION");
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
    codeVarList = listInitCodeVariableList(10);
    tokenVectorCreate(&tokenVector);
    tokenize(&tokenVector, fileBuffer, fileLength);

    parseFunction();
    print_instructions();

    tokenVectorDispose(&tokenVector);
    free(fileBuffer);
}
