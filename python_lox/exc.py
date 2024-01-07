import typing as ty

if ty.TYPE_CHECKING:
    from .token import Token
else:
    Token = None


class LoxError(Exception):
    def __init__(self, token: Token, message: str) -> None:
        self.token = token
        self.message = message


class LoxRuntimeError(Exception):
    ...


class LoxTypeError(LoxRuntimeError):
    ...


class LoxZeroDivisionError(LoxRuntimeError):
    ...


class LoxUndefinedVariable(LoxRuntimeError):
    ...


class ParserError(LoxError):
    ...


class MissingExpr(ParserError):
    ...
