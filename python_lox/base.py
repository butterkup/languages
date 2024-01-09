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
    
    def visit_logical(self, expr):
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
    
    def visit_if(self, stmt):
        ...
    
    def visit_while(self, stmt):
        ...


class Expr(typing.Protocol):
    def accept(self, visitor: "ExprVisitor"):
        ...


class Stmt(typing.Protocol):
    def accept(self, visitor: "StmtVisitor"):
        ...

