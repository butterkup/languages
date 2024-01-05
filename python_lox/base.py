import typing
from .token import TokenType


class Visitor(typing.Protocol):
    def visit_binary(self, expr):
        ...

    def visit_literal(self, expr):
        ...

    def visit_grouping(self, expr):
        ...

    def visit_unary(self, expr):
        ...

    def visit_ternary(self, expr):
        ...


class Expr(typing.Protocol):
    def accept(self, visitor: "Visitor"):
        ...


class ASTPrinter(Visitor):
    def visit_binary(self, expr):
        right = expr.right.accept(self)
        left = expr.left.accept(self)
        return f"{expr.operator.lexeme}({left}, {right})"

    def visit_grouping(self, expr):
        return f"G({expr.expression.accept(self)})"

    def visit_literal(self, expr):
        return str(expr.value)

    def visit_unary(self, expr):
        return f"({expr.operator.lexeme}{expr.right.accept(self)})"

    def visit_ternary(self, expr):
        onfalse = expr.onfalse.accept(self)
        ontrue = expr.ontrue.accept(self)
        cond = expr.condition.accept(self)
        return f"({cond} ? {ontrue} : {onfalse})"
