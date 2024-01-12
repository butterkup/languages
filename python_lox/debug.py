from .base import ExprVisitor, StmtVisitor
import typing as ty

if ty.TYPE_CHECKING:
    from .reporter import Reporter
    from .env import Environment
else:
    Reporter = Environment = None

PARENS = False


def parens(func):
    parens = lambda *a, **k: "(" + func(*a, **k) + ")"
    return parens if PARENS else func


class ExprPrinter(ExprVisitor):
    def __init__(self, reporter: Reporter, env: Environment) -> None:
        self.env = env
        reporter.quoted = True
        self.reporter = reporter
        self.depth = 0

    @property
    def dent(self) -> str:
        return "  " * self.depth

    @parens
    def visit_binary(self, expr):
        right = expr.right.accept(self)
        left = expr.left.accept(self)
        return f"{left} {expr.operator.lexeme} {right}"

    def visit_grouping(self, expr):
        return f"({expr.expression.accept(self)})"

    def visit_literal(self, expr):
        return self.reporter.string(expr.value)

    @parens
    def visit_unary(self, expr):
        return f"{expr.operator.lexeme}{expr.right.accept(self)}"

    @parens
    def visit_ternary(self, expr):
        onfalse = expr.onfalse.accept(self)
        ontrue = expr.ontrue.accept(self)
        cond = expr.condition.accept(self)
        return f"{cond} ? {ontrue} : {onfalse}"

    @parens
    def visit_assign(self, expr):
        name = expr.name.lexeme
        value = expr.expression
        return f"{name} = {value.accept(self)}"

    def visit_variable(self, expr):
        return expr.value.lexeme

    def visit_logical(self, expr):
        return self.visit_binary(expr)

    def visit_call(self, expr):
        callee = expr.callee.accept(self)
        args = [arg.accept(self) for arg in expr.arguments]
        return f"{callee}({', '.join(args)})"


class StmtPrinter(ExprPrinter, StmtVisitor):
    def visit_block(self, stmt):
        self.depth += 1
        lines = [self.dent + line.accept(self) for line in stmt.statements]
        self.depth -= 1
        lines = ["{", *lines, self.dent + "}"]
        return "\n".join(lines)

    def visit_if(self, stmt):
        cond = stmt.condition.accept(self)
        then = stmt.then_.accept(self)
        else_ = "" if stmt.else_ is None else f"else {stmt.else_.accept(self)}"
        return f"if ({cond}) {then.lstrip()} {else_.lstrip()}"

    def visit_print(self, stmt):
        return f"print {stmt.expression.accept(self)};"

    def visit_var(self, stmt):
        return f"var {stmt.name.lexeme} = {stmt.expression.accept(self)};"

    def visit_expression(self, stmt):
        return stmt.expression.accept(self) + ";"

    def visit_while(self, stmt):
        return (
            f"while ({stmt.condition.accept(self)}) {stmt.body.accept(self).lstrip()}"
        )

    def interpret(self, program):
        print("\n".join([s.accept(self) for s in program]))

    def visit_function(self, stmt):
        name = stmt.name.lexeme
        body = " " + stmt.body.accept(self) if stmt.body.statements else "{}"
        params = [param.lexeme for param in stmt.params]
        return f'fun {name}({", ".join(params)}){body}'

    def visit_return(self, stmt):
        return f"return {stmt.value.accept(self)};"
