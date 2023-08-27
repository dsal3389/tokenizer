
CC=clang
C_FLAGS=-Wall -o tokenizer
C_SOURCE=src/tokenizer/*.c


build:
	$(CC) $(C_FLAGS) $(C_SOURCE)
