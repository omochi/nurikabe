CFLAGS += -Wall -Wextra -Werror  -O3
all : main
main : main.o OMMem.o
clean : 
	rm -f *.o main
