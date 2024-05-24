#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "tokenize.h"

const int G_STORAGE_CLASS_SPECIFIERS_COUNT = 6;
const char *G_STORAGE_CLASS_SPECIFIERS[] = {
        "typedef",
        "extern",
        "static",
        "_Thread_local",
        "auto",
        "register",
};

const int G_TYPE_SPECIFIERS_COUNT = 11;
const char *G_TYPE_SPECIFIERS[] = {
        "void",
        "char",
        "short",
        "int",
        "long",
        "float",
        "double",
        "signed",
        "unsigned",
        "_Bool",
        "_Complex",
};

const int G_TYPE_QUALIFIERS_COUNT = 4;
const char *G_TYPE_QUALIFIERS[] = {
        "const",
        "restrict",
        "volatile",
        "_Atomic"
};

const int G_FUNCTION_SPECIFIERS_COUNT = 2;
const char *G_FUNCTION_SPECIFIERS[] = {
        "inline",
        "_Noreturn"
};

const int G_PUNCTUATORS_COUNT = 48;
const char* G_PUNCTUATORS[] = {
        "[",
        "]",
        "(",
        ")",
        "{",
        "}",
        "...",
        "<<=",
        ">>=",
        "->",
        "++",
        "--",
        "<<",
        ">>",
        "<=",
        ">=",
        "==",
        "!=",
        "&&",
        "||",
        "*=",
        "/=",
        "%=",
        "+=",
        "-=",
        "&=",
        "^=",
        "|=",
        "##",
        ",",
        "#",
        ".",
        "&",
        "*",
        "+",
        "-",
        "~",
        "!",
        "/",
        "%",
        "<",
        "<",
        "^",
        "|",
        "?",
        ":",
        ";",
        "=",
};

const int G_ALIGNMENT_SPECIFIERS_COUNT = 1;
const char *G_ALIGNMENT_SPECIFIERS[] = {
        "_Alignas"
};

const int G_KEYWORDS_COUNT = 18;
const char *G_KEYWORDS[] = {
        "alignof",
        "break",
        "case",
        "continue",
        "default",
        "do",
        "else",
        "enum",
        "for",
        "goto",
        "if",
        "register",
        "return",
        "sizeof",
        "struct",
        "switch",
        "union",
        "while"
};

bool isIdentifierCharacter(char c, bool first)
{
    if(c == 95) return true;
    if(c <= 57 && c >= 48 && !first) return true;
    if(c <= 90 && c >= 65) return true;
    if(c <= 122 && c >= 97) return true;
    return false;
}

void tokenVectorCreate(TokenVector *vector)
{
    TokenVector tv = {0};
    tv.capacity = 100;
    tv.tokens = (Token*)malloc(sizeof(Token) * tv.capacity);
    *vector = tv;
}

void tokenVectorDispose(TokenVector *vector)
{
    free(vector->tokens);
}

static void tokenVectorPush(TokenVector *vector, const Token *token)
{
    if(vector->length >= vector->capacity)
    {
        Token *newArray = (Token*)malloc(sizeof(Token) * vector->capacity * 2 + sizeof(Token));
        memcpy(newArray, vector->tokens, sizeof(Token) * vector->capacity);
        free(vector->tokens);
        vector->tokens = newArray;
        vector->capacity = vector->capacity * 2 + 1;
    }
    vector->tokens[vector->length] = *token;
    vector->length++;
}

//Checks if str (NULL TERMINATED) starts with any one of the strings in strArray.
//Returns the string that matched if it is found, NULL otherwise.
static const char *startsWithOneOf(char *str, const char *strArray[], size_t strArrayLength)
{
    for(int i = 0; i < strArrayLength; i++)
    {
        const char *compareStr = strArray[i];
        if(!strncmp(str, compareStr, strlen(compareStr))) return compareStr;
    }
    return NULL;
}

static int stringLiteralLength(const char *fileBuffer, int fileBufferOffset, int fileBufferLength)
{
    if(*(fileBuffer + fileBufferOffset) != '"') return 0;
    int length = 1;
    fileBufferOffset++;
    bool escape = false;
    while(fileBufferOffset < fileBufferLength)
    {
        if(escape)
        {
            escape = false;
            length++;
            fileBufferOffset++;
            continue;
        }
        char c = *(fileBuffer + fileBufferOffset);
        if(c == '\\')
        {
            escape = true;
            length++;
            fileBufferOffset++;
            continue;
        }
        if(c != '"')
        {
            length++;
            fileBufferOffset++;
            continue;
        }
        length++;
        break;
    }
    return length;
}

static int charLiteralLength(const char *fileBuffer, int fileBufferOffset, int fileBufferLength)
{
    if(*(fileBuffer + fileBufferOffset) != '\'') return 0;
    int length = 1;
    fileBufferOffset++;
    bool escape = false;
    while(fileBufferOffset < fileBufferLength)
    {
        if(escape)
        {
            escape = false;
            length++;
            fileBufferOffset++;
            continue;
        }
        char c = *(fileBuffer + fileBufferOffset);
        if(c == '\\')
        {
            escape = true;
            length++;
            fileBufferOffset++;
            continue;
        }
        if(c != '\'')
        {
            length++;
            fileBufferOffset++;
            continue;
        }
        length++;
        break;
    }
    return length;
}

static int intLiteralLength(const char *fileBuffer, int fileBufferOffset, int fileBufferLength)
{
    char c = *(fileBuffer + fileBufferOffset);
    if(c != '-' && !isdigit(c)) return 0;
    int length = 1;
    fileBufferOffset++;
    while(fileBufferOffset < fileBufferLength)
    {
        c = *(fileBuffer + fileBufferOffset);
        if(!isdigit(c))
            return length;
        length++;
        fileBufferOffset++;
    }
    return length;
}

static int identifierLength(const char *fileBuffer, int fileBufferOffset, int fileBufferLength)
{
    int length = 0;
    if(!isIdentifierCharacter(*(fileBuffer + fileBufferOffset), true))
    {
        return length;
    }
    length++;
    fileBufferOffset++;

    while(fileBufferOffset < fileBufferLength)
    {
        char currentChar = *(fileBuffer + fileBufferOffset);
        if(isIdentifierCharacter(currentChar, false))
        {
            length++;
            fileBufferOffset++;
            continue;
        }
        break;
    }
    return length;
}

void tokenize(TokenVector *vector, char* fileBuffer, int fileBufferLength)
{
    int fileBufferOffset = 0;
    int fileLineCount = 1;
    while(fileBufferOffset < fileBufferLength)
    {
        char *tokenStrPtr = fileBuffer + fileBufferOffset;
        if(*tokenStrPtr == '\n') fileLineCount++;
        if(isspace(*tokenStrPtr))
        {
            fileBufferOffset++;
            continue;
        }
        Token token = {0};
        token.fileRow = fileLineCount;
        const char *matchedStr = startsWithOneOf(tokenStrPtr, G_STORAGE_CLASS_SPECIFIERS, G_STORAGE_CLASS_SPECIFIERS_COUNT);
        if(matchedStr)
        {
            int matchedStrLength = (int)strlen(matchedStr);
            if(fileBufferOffset + matchedStrLength >= fileBufferLength || !isIdentifierCharacter(*(tokenStrPtr + matchedStrLength), false))
            {
                token.tokenStr = tokenStrPtr;
                token.tokenStrLength = matchedStrLength;
                token.tokenType = TT_STORAGE_CLASS;

                fileBufferOffset += matchedStrLength;
                tokenVectorPush(vector, &token);
                continue;
            }
        }

        matchedStr = startsWithOneOf(tokenStrPtr, G_TYPE_SPECIFIERS, G_TYPE_SPECIFIERS_COUNT);
        if(matchedStr)
        {
            int matchedStrLength = (int)strlen(matchedStr);
            if(fileBufferOffset + matchedStrLength >= fileBufferLength || !isIdentifierCharacter(*(tokenStrPtr + matchedStrLength), false))
            {
                token.tokenStr = tokenStrPtr;
                token.tokenStrLength = matchedStrLength;
                token.tokenType = TT_TYPE_SPECIFIER;

                fileBufferOffset += matchedStrLength;
                tokenVectorPush(vector, &token);
                continue;
            }
        }

        matchedStr = startsWithOneOf(tokenStrPtr, G_TYPE_QUALIFIERS, G_TYPE_QUALIFIERS_COUNT);
        if(matchedStr)
        {
            int matchedStrLength = (int)strlen(matchedStr);
            if(fileBufferOffset + matchedStrLength >= fileBufferLength || !isIdentifierCharacter(*(tokenStrPtr + matchedStrLength), false))
            {
                token.tokenStr = tokenStrPtr;
                token.tokenStrLength = matchedStrLength;
                token.tokenType = TT_TYPE_QUALIFIER;

                fileBufferOffset += matchedStrLength;
                tokenVectorPush(vector, &token);
                continue;
            }
        }

        matchedStr = startsWithOneOf(tokenStrPtr, G_FUNCTION_SPECIFIERS, G_FUNCTION_SPECIFIERS_COUNT);
        if(matchedStr)
        {
            int matchedStrLength = (int)strlen(matchedStr);
            if(fileBufferOffset + matchedStrLength >= fileBufferLength || !isIdentifierCharacter(*(tokenStrPtr + matchedStrLength), false))
            {
                token.tokenStr = tokenStrPtr;
                token.tokenStrLength = matchedStrLength;
                token.tokenType = TT_FUNC_SPECIFIER;

                fileBufferOffset += matchedStrLength;
                tokenVectorPush(vector, &token);
                continue;
            }
        }

        matchedStr = startsWithOneOf(tokenStrPtr, G_ALIGNMENT_SPECIFIERS, G_ALIGNMENT_SPECIFIERS_COUNT);
        if(matchedStr)
        {
            int matchedStrLength = (int)strlen(matchedStr);
            if(fileBufferOffset + matchedStrLength >= fileBufferLength || !isIdentifierCharacter(*(tokenStrPtr + matchedStrLength), false))
            {
                token.tokenStr = tokenStrPtr;
                token.tokenStrLength = matchedStrLength;
                token.tokenType = TT_ALIGNMENT_SPECIFIER;

                fileBufferOffset += matchedStrLength;
                tokenVectorPush(vector, &token);
                continue;
            }
        }

        matchedStr = startsWithOneOf(tokenStrPtr, G_PUNCTUATORS, G_PUNCTUATORS_COUNT);
        if(matchedStr)
        {
            int matchedStrLength = (int)strlen(matchedStr);
            token.tokenStr = tokenStrPtr;
            token.tokenStrLength = matchedStrLength;
            token.tokenType = TT_PUNCTUATOR;

            fileBufferOffset += matchedStrLength;
            tokenVectorPush(vector, &token);
            continue;
        }

        matchedStr = startsWithOneOf(tokenStrPtr, G_KEYWORDS, G_KEYWORDS_COUNT);
        if(matchedStr)
        {
            int matchedStrLength = (int)strlen(matchedStr);
            token.tokenStr = tokenStrPtr;
            token.tokenStrLength = matchedStrLength;
            token.tokenType = TT_KEYWORD;

            fileBufferOffset += matchedStrLength;
            tokenVectorPush(vector, &token);
            continue;
        }

        int foundLength = stringLiteralLength(fileBuffer, fileBufferOffset, fileBufferLength);
        if(foundLength)
        {
            token.tokenStr = fileBuffer + fileBufferOffset;
            token.tokenStrLength = foundLength;
            token.tokenType = TT_STRING_LITERAL;

            fileBufferOffset += foundLength;
            tokenVectorPush(vector, &token);
            continue;
        }

        foundLength = charLiteralLength(fileBuffer, fileBufferOffset, fileBufferLength);
        if(foundLength)
        {
            token.tokenStr = fileBuffer + fileBufferOffset;
            token.tokenStrLength = foundLength;
            token.tokenType = TT_CHAR_LITERAL;

            fileBufferOffset += foundLength;
            tokenVectorPush(vector, &token);
            continue;
        }

        foundLength = intLiteralLength(fileBuffer, fileBufferOffset, fileBufferLength);
        if(foundLength)
        {
            token.tokenStr = fileBuffer + fileBufferOffset;
            token.tokenStrLength = foundLength;
            token.tokenType = TT_INT_LITERAL;

            fileBufferOffset += foundLength;
            tokenVectorPush(vector, &token);
            continue;
        }

        foundLength = identifierLength(fileBuffer, fileBufferOffset, fileBufferLength);
        if(foundLength)
        {
            token.tokenStr = fileBuffer + fileBufferOffset;
            token.tokenStrLength = foundLength;
            token.tokenType = TT_IDENTIFIER;

            fileBufferOffset += foundLength;
            tokenVectorPush(vector, &token);
            continue;
        }

        printf("Unable to parse token on line %i\n", fileLineCount);
        return;
    }
}

Token *tokenVectorAt(TokenVector *tv, int index)
{
    if(index >= tv->length)
        return NULL;
    return &tv->tokens[index];
}

