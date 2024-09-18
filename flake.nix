{
  description = "Sartre";
  inputs = {
    flake-utils.url = "github:numtide/flake-utils";
    nixpkgs.url = "github:NixOS/nixpkgs/23.05";
  };
  outputs = { self, nixpkgs, flake-utils }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = nixpkgs.legacyPackages.${system};
        buildInputs = with pkgs; [
          SDL2.dev
          SDL2_image
          SDL2_mixer.dev
          SDL2_ttf
          xorg.libX11
          gcc
          astyle
          alsa-lib
        ];
        ldLibraryPath = pkgs.lib.makeLibraryPath buildInputs;
        cpath = "${pkgs.SDL2.dev}/include/SDL2:${pkgs.SDL2_image}/include/SDL2:${pkgs.SDL2_mixer.dev}/include/SDL2:${pkgs.SDL2_ttf}/include/SDL2";
      in {
        formatter = pkgs.nixfmt;

        devShells.default = pkgs.mkShell {
          buildInputs = buildInputs;
          LD_LIBRARY_PATH = ldLibraryPath;
          CPATH = cpath;
        };

        packages.default = pkgs.stdenv.mkDerivation {
          pname = "sartre";
          version = "0.1.0";

          src = ./.;

          buildInputs = buildInputs;

          buildPhase = ''
            export LD_LIBRARY_PATH=${ldLibraryPath}
            export CPATH=${cpath}
            mkdir -p obj
            make
          '';

          installPhase = ''
            mkdir -p $out/bin
            cp main $out/bin/sartre
          '';

        };

        defaultApp = {
          type = "app";
          program = "${self.packages.${system}.default}/bin/sartre";
        };
      });
}
