#ifndef STACK_H
#define STACK_H

#define INITIAL_CAPACITY 12

#include <stdlib.h>
#include <stdint.h>


/* Extremely generic LIFO Stack object. Dereference any pointers at your own discretion! */
/* In this program, it is used to store fds. Dereferencing the void* returned will crash! */
struct Stack
{
    void **ptrs;
    size_t capacity;
    size_t length;
};

typedef struct Stack Stack;

Stack *stack_init();
void stack_push(Stack *s, void *ptr);
void *stack_pop(Stack *s);
void stack_free(Stack *s);



#endif 