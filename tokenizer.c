#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "value.h"
#include "tokenizer.h"
#include "talloc.h"
#include "linkedlist.h"

// Read all of the input from stdin, and return a linked list consisting of the
// tokens.
Value *tokenize() {
  char charRead;
  Value *list = makeNull();
  charRead = (char)fgetc(stdin);

  char *digits = talloc(sizeof(char)*13);
  strcpy(digits, "01234567890.");
  
  char *signs = talloc(sizeof(char)*3);
  strcpy(signs, "+-");

  char *symbols = talloc(sizeof(char)*80);
  strcpy(symbols, "!$%&*/:<=>?~_^abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ+-.0123456789");

  while (charRead != EOF) {

    //string
    if (charRead == '\"') {
      char *str = talloc(sizeof(char)*300);
      charRead = (char)fgetc(stdin);
      str[0] = '\"';
      int i = 1;
      while (charRead != '\"' && charRead != EOF) {
        str[i] = charRead;
        i++;
        charRead = (char)fgetc(stdin);
      }
      str[i] ='\"';
      str[i+1] = '\0';
      Value *v = talloc(sizeof(Value));
      v->type = STR_TYPE;
      v->s = str;
      list = cons(v, list);
    }

    //comment
    else if (charRead == ';') {
      while (charRead != '\n' && charRead != EOF) {
        charRead = (char)fgetc(stdin);
      }
      charRead = (char)ungetc(charRead, stdin);
    }

    //open
    else if (charRead == '(') {
      Value *v = talloc(sizeof(Value));
      v->type = OPEN_TYPE;
      list = cons(v, list);
    }
    
    //close
    else if (charRead == ')') {
      Value *v = talloc(sizeof(Value));
      v->type = CLOSE_TYPE;
      list = cons(v, list);
    }
    
    //bool
    else if (charRead == '#') {
      charRead = (char)fgetc(stdin);
      Value *v = talloc(sizeof(Value));
      v->type = BOOL_TYPE;

      //true
      if (charRead == 't') {
        v->i = 1;
      }

      //false
      else if (charRead == 'f') {
        v->i = 0;
      }

      else {
        printf("ERROR");
        texit(1);
      }

      list = cons(v, list);
    }

    //signs
    else if (strchr(signs, charRead) != NULL) {
      Value *v = talloc(sizeof(Value));
      char *sign = talloc(sizeof(char)*300);
      sign[0] = charRead;

      bool isPlus = false;
      if (charRead == '+') {
        isPlus = true;
      }
      
      charRead = (char)fgetc(stdin);
      int i = 1;

      //symbol
      if (charRead == ' ' || charRead == ')') {
        v->type = SYMBOL_TYPE;
        //strcpy(v->s, sign[0]);
        if (isPlus) {
          v->s = "+\0";
        }
        else {
          v->s = "-\0";
        }
        //v->s = sign[0];
        list = cons(v, list);
        if (charRead == ')') {
          charRead = (char)ungetc(charRead, stdin);
        }
      }

      //number
      else if (strchr(digits, charRead) != NULL) {
        while (strchr(digits, charRead) != NULL) {
          sign[i] = charRead;
          charRead = (char)fgetc(stdin);
          i++;
        }
        sign[i] = '\0';
        charRead = (char)ungetc(charRead, stdin);
        if (strchr(sign, '.')) {
          v->type = INT_TYPE;
          int newNum = atoi(sign);
          v->i = newNum;
        }
        else {
          v->type = DOUBLE_TYPE;
          double newNum = atof(sign);
          v->d = newNum;
        }
      
        list = cons(v, list);
      }
    }

    //numbers
    else if (strchr(digits, charRead) != NULL) {
      Value *v = talloc(sizeof(Value));
      char *num = talloc(sizeof(char)*300);
      int i = 0;
      while (strchr(digits, charRead) != NULL) {
        num[i] = charRead;
        charRead = (char)fgetc(stdin);
        i++;
      }
      num[i] = '\0';
      charRead = (char)ungetc(charRead, stdin);
      if (!strchr(num, '.')) {
      //if (strchr(num, '.') == NULL) {
        v->type = INT_TYPE;
        int newNum = atoi(num);
        v->i = newNum;
      }
      else {
        v->type = DOUBLE_TYPE;
        double newNum = atof(num);
        v->d = newNum;
      }
      
      list = cons(v, list);
    }

    //space
    else if (charRead == ' ' || charRead == '\n') {
      int hibye = 0; //do nothing
    }

    //symbols
    else if (strchr(symbols, charRead) != NULL){
      char *sym = talloc(sizeof(char)*300);
      int i = 0;
      while (strchr(symbols, charRead) != NULL) {
        sym[i] = charRead;
        i++;
        charRead = (char)fgetc(stdin);
      }
      charRead = (char)ungetc(charRead, stdin);
      sym[i] = '\0';
      Value *v = talloc(sizeof(Value));
      v->type = SYMBOL_TYPE;
      v->s = sym;

      list = cons(v, list);
    }

    else {
      printf("ERROR");
      texit(1);
    }

    charRead = (char)fgetc(stdin);
  }
  
  Value *revList = reverse(list);
  return revList;
}

// Displays the contents of the linked list as tokens, with type information
void displayTokens(Value *list) {
  Value *curVal = list;
  while (curVal -> type != NULL_TYPE) {
    if (car(curVal) -> type == INT_TYPE) {
      printf("%i:integer\n", car(curVal)->i);
    }
    else if (car(curVal) -> type == DOUBLE_TYPE) {
      printf("%0.6f:double\n", car(curVal)->d);
    }
    else if (car(curVal) -> type == STR_TYPE) {
      printf("%s:string\n", car(curVal)->s);
    }
    else if (car(curVal) -> type == BOOL_TYPE) {
      if (car(curVal)->i == 1) {
        printf("#t:boolean\n");
      }
      else if (car(curVal)->i == 0) {
        printf("#f:boolean\n");
      }
    }
    else if (car(curVal) -> type == SYMBOL_TYPE) {
      printf("%s:symbol\n", car(curVal)->s);
    }
    else if (car(curVal) -> type == OPEN_TYPE) {
      printf("(:open\n");
    }
    else if (car(curVal) -> type == CLOSE_TYPE) {
      printf("):close\n");
    }
    curVal = cdr(curVal);
  }
}