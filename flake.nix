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
          perf
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
          marksman
          icu
          catch2
        ];
        myBuildInputs = with pkgs; [
          bashInteractive
        ];
        thirdPartyChessEngines = with pkgs; [
          stockfish
        ];
        sharedAttributes = {
            buildInputs = myBuildInputs;
            packages = miscPackages ++ graphicsPackages ++ mathsPackages ++ lspAndTestPackages ++ thirdPartyChessEngines;
            shellHook = ''export SHELL=${pkgs.bashInteractive}/bin/bash
                          # dotnet dependency
                          # this line allows marksman to understand global languages intelligently, rather than interpreting everything as an array of bytes
                          # eg can select words from languages without spaces or know that é is just e with an accent and not a completely different letter
                          export LD_LIBRARY_PATH="${pkgs.lib.makeLibraryPath [ pkgs.icu ]}:$LD_LIBRARY_PATH"
            '';
        };
        in {
          default = pkgs.mkShell sharedAttributes // {};
          clang = pkgs.mkShell.override { stdenv = pkgs.clangStdenv; }  sharedAttributes // {};
        };
      };
    };
}
