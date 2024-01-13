import typing as ty

if ty.TYPE_CHECKING:
    from .token import Token
else:
    Token = None


class BaseLoxError(Exception):
    ...


class LoxError(BaseLoxError):
    def __init__(self, token: Token, message: str) -> None:
        self.token = token
        self.message = message


class LoxRuntimeError(LoxError):
    ...


class ReturnValue(BaseLoxError):
    def __init__(self, value) -> None:
        self.return_value = value


class NextIteration(BaseLoxError):
    ...


class ExitIteration(BaseLoxError):
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
