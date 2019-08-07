CFLAG = -lreadline

all:
	gcc main.c parse.c $(CFLAG) -o main.o

.PHONY: clean
clean:
	rm main.o
