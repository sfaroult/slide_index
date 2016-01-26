MINIZ=./miniz
OBJ=si_settings.o si_util.o si_tree_ops.o si_db_ops.o si_parse_ops.o \
	miniz.o
CFLAGS= -g -Wall -I$(MINIZ)

all: slide_index

slide_index: slide_index.o  $(OBJ)
	gcc $(CFLAGS) -o slide_index slide_index.o $(OBJ) -lsqlite3

slide_index.o: slide_index.c
	gcc -c $(CFLAGS) -o slide_index.o slide_index.c

si_tree_ops.o: si_tree_ops.c
	gcc -c $(CFLAGS) -o si_tree_ops.o si_tree_ops.c

si_db_ops.o: si_db_ops.c
	gcc -c $(CFLAGS) -o si_db_ops.o si_db_ops.c

si_parse_ops.o: si_parse_ops.c
	gcc -c $(CFLAGS) -o si_parse_ops.o si_parse_ops.c

si_settings.o: si_settings.c
	gcc -c $(CFLAGS) -o si_settings.o si_settings.c

si_util.o: si_util.c
	gcc -c $(CFLAGS) -o si_util.o si_util.c

miniz.o: $(MINIZ)/miniz.c
	gcc -c $(CFLAGS) -o miniz.o $(MINIZ)/miniz.c

clean:
	rm slide_index *.o

