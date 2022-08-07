CC = clang

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
	NAME := elfReader
	SRC := elf.c
else
	NAME := machoReader
	SRC := macho.c
endif

all: $(NAME)

$(NAME):
	$(CC) $(SRC) -o $(NAME)

re: clean all

clean: 
	rm $(NAME)
