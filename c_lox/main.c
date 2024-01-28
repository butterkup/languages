#include <lox/all.h>
#include <stdio.h>

void test_scanner() {
  const char *src = "1 + 3 / 5 - 9";
  scanner_init(src);
  Token tk;
  for (;;) {
    tk = scan();
    token_print(&tk);
    if (tk.type == TOKEN_EOF)
      break;
  }
}

bool ola() {
  if (3 > 2)
    return true;
  return false;
}

int main(int argc, char **argv) {
  test_scanner();
  // vm_init();
  // if (argc == 1)
  //   repl();
  // else if (argc == 2)
  //   run_file(argv[1]);
  // else {
  //   fputs("Usage: clox [path]\n", stderr);
  //   exit(64);
  // }
  // vm_delete();
}
