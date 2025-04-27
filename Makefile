
GCC = gcc -std=c99 -Wall -O2 
SRC = main.c rsa.c BigInt.c
OBJ = $(SRC:.c=.o)
HS = rsa.h BigInt.h
EXEC = rsa_run

.o: .c
	$(GCC) -c $< -o $@

all: $(OBJ)
	$(GCC) $(OBJ) -o $(EXEC)

test: clean all
	./$(EXEC) enc input.txt enc.out

	./$(EXEC) dec enc.out dec.out

	diff input.txt dec.out

clean:
	@rm -rf $(EXEC) $(OBJ) *.out private.key public.key 