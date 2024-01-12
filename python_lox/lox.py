# from .debug import StmtPrinter as ASTInterpreter
from .interpreter import ASTInterpreter
from .reporter import Reporter, eprint
from .globals import getglobals
from .resolver import Resolver
from .env import Environment
from .parser import Parser
from .lexer import Lexer
from pathlib import Path


class Lox:
    def __init__(self) -> None:
        self.reporter = Reporter()
        self.env = Environment(getglobals(self.reporter.string))

    def run_file(self, file: str | Path, *argv: str):
        content = Path(str(file)).read_text("utf-8")
        self.run(content, *argv)
        if self.reporter.had_error:
            exit(65)
        if self.reporter.had_runtime_error:
            exit(70)

    def run(self, src: str, *argv: str):
        # Tokenizer -> Tokens
        lexer = Lexer(src, self.reporter)
        tokens = lexer.scan_tokens()

        if self.reporter.had_error:
            eprint("Errors occurred in the tokenizing stage, aborting!")
            return

        # Parser -> AST
        parser = Parser(tokens, self.reporter)
        program = parser.parse()

        if self.reporter.had_error:
            eprint("Errors occurred in the parsing stage, aborting!")
            return

        resolver = Resolver(self.reporter)
        resolution = resolver.resolve(*program)

        if self.reporter.had_error:
            eprint("Errors occurred in the resolution stage, aborting!")
            return

        # Executor -> TreeWalkInterpreter
        evaluator = ASTInterpreter(self.reporter, resolution, self.env)
        evaluator.interpret(program)

        if self.reporter.had_runtime_error:
            eprint("Errors occurred in the interpreter stage, aborting!")
            return

    def repr(self, *argv):
        buffer, linenumber, prompt = "", 1, "   1 # "
        multi_prompt = 1
        try:
            while True:
                line = input(prompt)
                multi_prompt += 1
                if line.lower() in (".exit", ".quit"):
                    exit(0)
                if line.lower() == ".clear":
                    linenumber += 1
                    print("\033[H\033[2J\033[3J")
                    continue
                if line.endswith(":"):
                    buffer += line[:-1] + "\n"
                    prompt = str(multi_prompt).rjust(5) + "  "
                    continue
                buffer += line
                linenumber += 1
                self.reporter.reset()
                self.run(buffer, *argv)
                multi_prompt = 1
                buffer = ""
                prompt = f"{str(linenumber).rjust(4)} # "
        except KeyboardInterrupt:
            exit(1)
        finally:
            print("Bye!!")
