CC = mpicc
CFLAGS = -O3 -Wall -I.
RM = rm -f

OBJ = main.o

%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

main: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

clean:
	$(RM) $(OBJ) *.0 main
