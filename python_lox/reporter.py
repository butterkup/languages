from sys import stderr as _stderr
from .token import TokenType

__all__ = ("Reporter",)

eprint = lambda *a, **k: print(*a, **k, file=_stderr)


class Reporter:
    def __init__(self) -> None:
        self.had_error: bool = False
        self.had_runtime_error = False

    def report(self, line: int, where: str, message: str):
        self.had_error = True
        self._print(f"[line {line}] Error {where}: {message}")

    def report_error(self, line: int, error: str, message: str):
        self.had_runtime_error = True
        self._print(f"[line {line}]: Error: {error}: {message}")

    def _print(self, msg: str):
        eprint(msg)

    def error(self, line: int, message: str):
        self.report(line, "", message)

    def string(self, value):
        if value is None:
            return "nil"
        elif isinstance(value, bool):
            return str(value).lower()
        elif isinstance(value, str):
            return f'"{value}"'
        return str(value)

    def runtime_error(self, error):
        self.report_error(error.token.line, error.__class__.__name__, error.message)

    def reset(self):
        self.had_error = False
        self.had_runtime_error = False
