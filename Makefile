CC := gcc

INC_DIRS := -I lib/yxml

SRC_F := lib/yxml/yxml.c

SRC_F += src/main.c

all:
	$(CC) -o build/xmlc $(INC_DIRS) $(SRC_F)
