CC = g++
LFLAGS = -lX11 -lm -lpthread -lSDL2 -lSDL2_image -lSDL2_mixer -lSDL2_ttf -lGL -lGLEW
ODIR = obj
SRCDIR = ./src

_OBJ = main.o shader_utils.o matrix.o
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))

$(ODIR)/%.o: $(SRCDIR)/%.cpp
	@mkdir -p $(ODIR)
	$(CC) -c -o $@ $^ $(CFLAGS)

# Compile main
main: $(OBJ)
	$(CC) $^ -o $@ $(CFLAGS) $(LFLAGS)

.PHONY: run
run:
	nix run --override-input nixpkgs nixpkgs/nixos-23.05 --impure github:guibou/nixGL -- ./main

.PHONY: run
run_fullscreen:
	nix run --override-input nixpkgs nixpkgs/nixos-23.05 --impure github:guibou/nixGL -- ./main --fullscreen


.PHONY: smoketest
smoketest:
	nix run --override-input nixpkgs nixpkgs/nixos-23.05 --impure github:guibou/nixGL -- ./main --smoke

.PHONY: shell
shell:
	nix develop

.PHONY: format
format:
	@for k in $(shell find src -name "*.cpp" -o -name "*.h"); do astyle --style=kr --indent=tab=4 $$k ; done
