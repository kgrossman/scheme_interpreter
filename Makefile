CC = clang
CFLAGS = -g

# To use my binaries, comment out the very next line and uncomment the following
SRCS = linkedlist.c talloc.c main.c tokenizer.c parser.c interpreter.c
#SRCS = lib/linkedlist.o lib/talloc.o main.c lib/tokenizer.o lib/parser.o interpreter.c

HDRS = linkedlist.h talloc.h value.h tokenizer.h parser.h interpreter.h
OBJS = $(SRCS:.c=.o)

.PHONY: interpreter
interpreter: $(OBJS)
	rm -f vgcore.*
	$(CC)  $(CFLAGS) $^  -o $@

.PHONY: phony_target
phony_target:

%.o : %.c $(HDRS) phony_target
	$(CC)  $(CFLAGS) -c $<  -o $@

clean:
	rm -f *.o
	rm -f interpreter

test: interpreter
	python3 tester.py

capstone: interpreter
	python3 tester.py tests-capstone
