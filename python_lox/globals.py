from .base import Callable, StmtVisitor
from time import monotonic
from .env import Environment
from typing import Protocol


class _nativefn(Callable, Protocol):
    def __str__(self):
        return f"<fn:native>"

    __repr__ = __str__


class Clock(_nativefn):
    def arity(self) -> int:
        return 0

    def call(self, visitor: StmtVisitor, args: list[object]):
        return monotonic()


class PrintMany(_nativefn):
    def arity(self) -> int:
        return -1

    def call(self, visitor: StmtVisitor, args: list[object]):
        return print(*args)


class String(_nativefn):
    def __init__(self, tostr) -> None:
        self.tostr = tostr

    def arity(self) -> int:
        return 1

    def call(self, visitor: StmtVisitor, args: list[object]):
        return self.tostr(*args)


class Env(_nativefn):
    def __init__(self, tostr) -> None:
        self.tostr = tostr

    def arity(self) -> int:
        return 0

    def call(self, visitor: StmtVisitor, _):
        env = visitor.env  # type: ignore
        cnt = 0
        while env is not None:
            _env = {key: self.tostr(value) for key, value in env.items()}
            print(f"{cnt}: {_env}")
            env = env.outer
            cnt += 1


def getglobals(tostr):
    loxglobals = Environment()
    loxglobals["clock"] = Clock()
    loxglobals["env"] = Env(tostr)
    loxglobals["printmany"] = PrintMany()
    loxglobals["string"] = String(tostr)
    return loxglobals
