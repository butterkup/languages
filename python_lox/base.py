import typing

if typing.TYPE_CHECKING:
    from .env import Environment
    from .reporter import Reporter
else:
    Environment = None
    Reporter = None


class ExprVisitor(typing.Protocol):
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

    def visit_variable(self, expr):
        ...

    def visit_assign(self, expr):
        ...


class StmtVisitor(typing.Protocol):
    def visit_print(self, stmt):
        ...

    def visit_expression(self, stmt):
        ...

    def visit_var(self, stmt):
        ...
    
    def visit_block(self, stmt):
        ...


class Expr(typing.Protocol):
    def accept(self, visitor: "ExprVisitor"):
        ...


class ASTPrinter(ExprVisitor):
    def __init__(self, reporter: Reporter, env: Environment) -> None:
        self.env = env
        self.reporter = reporter

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

    def visit_assign(self, expr):
        name = expr.name.lexeme
        value = expr.expression
        return f"Assign({name} = {value.accept(self)})"

    def visit_variable(self, expr):
        name = expr.value.lexeme
        value = self.env.getdef(expr.value)
        return f"Var({name} -> {self.reporter.string(value)})"
