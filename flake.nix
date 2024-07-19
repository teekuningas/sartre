{
  description = "Sartre";
  inputs = {
    flake-utils.url = "github:numtide/flake-utils";
    nixpkgs.url = "github:NixOS/nixpkgs/23.05";
  };
  outputs = { self, nixpkgs, flake-utils }:
    flake-utils.lib.eachDefaultSystem (system:
      let pkgs = nixpkgs.legacyPackages.${system};
      in {
        formatter = pkgs.nixfmt;
        devShells.default = pkgs.mkShell rec {
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
          LD_LIBRARY_PATH = pkgs.lib.makeLibraryPath buildInputs;
          CPATH = "${pkgs.SDL2.dev}/include/SDL2:${pkgs.SDL2_image}/include/SDL2:${pkgs.SDL2_mixer.dev}/include/SDL2:${pkgs.SDL2_ttf}/include/SDL2";
        };
      });
}

