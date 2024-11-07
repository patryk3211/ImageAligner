{
  outputs = { nixpkgs, ... }: let
    system = "x86_64-linux";

    pkgs = (import nixpkgs { inherit system; });
    lib = nixpkgs.lib;
    
    llvm = pkgs.llvmPackages;
    libs = "";

    buildSrc = ''
      #!/usr/bin/env bash
      cmake --build $(realpath build)
    '';

    reconfSrc = ''
      #!/usr/bin/env bash
      cmake -S . -B build -G Ninja -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
    '';
  in {
    devShells.${system}.default = pkgs.mkShell.override {
      stdenv = pkgs.clangStdenv;
    } {
      nativeBuildInputs = with pkgs; [
        cmake ninja pkg-config
      ];
      
      buildInputs = with pkgs; [
        clang-tools
        gtk4 glib pcre2 util-linux libselinux libsepol fribidi libthai libdatrie expat xorg.libXdmcp lerc gtkmm4 blueprint-compiler
        libadwaita appstream
        cfitsio
        # gtk3 cfitsio gsl exiv2 gnuplot opencv fftwFloat librtprocess wcslib
        # libconfig libraw libtiff libpng libjpeg libheif ffms ffmpeg json-glib
        # libjxl libxisf libgit2 curl

      ];

      CXXFLAGS = builtins.concatStringsSep " " [
        (lib.removeSuffix "\n" (builtins.readFile "${llvm.clang}/nix-support/cc-cflags"))
        (lib.removeSuffix "\n" (builtins.readFile "${llvm.clang}/nix-support/libc-cflags"))
        (lib.removeSuffix "\n" (builtins.readFile "${llvm.clang}/nix-support/libcxx-cxxflags"))
      ];

      shellHook = ''
        export CXXFLAGS+=$NIX_CFLAGS_COMPILE

        export PATH="$(realpath build):$PATH"

        cmake -S . -B build -G Ninja -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
        if [[ ! -e build/reconf ]]; then
          printf "${reconfSrc}" > build/reconf
          chmod +x build/reconf
        fi
        if [[ ! -e build/build ]]; then
          printf "${buildSrc}" > build/build
          chmod +x build/build
        fi
        if [[ ! -e compile_commands.json ]]; then
          ln -s build/compile_commands.json
        fi
      '';
    };
  };
}

