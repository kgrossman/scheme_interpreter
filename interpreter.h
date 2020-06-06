#include "value.h"
#include "linkedlist.h"
#include "talloc.h"
#include "parser.h"
#include "tokenizer.h"

#ifndef _INTERPRETER
#define _INTERPRETER

void interpret(Value *tree);
Value *eval(Value *expr, Frame *frame);
void printValue(Value *value);

#endif

