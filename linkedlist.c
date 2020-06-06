#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "linkedlist.h"
#include "talloc.h"

// Create a new NULL_TYPE value node.
Value *makeNull() {
  Value *v = talloc(sizeof(Value));
  v -> type = NULL_TYPE;
  return v;
}

// Create a new CONS_TYPE value node.
Value *cons(Value *newCar, Value *newCdr) {
  Value *v = talloc(sizeof(Value));
  v->type = CONS_TYPE;
  (v->c).car = newCar;
  (v->c).cdr = newCdr;
  return v;
}

// Display the contents of the linked list to the screen in some kind of
// readable format
void display(Value *list) {
  Value *curVal = list;

  while (curVal -> type != NULL_TYPE) {
    if (car(curVal) -> type == INT_TYPE) {
      printf("%i", car(curVal)->i);
    }
    else if (car(curVal) -> type == BOOL_TYPE) {
      if (car(curVal)->i == 0) {
        printf("#f");
      }
      else {
        printf("#t");
      }
    }
    else if (car(curVal) -> type == DOUBLE_TYPE) {
      printf("%g", car(curVal)->d);
    }
    else if (car(curVal) -> type == STR_TYPE || car(curVal) -> type == SYMBOL_TYPE) {
      printf("%s", car(curVal)->s);
    }
    else if (car(curVal) -> type == OPEN_TYPE){
      printf("(");
    }
    else if (car(curVal) -> type == CLOSE_TYPE){
      printf(")");
    }
    else if (car(curVal) -> type == CONS_TYPE){
      display(car(curVal));
    }
    curVal = cdr(curVal);
  }
}

//adds a new Value v to the end of list
//modifies list itself
void appendInPlace(Value *list, Value *v) {
  
  Value *curVal = list;

  if (curVal->type == NULL_TYPE) {
    list = cons(v, list);
    return;
  }

  while (cdr(curVal)->type != NULL_TYPE) {
    curVal = cdr(curVal);
  }

  Value *vCons = cons(v, cdr(curVal));
  curVal->c.cdr = vCons;
}

// Return a new list that is the reverse of the one that is passed in. All
// content within the list should be duplicated; there should be no shared
// memory whatsoever between the original list and the new one.
//
// FAQ: What if there are nested lists inside that list?
// ANS: There won't be for this assignment. There will be later, but that will
// be after we've got an easier way of managing memory.
Value *reverse(Value *list) {

  Value *curVal = list;
  Value *prevVal = makeNull();
  Value *dupCurVal;
  Value *dupCar;
  Value *dupCdr;
  char *dupS;
  
  while (curVal->type != NULL_TYPE) {

    //duplicate the car of the current Value
    dupCar = talloc(sizeof(Value));
    dupCar->type = car(curVal)->type;
    if (dupCar->type == INT_TYPE || dupCar->type == BOOL_TYPE) {
      dupCar->i = car(curVal)->i;
    } 
    else if (dupCar->type == DOUBLE_TYPE) {
      dupCar->d = car(curVal)->d;
    }
    else if (dupCar->type == STR_TYPE || dupCar->type == SYMBOL_TYPE) {
      dupS = talloc(sizeof(char)*(strlen(car(curVal)->s)) + 1);
      strcpy(dupS, car(curVal)->s);
      dupCar->s = dupS;
    }
    dupCurVal = cons(dupCar,prevVal); //duplicate the current Value

    //move onto the next Value
    prevVal = dupCurVal;
    curVal = cdr(curVal);
  }

  return prevVal;
}

// Utility to make it less typing to get car value. Use assertions to make sure
// that this is a legitimate operation.
Value *car(Value *list) {
  return (list->c).car;
}

// Utility to make it less typing to get cdr value. Use assertions to make sure
// that this is a legitimate operation.
Value *cdr(Value *list) {
  return (list->c).cdr;
}

// Utility to check if pointing to a NULL_TYPE value. Use assertions to make sure
// that this is a legitimate operation.
bool isNull(Value *value) {
  if (value->type == NULL_TYPE) {
    return true;
  }
  return false;
}

// Measure length of list. Use assertions to make sure that this is a legitimate
// operation.
int length(Value *value) {
  Value *curVal = value;
  int count = 0;
  while (curVal -> type != NULL_TYPE) {
    curVal = cdr(curVal);
    count++;
  }
  return count;
}