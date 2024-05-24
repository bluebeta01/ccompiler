#ifndef CCOMPILER_VEC_H
#define CCOMPILER_VEC_H
#include <stdlib.h>

#define listDeclare(type, name) \
typedef struct                  \
{                               \
    int length;                 \
    int capacity;               \
    type *data;                 \
} name;                         \
extern void listPush##name( name *list, type value); \
extern type *listAt##name( name *list, int index); \
extern name listInit##name( int capacity); \

#define listDefine(type, name) \
void listPush##name( name *list, type value) \
{ \
    if(list->length == list->capacity) \
    { \
        type *newArray = malloc(sizeof(type) * (list->capacity * 2 + 1)); \
        memcpy(newArray, list->data, sizeof(type) * list->capacity); \
        free(list->data); \
        list->data = newArray; \
        list->capacity = list->capacity * 2 + 1; \
    } \
    list->data[list->length] = value; \
    list->length++; \
}                              \
type *listAt##name( name *list, int index)    \
{                              \
    if(index >= list->length) return NULL;   \
    return &list->data[index];                               \
} \
name listInit##name( int capacity) \
{ \
    name list = {0}; \
    list.capacity = capacity; \
    list.data = malloc(sizeof(type) * capacity);  \
    return list; \
}
#endif //CCOMPILER_VEC_H
