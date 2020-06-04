#ifndef _INTERPRETER
#define _INTERPRETER

void interpret(Value *tree);
Value *eval(Value *expr, Frame *frame);
void printValue(Value *value);

#endif

