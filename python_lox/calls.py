from .stmt_ast import Function, Var, Block
from .base import StmtVisitor, Callable
from .expr_ast import Literal
from .base import StmtVisitor
from .exc import ReturnValue, LoxRuntimeError
from .env import Environment
from .token import Token


class LoxInstance:
    def __init__(self, klass: "LoxClass") -> None:
        self.klass = klass
        self.fields = {}

    def get(self, token: "Token"):
        name = token.lexeme
        if name in self.fields:
            return self.fields[name]
        meth = self.klass.get(token, self)
        if meth is not None:
            return meth
        raise LoxRuntimeError(token, f"Cannot access attribute {name!r} of {self!s}")

    def set(self, name: Token, value: object):
        self.fields[name.lexeme] = value
        return value

    def __str__(self) -> str:
        return f"<instance of {self.klass!s}>"


class LoxClass(Callable):
    def __init__(self, name: str, fields: dict) -> None:
        self.fields = fields
        self.name = name

    def call(self, visitor: StmtVisitor, args: list[object]):
        init = self.fields.get("init", None)
        instance = LoxInstance(self)
        if isinstance(init, LoxMethod):
            init.bind(instance).call(visitor, args)
        return instance

    def get(self, token: Token, instance: LoxInstance | None = None):
        name = token.lexeme
        meth = self.fields.get(name, None)
        if isinstance(meth, LoxMethod) and instance is not None:
            return meth.bind(instance)
        return meth

    def set(self, name: Token, value: object):
        self.fields[name.lexeme] = value
        return value

    def arity(self) -> int:
        init = self.fields.get("init", None)
        if isinstance(init, LoxMethod):
            return init.bind(None).arity()
        return 0

    def __str__(self) -> str:
        init = self.fields.get("init", None)
        sig = "()"
        if isinstance(init, LoxMethod):
            func = init.bind(None)
            args = ", ".join(p.lexeme for p in func.function.params)
            sig = "(" + args + ")"
        return f"<class {self.name}{sig}>"


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


class LoxMethod(LoxFunction):
    def bind(self, instance) -> LoxFunction:
        env = Environment(self.closure)
        env["this"] = instance
        return LoxFunction(self.function, env)

    def arity(self) -> int:
        return 1

    def call(self, visitor: StmtVisitor, args: list[object]):
        return self.bind(*args)

    def __str__(self) -> str:
        name = self.function.name.lexeme
        return f"<method {name}(instance)>"
