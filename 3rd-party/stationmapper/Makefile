# Определение операционной системы
ifeq ($(OS),Windows_NT)
    LIB_EXT = dll
    EXE_EXT = .exe
    RM = del /Q
else
    LIB_EXT = so
    EXE_EXT =
    RM = rm -f
endif

LIB_NAME = libstationmapper
LIB_FILE = $(LIB_NAME).$(LIB_EXT)
EXAMPLE = example$(EXE_EXT)

run: $(EXAMPLE)
ifeq ($(OS),Windows_NT)
	$(EXAMPLE) data\map_1.bmp data\map_1.csv data\stations_1.csv
else
	./$(EXAMPLE) data/map_1.bmp data/map_1.csv data/stations_1.csv
endif

stationmapper.o: src/stationmapper.c
	gcc -c -fpic src/stationmapper.c -I include

$(LIB_FILE): stationmapper.o
	gcc -shared -o $(LIB_FILE) stationmapper.o

example: example_src/example.c $(LIB_FILE)
	gcc -L. -o $(EXAMPLE) example_src/example.c -lstationmapper -I include

clean:
	$(RM) stationmapper.o $(LIB_FILE) $(EXAMPLE) 2>NUL || true

all: clean example run
