from .base import Callable, StmtVisitor
from time import time
from .env import Environment
from typing import Protocol


class _nativefn(Callable, Protocol):
    def __str__(self):
        return f"<fn:native>"


class Clock(_nativefn):
    def arity(self) -> int:
        return 0

    def call(self, visitor: StmtVisitor, args: list[object]):
        return time()


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


def getglobals(tostr):
    loxglobals = Environment()
    loxglobals["clock"] = Clock()
    loxglobals["printmany"] = PrintMany()
    loxglobals["string"] = String(tostr)
    return loxglobals
