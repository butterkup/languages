from .stmt_ast import Function, Var, Block
from .base import StmtVisitor, Callable
from .expr_ast import Literal
from .exc import ReturnValue
from .env import Environment


class LoxFunction(Callable):
    def __init__(self, function: Function, env: Environment) -> None:
        self.function = function
        self.closure = env

    def arity(self) -> int:
        return len(self.function.params)

    def call(self, visitor: StmtVisitor, args: list[object]):
        bindings = [
            Var(param, Literal(arg)) for param, arg in zip(self.function.params, args)
        ]
        bound = [*bindings, *self.function.body.statements]
        previous = visitor.env  # type: ignore
        try:
            visitor.env = self.closure  # type: ignore
            return visitor.visit_block(Block(bound))
        except ReturnValue as e:
            return e.return_value
        finally:
            visitor.env = previous  # type: ignore

    def __str__(self) -> str:
        name = self.function.name.lexeme
        args = ", ".join([p.lexeme for p in self.function.params])
        return f"<fn {name}({args})>"

    __repr__ = __str__

    def accept(self, visitor):
        return self.function.accept(visitor)
