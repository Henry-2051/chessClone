{
  description = "C++ Development with Nix in 2025";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
  };

  outputs = inputs@{ flake-parts, ... }:
    flake-parts.lib.mkFlake { inherit inputs; } {
      # This is the list of architectures that work with this project
      systems = [
        "x86_64-linux" "aarch64-linux" "aarch64-darwin" "x86_64-darwin"
      ];
      perSystem = { config, self', inputs', pkgs, system, ... }: {

        # devShells.default describes the default shell with C++, cmake, boost,
        # and catch2
        devShells =
        let miscPackages = with pkgs; [
          boost
          cmake
        ];
        graphicsPackages = with pkgs; [
          glfw
          glfw3
          glew
          mesa
          libGL
          stb

          sfml_2
          imgui
        ];
        mathsPackages = with pkgs; [
          glm
        ];
        lspAndTestPackages = with pkgs; [
          clang-tools
          catch2
        ];
        myBuildInputs = with pkgs; [
          bashInteractive
        ];
        sharedAttributes = {
            buildInputs = myBuildInputs;
            packages = miscPackages ++ graphicsPackages ++ mathsPackages ++ lspAndTestPackages ;
            shellHook = ''export SHELL=${pkgs.bashInteractive}/bin/bash'';
        };
        in {
          default = pkgs.mkShell sharedAttributes // {};
          clang = pkgs.mkShell.override { stdenv = pkgs.clangStdenv; }  sharedAttributes // {};
        };
      };
    };
}
