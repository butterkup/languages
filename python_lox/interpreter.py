from .token import TokenType, Token
from .reporter import Reporter
from .base import Visitor
import typing


class LoxRuntimeError(Exception):
    def __init__(self, operator: Token, message: str) -> None:
        self.operator = operator
        self.message = message


class LoxTypeError(LoxRuntimeError):
    ...


class LoxZeroDivisionError(LoxRuntimeError):
    ...


class ASTInterpreter(Visitor):
    def __init__(self, reporter: Reporter) -> None:
        self.reporter = reporter

    def visit_binary(self, expr):
        right = self.evaluate(expr.right)
        left = self.evaluate(expr.left)

        match expr.operator.token_type:
            case TokenType.STAR:
                self.check_numbers(expr.operator, right, left)
                return left * right
            case TokenType.PLUS:
                if (isinstance(right, str) and isinstance(left, str)) or (
                    isinstance(right, (float, int)) and isinstance(left, (float, int))
                ):
                    return left + right  # type: ignore
                raise LoxTypeError(
                    expr.operator,
                    "Binary operator (+) expected strings or "
                    f"numbers, got {right!r} and {left!r}",
                )
            case TokenType.MINUS:
                self.check_numbers(expr.operator, right, left)
                return left - right
            case TokenType.POW:
                self.check_numbers(expr.operator, right, left)
                return left**right
            case TokenType.SLASH:
                try:
                    self.check_numbers(expr.operator, right, left)
                    return left / right
                except ZeroDivisionError as e:
                    raise LoxZeroDivisionError(
                        expr.operator, f"division by zero at {left} / {right}"
                    ) from e
            case TokenType.GREATER:
                self.check_numbers(expr.operator, right, left)
                return left > right
            case TokenType.GREATER_THAN:
                self.check_numbers(expr.operator, right, left)
                return left >= right
            case TokenType.LESS:
                self.check_numbers(expr.operator, right, left)
                return left < right
            case TokenType.LESS_THAN:
                self.check_numbers(expr.operator, right, left)
                return left <= right
            case TokenType.EQUAL_EQUAL:
                return left == right
            case TokenType.BANG_EQUAL:
                return left != right

        typing.assert_never(expr.operator.token_type)

    def visit_grouping(self, expr):
        return self.evaluate(expr.expression)

    def visit_literal(self, expr):
        return expr.value

    def visit_ternary(self, expr):
        condition = self.evaluate(expr.condition)
        if self.is_truthy(condition):
            return self.evaluate(expr.ontrue)
        return self.evaluate(expr.onfalse)

    def visit_unary(self, expr):
        right = self.evaluate(expr.right)
        match expr.operator.token_type:
            case TokenType.MINUS:
                self.check_numbers(expr.operator, right, 0)
                return -right
            case TokenType.PLUS:
                self.check_numbers(expr.operator, right, 0)
                return +right
            case TokenType.BANG:
                return not self.is_truthy(right)
        typing.assert_never(expr.token_type)

    def is_truthy(self, value) -> bool:
        if value is None:
            return False
        if isinstance(value, bool):
            return value
        return True

    def evaluate(self, expr):
        return expr.accept(self)

    def _check_type(self, operator, stype, right, left, *types):
        rok = not isinstance(right, types)
        lok = not isinstance(left, types)
        target, got = "", []
        if rok and lok:
            target = "left and right"
            got = [left, right]
        elif lok:
            target = "left"
            got = [left]
        elif rok:
            target = "right"
            got = [right]
        if target:
            got = str(got)[1:-1]
            raise LoxTypeError(
                operator, f"Expected a {stype} {target} operand, received {got}"
            )

    def check_numbers(self, operator: Token, right, left):
        self._check_type(operator, "number", right, left, float, int)

    def check_strings(self, operator: Token, right, left):
        self._check_type(operator, "string", right, left, str)

    def interpret(self, expr):
        try:
            return self.reporter.string(self.evaluate(expr))
        except LoxRuntimeError as e:
            self.reporter.runtime_error(e)
