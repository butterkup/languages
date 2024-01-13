from .base import StmtVisitor, ExprVisitor, Callable, Expr
from .token import TokenType, Token
from .calls import LoxFunction
from .env import Environment
from .exc import *
import typing

if typing.TYPE_CHECKING:
    from .reporter import Reporter
else:
    Reporter = None


class ASTInterpreter(StmtVisitor, ExprVisitor):
    def __init__(
        self,
        reporter: Reporter,
        resolution: dict[Expr, int | None],
        globals: Environment | None = None,
    ) -> None:
        self.reporter = reporter
        self.globals = Environment() if globals is None else globals
        self.env = self.globals
        self.resolution = resolution or {}

    def lookup_variable(self, name: Token, expr: Expr):
        distance = self.resolution.get(expr)
        if distance is None:
            return self.globals.getdef(name)
        return self.env.getAt(distance, name, self.globals)

    def visit_break(self, stmt):
        raise ExitIteration

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

    def interpret(self, statements):
        try:
            for statement in statements:
                statement.accept(self)
        except LoxRuntimeError as e:
            self.reporter.runtime_error(e)

    def visit_expression(self, expr):
        self.evaluate(expr.expression)

    def visit_print(self, expr):
        value = self.evaluate(expr.expression)
        toprint = self.reporter.string(value)
        print(toprint)

    def visit_variable(self, expr):
        return self.lookup_variable(expr.value, expr)

    def visit_var(self, stmt):
        value = self.evaluate(stmt.expression)
        self.env.define(stmt.name, value, True)

    def visit_assign(self, expr):
        value = self.evaluate(expr.expression)
        distance = self.resolution.get(expr, None)
        if distance is None:
            return self.globals.define(expr.name, value)
        self.env.assignAt(distance, expr.name, value)
        return value

    def visit_block(self, stmt):
        previous = self.env
        try:
            self.env = Environment(previous)
            for statement in stmt.statements:
                statement.accept(self)
        finally:
            self.env = previous

    def visit_if(self, stmt):
        value = stmt.condition.accept(self)
        if self.is_truthy(value):
            stmt.then_.accept(self)
        elif stmt.else_ is not None:
            stmt.else_.accept(self)

    def visit_logical(self, expr):
        left = self.evaluate(expr.left)
        if expr.operator.token_type == TokenType.OR:
            if self.is_truthy(left):
                return left
        elif not self.is_truthy(left):
            return left
        return self.evaluate(expr.right)

    def visit_while(self, stmt):
        while self.is_truthy(stmt.condition.accept(self)):
            try:
                stmt.body.accept(self)
            except ExitIteration:
                break

    def visit_call(self, expr):
        callee = self.evaluate(expr.callee)
        if not isinstance(callee, Callable):
            raise LoxTypeError(
                expr.open_paren, f"{self.reporter.string(callee)} is not callable."
            )
        args = [arg.accept(self) for arg in expr.arguments]
        if callee.arity() != -1 and len(args) != callee.arity():
            raise LoxTypeError(
                expr.open_paren,
                f"expected {callee.arity()} arguments but received {len(args)}.",
            )
        return callee.call(self, args)

    def visit_function(self, stmt):
        function = LoxFunction(stmt, self.env)
        self.env.define(stmt.name, function, True)

    def visit_return(self, stmt):
        raise ReturnValue(stmt.value.accept(self))
