#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include "value.h"
#include "talloc.h"

Value *pointers;
bool initialized = false;

//Duplicated code from linkedlist.c

// Create a new NULL_TYPE value node.
Value *makeNullNEW() {
  Value *v = malloc(sizeof(Value));
  v -> type = NULL_TYPE;
  return v;
}

// Create a new CONS_TYPE value node.
Value *consNEW(Value *newCar, Value *newCdr) {
  Value *v = malloc(sizeof(Value));
  v->type = CONS_TYPE;
  (v->c).car = newCar;
  (v->c).cdr = newCdr;
  return v;
}

// Utility to make it less typing to get car value. Use assertions to make sure
// that this is a legitimate operation.
Value *carNEW(Value *list) {
  return (list->c).car;
}

// Utility to make it less typing to get cdr value. Use assertions to make sure
// that this is a legitimate operation.
Value *cdrNEW(Value *list) {
  return (list->c).cdr;
}

void displayNEW(Value *list) {
  Value *curVal = list;

  while (curVal -> type != NULL_TYPE) {
    if (carNEW(curVal) -> type == INT_TYPE || carNEW(curVal) -> type == BOOL_TYPE) {
      printf("%i\n", carNEW(curVal)->i);
    }
    else if (carNEW(curVal) -> type == DOUBLE_TYPE) {
      printf("%g\n", carNEW(curVal)->d);
    }
    else if (carNEW(curVal) -> type == STR_TYPE || carNEW(curVal) -> type == SYMBOL_TYPE) {
      printf("%s\n", carNEW(curVal)->s);
    }
    else if (carNEW(curVal) -> type == PTR_TYPE) {
      printf("%p\n", carNEW(curVal)->p);
    }
    
    curVal = cdrNEW(curVal);
  }
}

// Replacement for malloc that stores the pointers allocated. It should store
// the pointers in some kind of list; a linked list would do fine, but insert
// here whatever code you'll need to do so; don't call functions in the
// pre-existing linkedlist.h. Otherwise you'll end up with circular
// dependencies, since you're going to modify the linked list to use talloc.
void *talloc(size_t size) {
  
  if (!initialized) {
    pointers = makeNullNEW();
    initialized = true;
  }

  Value *pointer = malloc(sizeof(Value));
  pointer->type = PTR_TYPE;
  pointer->p = malloc(size);

  pointers = consNEW(pointer, pointers);
 
  return pointer->p;
}

// Free all pointers allocated by talloc, as well as whatever memory you
// allocated in lists to hold those pointers.
void tfree() {
  if (!initialized)
    return;
  Value *curVal = pointers;
  Value *nextVal;
  while (curVal->type != NULL_TYPE) {
    nextVal = cdrNEW(curVal);
    free(carNEW(curVal)->p);
    free(carNEW(curVal));
    free(curVal);
    curVal = nextVal;
  }
  free(curVal);
  initialized = false;
}

// Replacement for the C function "exit", that consists of two lines: it calls
// tfree before calling exit. It's useful to have later on; if an error happens,
// you can exit your program, and all memory is automatically cleaned up.
void texit(int status) {
  tfree();
  exit(status);
}
