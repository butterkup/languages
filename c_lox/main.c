#include <lox/all.h>
#include <stdio.h>

int main(int argc, char **argv) {
  vm_init();
  if (argc == 1)
    repl();
  else if (argc == 2)
    run_file(argv[1]);
  else {
    fputs("Usage: clox [path]\n", stderr);
    exit(64);
  }
  vm_delete();
}
