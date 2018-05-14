CC := gcc

INC_DIRS := -I lib/sxml

SRC_F := lib/sxml/sxml.c

SRC_F += src/main.c
SRC_F += lib/sxml/sxmlsearch.c

all:
	$(CC) -o build/xmlc $(INC_DIRS) $(SRC_F)
