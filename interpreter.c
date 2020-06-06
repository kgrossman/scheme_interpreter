#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "interpreter.h"
#include "value.h"
#include "linkedlist.h"
#include "talloc.h"
#include "parser.h"
#include "tokenizer.h"

void printEvaluatedExpr(Value *evaluatedExpr);
Value *eval(Value *tree, Frame *frame);
Value *evalDefine(Value *args, Frame *frame);
Value *evalEach(Value *args, Frame *frame);
Value *apply(Value *fcn, Value *args);
Value *evalIf(Value *args, Frame *frame);
Value *evalLet(Value *args, Frame *frame);
Value *evalLetStar(Value *args, Frame *frame);
Value *evalLetrec(Value *args, Frame *frame);
Value *evalBegin(Value *args, Frame *frame);
Value *evalSetBang(Value *args, Frame *frame);
Value *evalLambda(Value *args, Frame *frame);
Value *handleQuote(Value *args);
Value *lookUpSymbol(Value *tree, Frame *frame);
void bind(char *name, Value *(*function)(struct Value *), Frame *frame);
Value *or(Value *args, Frame *frame);
Value *and(Value *args, Frame *frame);
Value *primitiveAdd(Value *args);
Value *primitiveMinus(Value *args);
Value *primitiveLessThan(Value *args);
Value *primitiveGreaterThan(Value *args);
Value *primitiveEquals(Value *args);
Value *primitiveNull(Value *args);
Value *primitiveCar(Value *args);
Value *primitiveCdr(Value *args);
Value *primitiveCons(Value *args);
void printType(Value *v);
void evaluationError();

// Thin wrapper that calls eval for each top-level S-expression in the program. 
void interpret(Value *tree) {

  Value *curExpr = tree;
  Frame *frame = talloc(sizeof(Frame));
  frame->parent = NULL;
  frame->bindings = makeNull();

  bind("+", primitiveAdd, frame);
  bind("-", primitiveMinus, frame);
  bind("<", primitiveLessThan, frame);
  bind(">", primitiveGreaterThan, frame);
  bind("=", primitiveEquals, frame);
  bind("null?", primitiveNull, frame);
  bind("car", primitiveCar, frame);
  bind("cdr", primitiveCdr, frame);
  bind("cons", primitiveCons, frame);

  while (curExpr->type != NULL_TYPE) {
    Value *evaluatedExpr = eval(car(curExpr), frame);
    printEvaluatedExpr(evaluatedExpr);
    curExpr = cdr(curExpr);
  }
}

// Given an expression tree and a frame in which to evaluate that expression, eval returns the value of the expression.
Value *eval(Value *tree, Frame *frame) {

  switch (tree->type)  {
    case INT_TYPE: {
      return tree;
    }
    case DOUBLE_TYPE: {
      return tree;
    }
    case STR_TYPE: {
      return tree;
    }
    case BOOL_TYPE: {
      return tree;
    }
    case SYMBOL_TYPE: {
      return lookUpSymbol(tree, frame);
    }  
    case CONS_TYPE: {
      Value *first = car(tree);
      Value *args = cdr(tree);

      // Sanity and error checking on first...

      if (first->type == SYMBOL_TYPE) {
        if (!strcmp(first->s,"if")) {
          return evalIf(args, frame);
        }

        if (!strcmp(first->s,"let")) {
          return evalLet(args,frame);
        }

        if (!strcmp(first->s,"let*")) {
          return evalLetStar(args,frame);
        }

        if (!strcmp(first->s,"letrec")) {
          return evalLetrec(args,frame);
        }

        if (!strcmp(first->s,"set!")) {
          return evalSetBang(args,frame);
        }

        if (!strcmp(first->s,"begin")) {
          return evalBegin(args,frame);
        }
        
        if (!strcmp(first->s,"quote")) {
          return handleQuote(args);
        }

        if (!strcmp(first->s,"define")) {
          return evalDefine(args, frame);
        }

        if (!strcmp(first->s,"lambda")) {
          return evalLambda(args, frame);
        }

        if (!strcmp(first->s,"or")) {
          return or(args, frame);
        }

        if (!strcmp(first->s,"and")) {
          return and(args, frame);
        }

        else {
          Value *evaledOperator = eval(first, frame);
          Value *evaledArgs = evalEach(args, frame);
          return apply(evaledOperator,evaledArgs);
        }
      }

      else {
        Value *evaledOperator = eval(first, frame);
        Value *evaledArgs = evalEach(args, frame);
        return apply(evaledOperator,evaledArgs);
      }

      break;
    }
    default: {
      evaluationError("unrecognized type");
      break;
    }
    //....
  }
  return makeNull(); //unreachable. just because the compiler complains if we don't return something here.
}

void bind(char *name, Value *(*function)(struct Value *), Frame *frame) {

  Value *funcName = talloc(sizeof(Value));
  funcName->type = SYMBOL_TYPE;
  funcName->s = name;
  Value *v = talloc(sizeof(Value));
  v->type = PRIMITIVE_TYPE;
  v->pf = function;
  frame->bindings = cons(cons(funcName, v), frame->bindings);
}

Value *evalBegin(Value *args, Frame *frame) {
  
  Value *curArg = args;
  Value *evaledCurArg;

  if (curArg->type == NULL_TYPE) {
    Value *returnVoid = talloc(sizeof(Value));
    returnVoid->type = VOID_TYPE;
    return returnVoid;
  }

  while (curArg->type != NULL_TYPE) {
    if (curArg->type != CONS_TYPE) {
    evaluationError("wrong type arg in begin");
    }
    evaledCurArg = eval(car(curArg), frame);
    curArg = cdr(curArg);
  }

  return evaledCurArg;
}

Value *evalSetBang(Value *args, Frame *frame) {

  if (args->type != CONS_TYPE || cdr(args)->type != CONS_TYPE) {
    evaluationError("not enough arguments in define");
  }
  if (cdr(cdr(args))->type != NULL_TYPE) {
    evaluationError("too many arguments in define");
  }

  if (car(args)->type != SYMBOL_TYPE) {
   evaluationError("wrong type argument in define");
  } 
  
  Value *var = car(args);
  Value *expr = car(cdr(args));
  Value *evalExpr = eval(expr, frame);

  Value *toReturn = talloc(sizeof(Value));
  toReturn->type = UNSPECIFIED_TYPE;

  Frame *curFrame = frame;
  while (curFrame != NULL) {
    Value *curVal = curFrame->bindings;
    while (curVal->type != NULL_TYPE) {
      if (!strcmp(car(car(curVal))->s, var->s)) {
        car(curVal)->c.cdr = evalExpr;
        return toReturn;
      }
      curVal = cdr(curVal);
    }
    curFrame = curFrame->parent;
  }

  evaluationError("in evalSetBang: symbol not found");
  return toReturn;
}

Value *evalLetrec(Value *args, Frame *frame) {
   /*
   (let ((var UNSPECFIED) ...)
   (let ((temp expr) ...)
    (set! var temp) ...
    (let () body1 body2 ...)))
    */

  Frame *newFrame = talloc(sizeof(Frame));
  newFrame->parent = frame;
  newFrame->bindings = makeNull();
  
  if (args->type != CONS_TYPE || cdr(args)->type != CONS_TYPE) {
    evaluationError("not enough arguments in letrec");
  }

  Value *curExpr = car(args);

  //create bindings
  while (curExpr->type != NULL_TYPE) {
    if (curExpr->type != CONS_TYPE || car(curExpr)->type != CONS_TYPE || cdr(car(curExpr))->type != CONS_TYPE) {
      evaluationError("not enough arguments in letrec variable assignment");
    }
    if (cdr(car(curExpr))->type == CONS_TYPE && cdr(cdr(car(curExpr)))->type != NULL_TYPE) {
      evaluationError("too many arguments in letrec variable assignment");
    }

    Value *curVar = car(car(curExpr));

    if (curVar->type != SYMBOL_TYPE) {
      evaluationError("in letrec: cannot assign value to a non-symbol");
    }
    
    Value *v = newFrame->bindings;
    while (v->type != NULL_TYPE) {
      if (!strcmp(car(car(v))->s, curVar->s)) {
        evaluationError("in letrec: cannot assign variable more that once");
      }
      v = cdr(v);
    }

    Value *unspecified = talloc(sizeof(Value));
    unspecified->type = UNSPECIFIED_TYPE;

    newFrame->bindings = cons(cons(curVar, unspecified), newFrame->bindings);

    curExpr = cdr(curExpr);
  }

  curExpr = car(args);

  //construct a list of all the evaled rhs's
  Value *evaledRhs = makeNull();
  Value *curVal;
  Value *evaledCurVal;

  while(curExpr->type != NULL_TYPE) {
    curVal = cdr(car(curExpr));
    evaledCurVal = eval(car(curVal), newFrame);
    evaledRhs = cons(evaledCurVal, evaledRhs);
    curExpr = cdr(curExpr);
  }

  Value *curBinding = newFrame->bindings;
  Value *curEvaledRhs = evaledRhs;
  
  while (curBinding->type != NULL_TYPE) {
    car(curBinding)->c.cdr = car(curEvaledRhs);
    curBinding = cdr(curBinding);
    curEvaledRhs = cdr(curEvaledRhs);
  }

  Value *evalExpr;
  Value *curArg = cdr(args);

  while (curArg->type != NULL_TYPE) {
    evalExpr = eval(car(curArg), newFrame);
    curArg = cdr(curArg);
  }

  return evalExpr;
}

Value *evalLetStar(Value *args, Frame *frame) {
  
  if (args->type != CONS_TYPE || cdr(args)->type != CONS_TYPE) {
    evaluationError("not enough arguments in let");
  }

  Value *curExpr = car(args);

  Frame *newFrame = talloc(sizeof(Frame));
  newFrame->parent = frame;
  newFrame->bindings = makeNull();

  //create bindings
  while (curExpr->type != NULL_TYPE) {
    if (curExpr->type != CONS_TYPE || car(curExpr)->type != CONS_TYPE || cdr(car(curExpr))->type != CONS_TYPE) {
      evaluationError("not enough arguments in let variable assignment");
    }
    if (cdr(car(curExpr))->type == CONS_TYPE && cdr(cdr(car(curExpr)))->type != NULL_TYPE) {
      evaluationError("too many arguments in let variable assignment");
    }

    Value *curVar = car(car(curExpr));
    if (curVar->type != SYMBOL_TYPE) {
      evaluationError("cannot assign value to a non-symbol");
    }
    
    Frame *newFrame = talloc(sizeof(Frame));
    newFrame->parent = frame;
    newFrame->bindings = makeNull();

    Value *v = newFrame->bindings;
    while (v->type != NULL_TYPE) {
      if (!strcmp(car(car(v))->s, curVar->s)) {
        evaluationError("cannot assign variable more that once");
      }
      v = cdr(v);
    }
    
    Value *curVal = eval(car(cdr(car(curExpr))), frame);
    newFrame->bindings = cons(cons(curVar, curVal), newFrame->bindings);

    frame = newFrame;
    curExpr = cdr(curExpr);
  }

  Value *evalExpr;
  Value *curArg = cdr(args);

  //evaluate body
  while (curArg->type != NULL_TYPE) {
    evalExpr = eval(car(curArg), frame);
    curArg = cdr(curArg);
  }
  return evalExpr;
}

Value *and(Value *args, Frame *frame) {
  
  Value *isTrue = talloc(sizeof(Value));
  isTrue->type = BOOL_TYPE;
  isTrue->i = 1;

  Value *curVal = args;
  while (curVal->type != NULL_TYPE) {
    if (eval(car(curVal), frame)->i == 0) {
      isTrue->i = 0;
      return isTrue;
    }
    curVal = cdr(curVal);
  }

  return isTrue;
}

Value *or(Value *args, Frame *frame) {
  
  Value *isTrue = talloc(sizeof(Value));
  isTrue->type = BOOL_TYPE;
  isTrue->i = 0;

  Value *curVal = args;
  while (curVal->type != NULL_TYPE) {
    if (eval(car(curVal), frame)->i == 1) {
      isTrue->i = 1;
      return isTrue;
    }
    curVal = cdr(curVal);
  }

  return isTrue;
}

Value *primitiveEquals(Value *args) {
  if(args->type == NULL_TYPE) {
    evaluationError("no args in primitive =");
  }
  if(args->type != CONS_TYPE || cdr(args)->type != CONS_TYPE) {
    evaluationError("too few args in primitive =");
  }
  if(cdr(cdr(args))->type != NULL_TYPE) {
    evaluationError("too many args in primitive =");
  }

  Value *isEqual = talloc(sizeof(Value));
  isEqual->type = BOOL_TYPE;
  isEqual->i = 0;

  if (car(args)->type == INT_TYPE) {
    if (car(cdr(args))->type == INT_TYPE) {
      if (car(args)->i == car(cdr(args))->i) {
        isEqual->i = 1;
      }
    }
    else if (car(cdr(args))->type == DOUBLE_TYPE) {
      if (car(args)->i == car(cdr(args))->d) {
        isEqual->i = 1;
      }
    }
    else {
      evaluationError("wrong type arg in primitive =");
    }
  }
  else if (car(args)->type == DOUBLE_TYPE) {
    if (car(cdr(args))->type == INT_TYPE) {
      if (car(args)->d == car(cdr(args))->i) {
        isEqual->i = 1;
      }
    }
    else if (car(cdr(args))->type == DOUBLE_TYPE) {
      if (car(args)->d == car(cdr(args))->d) {
        isEqual->i = 1;
      }
    }
    else {
      evaluationError("wrong type arg in primitive =");
    }
  }
  else {
    evaluationError("wrong type arg in primitive =");
  }

  return isEqual;
}

Value *primitiveGreaterThan(Value *args) {
  if(args->type == NULL_TYPE) {
    evaluationError("no args in primitive >");
  }
  if(args->type != CONS_TYPE || cdr(args)->type != CONS_TYPE) {
    evaluationError("too few args in primitive >");
  }
  if(cdr(cdr(args))->type != NULL_TYPE) {
    evaluationError("too many args in primitive >");
  }

  Value *isGreaterThan = talloc(sizeof(Value));
  isGreaterThan->type = BOOL_TYPE;
  isGreaterThan->i = 0;

  if (car(args)->type == INT_TYPE) {
    if (car(cdr(args))->type == INT_TYPE) {
      if (car(args)->i > car(cdr(args))->i) {
        isGreaterThan->i = 1;
      }
    }
    else if (car(cdr(args))->type == DOUBLE_TYPE) {
      if (car(args)->i > car(cdr(args))->d) {
        isGreaterThan->i = 1;
      }
    }
    else {
      evaluationError("wrong type arg in primitive >");
    }
  }
  else if (car(args)->type == DOUBLE_TYPE) {
    if (car(cdr(args))->type == INT_TYPE) {
      if (car(args)->d > car(cdr(args))->i) {
        isGreaterThan->i = 1;
      }
    }
    else if (car(cdr(args))->type == DOUBLE_TYPE) {
      if (car(args)->d > car(cdr(args))->d) {
        isGreaterThan->i = 1;
      }
    }
    else {
      evaluationError("wrong type arg in primitive >");
    }
  }
  else {
    evaluationError("wrong type arg in primitive >");
  }

  return isGreaterThan;
}

Value *primitiveLessThan(Value *args) {
  if(args->type == NULL_TYPE) {
    evaluationError("no args in primitive <");
  }
  if(args->type != CONS_TYPE || cdr(args)->type != CONS_TYPE) {
    evaluationError("too few args in primitive <");
  }
  if(cdr(cdr(args))->type != NULL_TYPE) {
    evaluationError("too many args in primitive <");
  }

  Value *isLessThan = talloc(sizeof(Value));
  isLessThan->type = BOOL_TYPE;
  isLessThan->i = 0;

  if (car(args)->type == INT_TYPE) {
    if (car(cdr(args))->type == INT_TYPE) {
      if (car(args)->i < car(cdr(args))->i) {
        isLessThan->i = 1;
      }
    }
    else if (car(cdr(args))->type == DOUBLE_TYPE) {
      if (car(args)->i < car(cdr(args))->d) {
        isLessThan->i = 1;
      }
    }
    else {
      evaluationError("wrong type arg in primitive <");
    }
  }
  else if (car(args)->type == DOUBLE_TYPE) {
    if (car(cdr(args))->type == INT_TYPE) {
      if (car(args)->d < car(cdr(args))->i) {
        isLessThan->i = 1;
      }
    }
    else if (car(cdr(args))->type == DOUBLE_TYPE) {
      if (car(args)->d < car(cdr(args))->d) {
        isLessThan->i = 1;
      }
    }
    else {
      evaluationError("wrong type arg in primitive <");
    }
  }
  else {
    evaluationError("wrong type arg in primitive <");
  }

  return isLessThan;
}

Value *primitiveMinus(Value *args) {
  Value *result = talloc(sizeof(Value));

  bool containsReal = false;
  Value *curArg = args;

  double diff = 0;
  if (car(curArg)->type == DOUBLE_TYPE) {
      containsReal = true;
      diff += car(curArg)->d;
    }
  else if (car(curArg)->type == INT_TYPE) {
      diff += car(curArg)->i;
  }
  else {
      evaluationError("nonnumerical argument in -");
    }
  curArg = cdr(curArg);

  while (curArg->type != NULL_TYPE) {
    if (car(curArg)->type == DOUBLE_TYPE) {
      containsReal = true;
      diff -= car(curArg)->d;
    }
    else if (car(curArg)->type == INT_TYPE) {
      diff -= car(curArg)->i;
    }
    else {
      evaluationError("nonnumerical argument in -");
    }
    curArg = cdr(curArg);
  }

  if (containsReal) {
    result->type = DOUBLE_TYPE;
    result->d = diff;
  }
  else {
    result->type = INT_TYPE;
    result->i = (int) diff;
  }

  return result;
}

//returns the sum of the args
//as an integer if all ints, otherwise as a double if there is at least one real arg
//error if any arg is nonnumerical
Value *primitiveAdd(Value *args) {

  Value *result = talloc(sizeof(Value));

  bool containsReal = false;
  Value *curArg = args;

  double sum = 0;
  
  while (curArg->type != NULL_TYPE) {
    if (car(curArg)->type == DOUBLE_TYPE) {
      containsReal = true;
      sum += car(curArg)->d;
    }
    else if (car(curArg)->type == INT_TYPE) {
      sum += car(curArg)->i;
    }
    else {
      evaluationError("nonnumerical argument in +");
    }
    curArg = cdr(curArg);
  }

  if (containsReal) {
    result->type = DOUBLE_TYPE;
    result->d = sum;
  }
  else {
    result->type = INT_TYPE;
    result->i = (int) sum;
  }

  return result;
}

Value *primitiveNull(Value *args) {
  
  Value *isNull = talloc(sizeof(Value));
  isNull->type = BOOL_TYPE;
  isNull->i = 0;
  
  if (args->type == NULL_TYPE) {
    evaluationError("no args in null?");
  }
  if (args->type != CONS_TYPE) {
    evaluationError("wrong type arg in null?");
  }
  if (cdr(args)->type != NULL_TYPE) {
    evaluationError("more than one arg in null?");
  }

  if (car(args)->type == CONS_TYPE && car(car(args))->type == NULL_TYPE) {
    isNull->i = 1;
  }
  
  return isNull;
}

Value *primitiveCar(Value *args) {
  if (args->type != CONS_TYPE || car(args)->type != CONS_TYPE || car(car(args))->type != CONS_TYPE) {
    evaluationError("wrong type argument in primitive car");
  }
  if (cdr(args)->type != NULL_TYPE) {
    evaluationError("too many args in primitive car");
  }
  return car(car(car(args)));
}

Value *primitiveCdr(Value *args) {

  if (args->type != CONS_TYPE || car(args)->type != CONS_TYPE) {
    evaluationError("wrong type argument in primitive cdr");
  }

  if (car(car(args))->type == NULL_TYPE) {
    return cons(makeNull(), makeNull()); //empty list
  }

  //improper list case
  if (cdr(car(car(args)))->type != CONS_TYPE && cdr(car(car(args)))->type != NULL_TYPE) {
    return cdr(car(car(args)));
  }
  
  //proper list case
  return cons(cdr(car(car(args))), makeNull());
}

Value *primitiveCons(Value *args) {

  if (args->type != CONS_TYPE) {
    evaluationError("wrong type arg in primitive cons");
  }
  if (cdr(args)->type != CONS_TYPE) {
    evaluationError("wrong type arg in primitive cons");
  }
  if (cdr(cdr(args))->type != NULL_TYPE) {
    evaluationError("too many args in primitive cons");
  }

  Value *carItem = car(args);
  Value *cdrItem;

  //improper list case
  if (car(cdr(args))->type != CONS_TYPE) {
    cdrItem = car(cdr(args));
  }
  //proper list case
  else {
    cdrItem = car(car(cdr(args)));
  }

  return cons(cons(carItem, cdrItem), makeNull());
}

Value *apply(Value *function, Value *args) {
  
  if (function->type == PRIMITIVE_TYPE) {
    return function->pf(args);
  }
  
  //function is a closure

  //check if function is closure??
  if (function->type != CLOSURE_TYPE) {
    evaluationError("function not a closure");
  }

  //it contains the body, param names, and pointer to env

  //create newFrame
  Frame *newFrame = talloc(sizeof(Frame));
  //make parent of newFrame point to the env that closure points to (function->cl.frame)
  newFrame->parent = function->cl.frame;

  //make bindings to connect formal parameters (function->cl.paramNames) with the actual parameters (args)
  newFrame->bindings = makeNull();

  Value *curFormal = function->cl.paramNames;
  Value *curActual = args;
  //create bindings
  while (curFormal->type != NULL_TYPE && curActual->type != NULL_TYPE) {
    newFrame->bindings = cons(cons(car(curFormal), car(curActual)), newFrame->bindings);

    curFormal = cdr(curFormal);
    curActual = cdr(curActual);
  }

  //error if different number arguments
  if (!(curFormal->type == NULL_TYPE && curActual->type == NULL_TYPE)) {
    evaluationError("inconsistent number of arguments in apply");
  }

  //evaluate the body (function->cl.functionCode), with newFrame as the frame
  return eval(function->cl.functionCode, newFrame);
}

Value *evalEach(Value *args, Frame *frame) {
  
  Value *curArg = args;
  Value *evalList = makeNull();
  Value *lastEvaledArg;

  while (curArg->type != NULL_TYPE) {

    //first iteration
    if (evalList->type == NULL_TYPE) {
      evalList = cons(eval(car(curArg), frame), makeNull());
      lastEvaledArg = evalList;
    }
    else {
      lastEvaledArg->c.cdr = cons(eval(car(curArg), frame), makeNull());
      lastEvaledArg = cdr(lastEvaledArg);
    }
    curArg = cdr(curArg);
  }
  
  return evalList;
}

Value *evalLambda(Value *args, Frame *frame) {

  if (args->type != CONS_TYPE || cdr(args)->type != CONS_TYPE) {
    evaluationError("not enough arguments in lambda");
  }
  
  if (cdr(cdr(args))->type != NULL_TYPE) {
    evaluationError("too many arguments in lambda");
  }

  //catch when formal parameters are not symbol type
  Value *curArg = args;
  
  while (curArg->type != NULL_TYPE) {
    if (car(args)->type != NULL_TYPE) {
      if (car(args)->type == CONS_TYPE) {
        if (car(car(args))->type != SYMBOL_TYPE) {
          evaluationError("formal argument of lambda not symbol type");
        }
      }
      else if (car(args)->type != SYMBOL_TYPE) {
        evaluationError("formal argument of lambda not symbol type");
      }
    }
    curArg = cdr(curArg);
  }

  Value *closure = talloc(sizeof(Value));
  closure->type = CLOSURE_TYPE;
  closure->cl.frame = frame;
  closure->cl.paramNames = car(args);

  curArg = closure->cl.paramNames;
  Value *current = cdr(curArg);

  while (curArg->type != NULL_TYPE) {
    while (current->type != NULL_TYPE) {
      if (car(curArg)->type == SYMBOL_TYPE && car(current)->type == SYMBOL_TYPE && !strcmp(car(curArg)->s, car(current)->s)) {
        evaluationError("duplicate formal parameter in lambda");
      }
      current = cdr(current);
    }
    curArg = cdr(curArg);
  }

  closure->cl.functionCode = car(cdr(args));

  return closure;
}

Value *evalDefine(Value *args, Frame *frame) {
  if (args->type != CONS_TYPE || cdr(args)->type != CONS_TYPE) {
    evaluationError("not enough arguments in define");
  }
  if (cdr(cdr(args))->type != NULL_TYPE) {
    evaluationError("too many arguments in define");
  }

 if (car(args)->type != SYMBOL_TYPE) {
   evaluationError("wrong type argument in define");
 } 
  
  Value *var = car(args);
  Value *expr = car(cdr(args));
  Value *evalExpr = eval(expr, frame);

  frame->bindings = cons(cons(var, evalExpr), frame->bindings);

  Value *toReturn = talloc(sizeof(Value));
  toReturn->type = VOID_TYPE;

  return toReturn;
}

Value *evalIf(Value *args, Frame *frame) {
  if (args->type != NULL_TYPE && cdr(args)->type != NULL_TYPE && cdr(cdr(args))->type != NULL_TYPE) {
    Value *condition = eval(car(args), frame);
    if (condition->type != BOOL_TYPE) {
      evaluationError("if statement condition not bool type");
    }
    if (condition->i) {
      return eval(car(cdr(args)), frame);
    }
    return eval(car(cdr(cdr(args))), frame);
  }
  evaluationError("if statement wrong number arguments");
  return makeNull(); //unreachable
}

Value *evalLet(Value *args, Frame *frame) {
  
  Frame *newFrame = talloc(sizeof(Frame));
  newFrame->parent = frame;
  newFrame->bindings = makeNull();
  
  if (args->type != CONS_TYPE || cdr(args)->type != CONS_TYPE) {
    evaluationError("not enough arguments in let");
  }

  Value *curExpr = car(args);

  //create bindings
  while (curExpr->type != NULL_TYPE) {
    if (curExpr->type != CONS_TYPE || car(curExpr)->type != CONS_TYPE || cdr(car(curExpr))->type != CONS_TYPE) {
      evaluationError("not enough arguments in let variable assignment");
    }
    if (cdr(car(curExpr))->type == CONS_TYPE && cdr(cdr(car(curExpr)))->type != NULL_TYPE) {
      evaluationError("too many arguments in let variable assignment");
    }

    Value *curVar = car(car(curExpr));
    if (curVar->type != SYMBOL_TYPE) {
      evaluationError("cannot assign value to a non-symbol");
    }
    
    Value *v = newFrame->bindings;
    while (v->type != NULL_TYPE) {
      if (!strcmp(car(car(v))->s, curVar->s)) {
        evaluationError("cannot assign variable more that once");
      }
      v = cdr(v);
    }

    Value *curVal = eval(car(cdr(car(curExpr))), frame);
    newFrame->bindings = cons(cons(curVar, curVal), newFrame->bindings);

    curExpr = cdr(curExpr);
  }

  Value *evalExpr;
  Value *curArg = cdr(args);

  //evaluate body
  while (curArg->type != NULL_TYPE) {
    evalExpr = eval(car(curArg), newFrame);
    curArg = cdr(curArg);
  }
  return evalExpr;
}

Value *handleQuote(Value *args) {
  if (args->type == NULL_TYPE) {
    evaluationError("no args after quote");
  }
  if (cdr(args)->type != NULL_TYPE) {
    evaluationError("too many args after quote");
  }
  return args;
}

Value *lookUpSymbol(Value *tree, Frame *frame) {

  Frame *curFrame = frame;
  while (curFrame != NULL) {
    Value *curVal = curFrame->bindings;
    while (curVal->type != NULL_TYPE) {
      if (!strcmp(car(car(curVal))->s, tree->s)) {
        return cdr(car(curVal));
      }
      curVal = cdr(curVal);
    }
    curFrame = curFrame->parent;
  }
  evaluationError("in lookUpSymbol: symbol not found");
  return makeNull(); //unreachable
}

void printEvaluatedExpr(Value *evaluatedExpr) {

    if (evaluatedExpr->type == INT_TYPE) {
      printf("%i\n", evaluatedExpr->i);
    }
    else if (evaluatedExpr->type == BOOL_TYPE) {
      if (evaluatedExpr->i == 0) {
        printf("#f\n");
      }
      else {
        printf("#t\n");
      }
    }
    else if (evaluatedExpr->type == DOUBLE_TYPE) {
      printf("%g\n", evaluatedExpr->d);
    }
    else if (evaluatedExpr->type == STR_TYPE || evaluatedExpr->type == SYMBOL_TYPE) {
      printf("%s\n", evaluatedExpr->s);
    }
    else if (evaluatedExpr->type == CONS_TYPE) {
      printTree(evaluatedExpr);
    }
    else if (evaluatedExpr->type == CLOSURE_TYPE) {
      printf("#<procedure>\n");
    }
}

//prints v->type
//NOTE: must update type names list whenever more types added in value.h
//type names must be in exact same order as defined in value.h
void printType(Value *v) {
  char *typeNames[18] = {"INT_TYPE", "DOUBLE_TYPE", "STR_TYPE", "CONS_TYPE", "NULL_TYPE", "PTR_TYPE","OPEN_TYPE", "CLOSE_TYPE", "BOOL_TYPE", "SYMBOL_TYPE", "OPENBRACKET_TYPE", "CLOSEBRACKET_TYPE", "DOT_TYPE", "SINGLEQUOTE_TYPE", "VOID_TYPE", "CLOSURE_TYPE", "PRIMITIVE_TYPE", "UNSPECIFIED_TYPE"};

  printf("%s\n", typeNames[(int) v->type]);
}

void evaluationError(char *errorMessage) {
  //printf("%s\n", errorMessage);
  printf("Evaluation error: %s\n", errorMessage);
  texit(1);
}
