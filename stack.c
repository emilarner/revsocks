#include "stack.h"

Stack *stack_init()
{
    Stack *s = (Stack*) malloc(sizeof(Stack));

    s->capacity = INITIAL_CAPACITY;
    s->length = 0;
    s->ptrs = (void**) malloc(INITIAL_CAPACITY * sizeof(void*));

    return s;
}

void stack_push(Stack *s, void *ptr)
{
    if (s->length >= s->capacity)
    {
        s->capacity *= 2;
        s->ptrs = (void**) realloc(s->ptrs, s->capacity * sizeof(void*));
    }

    s->ptrs[s->length] = ptr;
    s->length++;
}

void *stack_pop(Stack *s)
{
    return s->ptrs[s->length - 1];
}

void stack_free(Stack *s)
{
    free(s->ptrs);
    free(s);
}
