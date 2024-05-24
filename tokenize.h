#ifndef CCOMPILER_TOKENIZE_H
#define CCOMPILER_TOKENIZE_H
#include <stdbool.h>

typedef enum
{
    TT_INVALID,
    TT_STRING_LITERAL,
    TT_IDENTIFIER,
    TT_INT_LITERAL,
    TT_CHAR_LITERAL,
    TT_STORAGE_CLASS,
    TT_TYPE_SPECIFIER,
    TT_TYPE_QUALIFIER,
    TT_FUNC_SPECIFIER,
    TT_ALIGNMENT_SPECIFIER,
    TT_PUNCTUATOR,
    TT_KEYWORD
} TokenType;

typedef struct
{
    char *tokenStr;
    int tokenStrLength;
    TokenType tokenType;
    int fileRow;
} Token;

typedef struct
{
    Token *tokens;
    int length;
    int capacity;
} TokenVector;

extern void tokenVectorCreate(TokenVector *vector);
extern void tokenVectorDispose(TokenVector *vector);
extern Token *tokenVectorAt(TokenVector *tv, int index);

extern bool isIdentifierCharacter(char c, bool first);
extern void tokenize(TokenVector *vector, char* fileBuffer, int fileBufferLength);

extern const int G_STORAGE_CLASS_SPECIFIERS_COUNT;
extern const char *G_STORAGE_CLASS_SPECIFIERS[];
extern const int G_TYPE_SPECIFIERS_COUNT;
extern const char *G_TYPE_SPECIFIERS[];
extern const int G_TYPE_QUALIFIERS_COUNT;
extern const char *G_TYPE_QUALIFIERS[];
extern const int G_FUNCTION_SPECIFIERS_COUNT;
extern const char *G_FUNCTION_SPECIFIERS[];
extern const int G_PUNCTUATORS_COUNT;
extern const char* G_PUNCTUATORS[];
extern const int G_ALIGNMENT_SPECIFIERS_COUNT;
extern const char *G_ALIGNMENT_SPECIFIERS[];
extern const int G_KEYWORDS_COUNT;
extern const char *G_KEYWORDS[];
#endif //CCOMPILER_TOKENIZE_H
