from .interpreter import ASTInterpreter
from .reporter import Reporter, eprint
from .base import ASTPrinter
from .parser import Parser
from pathlib import Path
from .lexer import Lexer


class Lox:
    def __init__(self) -> None:
        self.reporter = Reporter()

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
        # for token in tokens:
        #     print(token)
        if self.reporter.had_error:
            eprint("Errors occurred in the tokenizing stage, aborting!")
            return

        # Parser -> AST
        parser = Parser(tokens, self.reporter)
        ast = parser.parse()

        if self.reporter.had_error:
            eprint("Errors occurred in the parsing stage, aborting!")
            return

        # Executor -> TreeWalkInterpreter
        if ast is not None:
            evaluator = ASTInterpreter(self.reporter)
            value = evaluator.interpret(ast)

            if self.reporter.had_runtime_error:
                eprint("Errors occurred in the interpreter stage, aborting!")
                return

            # Output the result of the expression
            print(value)

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
