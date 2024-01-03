from pathlib import Path
from .lexer import Lexer
from .reporter import Reporter, eprint


class Lox:
    def __init__(self) -> None:
        self.reporter = Reporter()

    def run_file(self, file: str | Path, *argv: str):
        content = Path(str(file)).read_text("utf-8")
        self.run(content, *argv)

    def run(self, src: str, *argv: str):
        # Tokenizer -> Tokens
        lexer = Lexer(src, self.reporter)
        tokens = lexer.scan_tokens()
        for token in tokens:
            print(token)
        if self.reporter.had_error:
            eprint("Errors occurred in the tokenizing section, aborting!")
            return
        # Parser -> AST
        # Executor -> TreeWalkInterpreter

    def repr(self, *argv):
        buffer, linenumber, prompt = "", 1, "[   1]> "
        try:
            while True:
                line = input(prompt)
                if line.lower() in ("exit", "quit"):
                    exit(0)
                if line.endswith(":"):
                    buffer += line[:-1] + '\n'
                    prompt = " " * 8
                    continue
                buffer += line
                linenumber += 1
                self.run(buffer, *argv)
                self.reporter.had_error = False
                buffer = ""
                prompt = f"[{str(linenumber).rjust(4)}]> "
        except KeyboardInterrupt:
            exit(1)
        finally:
            print("Bye!!")
