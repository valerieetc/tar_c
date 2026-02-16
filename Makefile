all: my_tar

my_tar: my_tar.o
	gcc -Wall -Wextra -Werror -o my_tar my_tar.o


my_tar.o: my_tar.c
	gcc -Wall -Wextra -Werror -c my_tar.c

clean:
	rm -f *.o

fclean: clean
	rm -f my_tar

re: fclean all