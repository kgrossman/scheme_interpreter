#include "value.h"
#include "linkedlist.h"
#include "talloc.h"
#include "parser.h"
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "interpreter.h"

Value *reverseParseTree(Value *tree);
void syntaxError(int depth);
Value *addToParseTree(Value *token, Value *tree);
Value *makeSubTree(Value *tree);
void printToken(Value *token);
void printSubTree(Value *subTree);
void printTree(Value *tree);

// Takes a list of tokens from a Racket program, and returns a pointer to a
// parse tree representing that program.
Value *parse(Value *tokens) {

  Value *tree = makeNull();
  int depth = 0;

  Value *current = tokens;
  assert(current != NULL && "Error (parse): null pointer");
  
  while (current->type != NULL_TYPE) {
    Value *token = car(current);
    if (token->type != CLOSE_TYPE) {
      if (token->type == OPEN_TYPE) {
        depth++;
      }
      tree = addToParseTree(token, tree);
    }
    else {
      depth--;
      tree = makeSubTree(tree);
    }

    current = cdr(current);
  }

  if (depth != 0) {
    syntaxError(depth);
  }

  tree = reverseParseTree(tree);
  return tree;
}

//push non-close-paren tokens onto stack
Value *addToParseTree(Value *token, Value *tree) {
  tree = cons(token, tree);
  return tree;
}

//called when we hit a close paren
Value *makeSubTree(Value *tree) {
  
  Value *subTree = makeNull();
  Value *current = tree;

  while (current->type != NULL_TYPE && car(current)->type != OPEN_TYPE) {
    Value *token = car(current);
    subTree = cons(token, subTree);
    current = cdr(current);
  }

  if (current->type == NULL_TYPE) {
    syntaxError(-1);
  }

  tree = cons(subTree, cdr(current));

  return tree;
}

// Prints the tree to the screen in a readable fashion. It should look just like
// Racket code; use parentheses to indicate subtrees.
void printTree(Value *tree) {
  
  Value *curVal = tree;

  while (curVal->type != NULL_TYPE) {
    if (car(curVal)->type == CONS_TYPE) {
      printSubTree(car(curVal));
    }
    else if (car(curVal)->type == NULL_TYPE) {
      printf("()");
    }
    else {
      printToken(car(curVal));
    }
    curVal = cdr(curVal);
  }
  printf("\n");
}

//helper function for printTree
//recursive, to account for nesting
void printSubTree(Value *subTree) {

  printf("(");

  Value *curVal = subTree; 
  while (curVal->type != NULL_TYPE) {
    
    if (curVal->type == CONS_TYPE) {
      
      if (cdr(curVal)->type != CONS_TYPE && cdr(curVal)->type != NULL_TYPE) {
        if (car(curVal)->type != CONS_TYPE && car(curVal)->type != NULL_TYPE) {
          printToken(car(curVal));
          printf(" . ");
          printToken(cdr(curVal));
        }
        else {
          if (car(car(curVal))->type != CONS_TYPE) {
            printSubTree(car(curVal));
          }
          else {
            printSubTree(car(car(curVal)));
          }
          printf(" . ");
          printToken(cdr(curVal));
        }
        break;
      }

      else if (car(curVal)->type == CONS_TYPE) {
        if (car(car(curVal))->type != CONS_TYPE) {
            printSubTree(car(curVal));
          }
          else {
            printSubTree(car(car(curVal)));
          }
      }
      else if (car(curVal)->type == NULL_TYPE) {
        printf("()");
      }
      else {
        printToken(car(curVal));
      }
    }
    else {
      printToken(curVal);
      break;
    }
    
    curVal = cdr(curVal);
  }

  printf(") ");
}

void printToken(Value *token) {
  if (token->type == INT_TYPE) {
    printf("%i ", token->i);
  }
  else if (token->type == DOUBLE_TYPE) {
    printf("%0.6f ", token->d);
  }
  else if (token->type == STR_TYPE || token->type == SYMBOL_TYPE) {
    printf("%s ", token->s);
  }
  else if (token->type == BOOL_TYPE) {
    if (token->i == 1) {
      printf("#t ");
    }
    else if (token->i == 0) {
      printf("#f ");
    }
  }
}

//new reverse function, tailored to parse trees
//handles nested trees
//uses original memory
Value *reverseParseTree(Value *tree) {

  Value *curVal = tree;

  Value *prevVal = makeNull();
  Value *nextVal;

  while (curVal->type != NULL_TYPE) {
    nextVal = cdr(curVal);
    (curVal->c).cdr = prevVal;
    prevVal = curVal;
    curVal = nextVal;
  }

  return prevVal;
}

void syntaxError(int depth) {
  if (depth < 0)
    printf("Syntax error: too many close parentheses \n");
  else if (depth > 0)
    printf("Syntax error: not enough close parentheses\n");
  texit(1);
}
