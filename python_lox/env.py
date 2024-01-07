from .exc import LoxUndefinedVariable
import typing as ty

sentinel = object()

if ty.TYPE_CHECKING:
    from .token import Token
else:
    Token = None


class Environment(dict[str, object]):
    def __init__(self, outer: ty.Union["Environment", None] = None):
        self.outer = outer

    def define(self, name: Token, value: object, new=False):
        if name.lexeme in self or new:
            self[name.lexeme] = value
        elif self.outer:
            self.outer.define(name, value)
        else:
            self.undefined(name)

    def getdef(self, name: Token) -> object:
        value = self.get(name.lexeme, sentinel)
        if value is sentinel:
            if self.outer:
                return self.outer.getdef(name)
            self.undefined(name)
        return value

    def undefined(self, name: Token):
        raise LoxUndefinedVariable(name, f"Undefined variable {name.lexeme!r}")
