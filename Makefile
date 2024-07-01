IDIR=./src
CC=gcc
CFLAGS=-I$(IDIR) -O2 -Wall
LFLAGS = -lX11 -lm -lpthread -lSDL2 -lSDL2_image -lSDL2_mixer
ODIR=obj
SRCDIR=./src

_OBJ = main.o
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))

obj/%.o: $(SRCDIR)/%.cpp
	mkdir -p obj
	$(CC) -c -o $@ $^ $(CFLAGS)

main: $(OBJ)
	$(CC) $^ -o $@ $(CFLAGS) $(LFLAGS)

.PHONY: run
run:
	nix run --override-input nixpkgs nixpkgs/nixos-23.05 --impure github:guibou/nixGL -- ./main --windowed

.PHONY: clean
clean:
	rm -f $(ODIR)/*.o *~ core $(IDIR)/*~ 

.PHONY: shell
shell:
	nix develop

.PHONY: format
format:
	@for k in $(shell find src -name "*.cpp" -o -name "*.h"); do astyle --style=kr --indent=tab=4 $$k ; done
