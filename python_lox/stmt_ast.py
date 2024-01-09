import typing


__all__ = 'Expression', 'Print', 'Var', 'Block', 'If', 'While'

if typing.TYPE_CHECKING:
    from .base import Expr
    from .token import Token
    from .base import StmtVisitor as Visitor
    from .base import Stmt
else:
    Expr = None
    Token = None
    Visitor = None
    Stmt = None


class Expression:
    def __init__(self: 'typing.Self', expression: 'Expr') -> None:
        self.expression = expression

    def accept(self: 'typing.Self', visitor: 'Visitor'):
        return visitor.visit_expression(self)


class Print:
    def __init__(self: 'typing.Self', expression: 'Expr') -> None:
        self.expression = expression

    def accept(self: 'typing.Self', visitor: 'Visitor'):
        return visitor.visit_print(self)


class Var:
    def __init__(self: 'typing.Self', name: 'Token', expression: 'Expr') -> None:
        self.name = name
        self.expression = expression

    def accept(self: 'typing.Self', visitor: 'Visitor'):
        return visitor.visit_var(self)


class Block:
    def __init__(self: 'typing.Self', statements: 'list') -> None:
        self.statements = statements

    def accept(self: 'typing.Self', visitor: 'Visitor'):
        return visitor.visit_block(self)


class If:
    def __init__(self: 'typing.Self', condition: 'Expr', then_: 'Stmt', else_: 'Stmt|None') -> None:
        self.condition = condition
        self.then_ = then_
        self.else_ = else_

    def accept(self: 'typing.Self', visitor: 'Visitor'):
        return visitor.visit_if(self)


class While:
    def __init__(self: 'typing.Self', condition: 'Expr', body: 'Stmt') -> None:
        self.condition = condition
        self.body = body

    def accept(self: 'typing.Self', visitor: 'Visitor'):
        return visitor.visit_while(self)
