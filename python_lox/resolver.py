from .base import ExprVisitor, StmtVisitor, Stmt, Expr
from typing import Any
import typing as ty
from collections import defaultdict

if ty.TYPE_CHECKING:
    from . import expr_ast as east
    from . import stmt_ast as sast
    from .reporter import Reporter
    from .token import Token
else:
    ASTInterpreter = Reporter = None

    class _None:
        def __getattr__(self, default=None):
            return None

        def __setattr__(self, name: str, value: Any) -> None:
            return value

        def __getattribute__(self, __name: str) -> Any:
            return None

    east = sast = Token = _None()


class _Ctx:
    def __init__(self, ctx: "Context", value: str):
        self.ctx = ctx
        self.value = value

    def __enter__(self):
        self.ctx.context[self.value] += 1
        return self.ctx

    def __exit__(self, et, ev, tb):
        self.ctx.context[self.value] -= 1


class Context:
    def __init__(self) -> None:
        self.context: dict[str, int] = defaultdict(lambda: 0)

    def within(self, ctx: str):
        return _Ctx(self, ctx)

    def inside(self, ctx: str) -> bool:
        return self.context[ctx] > 0

    __contains__ = inside


CLASS = "CLASS"
FUNCTION = "FUNCTION"


class Scopes:
    def __init__(self, reporter: Reporter):
        self.reporter = reporter
        self.scopes: list[dict[str, bool]] = list()

    def __len__(self):
        return len(self.scopes)

    def __bool__(self):
        return bool(self.scopes)

    @property
    def scope(self):
        return self.scopes[-1]

    def declare(self, name: Token):
        if self.scopes:
            if name.lexeme in self.scope:
                self.reporter.error(
                    name.line,
                    f"Variable {name.lexeme!r} already declared in its scope.",
                )
            self.scope[name.lexeme] = False

    def define(self, name: Token):
        if self.scopes and name.lexeme in self.scope:
            self.scope[name.lexeme] = True

    def __enter__(self):
        scope = dict()
        self.scopes.append(scope)
        return scope

    def __exit__(self, et, ev, tb):
        self.scopes.pop()

    def resolve(self, name: Token) -> int | None:
        for index, scope in zip(range(len(self.scopes)), reversed(self.scopes)):
            if scope.get(name.lexeme, None) == True:
                return index


class Resolver(ExprVisitor, StmtVisitor):
    def __init__(self, reporter: Reporter) -> None:
        self.ctx = Context()
        self.reporter = reporter
        self.scopes = Scopes(reporter)
        self.define = self.scopes.define
        self.locals: dict[Expr, int] = {}
        self.declare = self.scopes.declare

    @property
    def scope(self):
        return self.scopes.scope

    def _resolve(self, *statements: Stmt | Expr):
        for statement in statements:
            statement.accept(self)

    def resolve_local(self, expr: Expr, name: Token):
        distance = self.scopes.resolve(name)
        if distance is not None:
            self.locals[expr] = distance

    def resolve_function(self, func: sast.Function):
        with self.scopes, self.ctx.within(FUNCTION):
            for param in func.params:
                self.declare(param)
                self.define(param)
            self._resolve(*func.body.statements)

    def visit_block(self, stmt: sast.Block):
        with self.scopes:
            self._resolve(*stmt.statements)

    def visit_var(self, stmt: sast.Var):
        self.declare(stmt.name)
        self._resolve(stmt.expression)
        self.define(stmt.name)

    def visit_variable(self, expr: east.Variable):
        self.resolve_local(expr, expr.value)

    def visit_assign(self, expr: east.Assign):
        self._resolve(expr.expression)
        self.resolve_local(expr, expr.name)

    def visit_function(self, stmt: sast.Function):
        self.declare(stmt.name)
        self.define(stmt.name)
        self.resolve_function(stmt)

    def visit_expression(self, stmt: sast.Expression):
        self._resolve(stmt.expression)

    def visit_if(self, stmt: sast.If):
        self._resolve(stmt.condition, stmt.then_)
        if stmt.else_ is not None:
            self._resolve(stmt.else_)

    def visit_print(self, stmt: sast.Print):
        self._resolve(stmt.expression)

    def visit_return(self, stmt: sast.Return):
        if FUNCTION not in self.ctx:
            self.reporter.error(
                stmt.keyword.line,
                "'return' statement must be used inside a function or method.",
            )
        self._resolve(stmt.value)

    def visit_while(self, stmt: sast.While):
        self._resolve(stmt.condition, stmt.body)

    def visit_binary(self, expr: ty.Union[east.Binary, east.Logical]):
        self._resolve(expr.right, expr.left)

    visit_logical = visit_binary

    def visit_call(self, expr: east.Call):
        self._resolve(expr.callee, *expr.arguments)

    def visit_grouping(self, expr: east.Grouping):
        self._resolve(expr.expression)

    def visit_literal(self, _: east.Literal):
        ...

    def visit_ternary(self, expr: east.Ternary):
        self._resolve(expr.condition, expr.ontrue, expr.onfalse)

    def visit_unary(self, expr: east.Unary):
        self._resolve(expr.right)

    def resolve(self, *root: Expr | Stmt):
        self._resolve(*root)
        return self.locals
