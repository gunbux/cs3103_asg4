{ pkgs ? import <nixpkgs> {} }:

pkgs.mkShell {
  buildInputs = [
    pkgs.openssl
    pkgs.boost
    pkgs.gcc
    pkgs.python3
    pkgs.python3Packages.flask
    pkgs.dotenv
  ];
}
