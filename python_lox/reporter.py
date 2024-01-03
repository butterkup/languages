from sys import stderr as _stderr

eprint = lambda *a, **k: print(*a, **k, file=_stderr)


class Reporter:
    def __init__(self) -> None:
        self.had_error: bool = False

    def report(self, line: int, where: str, message: str):
        self.had_error = True
        eprint(f"[line {line}] Error {where}: {message}")

    def error(self, line: int, message: str):
        self.report(line, "", message)
