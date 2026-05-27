# GCC compiler and flags
CC = gcc
CFLAGS = -Wall -g

# Target executable name
TARGET = stage1exe

# Object files required
OBJS = driver.o lexer.o parser.o

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

driver.o: driver.c lexer.h parser.h
	$(CC) $(CFLAGS) -c driver.c

lexer.o: lexer.c lexerDef.h
	$(CC) $(CFLAGS) -c lexer.c

parser.o: parser.c parser.h parserDef.h lexer.h
	$(CC) $(CFLAGS) -c parser.c

clean:
	rm -f *.o $(TARGET) clean_source.txt listoferrors_*.txt