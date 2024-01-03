from pathlib import Path
from .lexer import Lexer
from .reporter import Reporter


class Lox:
    def __init__(self) -> None:
        self.reporter = Reporter()

    def run_file(self, file: str | Path, *argv: str):
        content = Path(str(file)).read_text("utf-8")
        self.run(content, *argv)

    def run(self, src: str, *argv: str):
        lexer = Lexer(src, self)
        tokens = lexer.scan_tokens()
        for token in tokens:
            print(token)
        if self.reporter.had_error:
            exit(65)

    def repr(self, *argv):
        buffer, linenumber, prompt = "", 1, "[   1]> "
        try:
            while True:
                line = input(prompt)
                if line.lower() in ("exit", "q", "quit"):
                    exit(0)
                if line.endswith(":"):
                    buffer += line[:-1]
                    prompt = "        "
                    continue
                linenumber += 1
                self.run(buffer, *argv)
                buffer = ""
                prompt = f"[{str(linenumber).rjust(4)}]> "
        except KeyboardInterrupt:
            exit(1)
        finally:
            print("Bye!!")
