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
        try:
            return self.klass.get(token, self)
        except LoxRuntimeError:
            raise LoxRuntimeError(
                token, f"Cannot access attribute {name!r} of {self!s}"
            )

    def set(self, name: Token, value: object):
        self.fields[name.lexeme] = value
        return value

    def __str__(self) -> str:
        return f"<instance of {self.klass!s}>"


class LoxClass(Callable):
    def __init__(
        self, name: str, fields: dict | None = None, base: "LoxClass | None" = None
    ) -> None:
        self.fields = fields or {}
        self.name = name
        self.base = base

    def call(self, visitor: StmtVisitor, args: list[object]):
        init = self.find_init()
        instance = LoxInstance(self)
        if isinstance(init, LoxMethod):
            init.bind(instance).call(visitor, args)
        return instance

    def find_init(self):
        init, base = NotImplemented, self
        while init is NotImplemented and base is not None:
            init = base.fields.get("init", NotImplemented)
            base = base.base
        return None if init is NotImplemented else init

    def get(self, token: Token, instance: LoxInstance | None = None):
        name = token.lexeme
        meth = self.fields.get(name, NotImplemented)
        if meth is NotImplemented:
            if self.base is not None:
                return self.base.get(token, instance)
            raise LoxRuntimeError(
                token, f"Cannot access attribute {name!r} of {self!s}"
            )
        if isinstance(meth, LoxMethod) and instance is not None:
            return meth.bind(instance)
        return meth

    def set(self, name: Token, value: object):
        self.fields[name.lexeme] = value
        return value

    def arity(self) -> int:
        init = self.find_init()
        if isinstance(init, LoxMethod):
            return len(init.function.params)
        return 0

    def __str__(self) -> str:
        init = self.find_init()
        sig = (
            ", ".join(p.lexeme for p in init.function.params)
            if isinstance(init, LoxMethod)
            else ""
        )
        return f"<class {self.name}({sig})>"


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


class BoundSuper:
    def __init__(self, klass, instance=None) -> None:
        self.instance = instance
        self.klass = klass

    def get(self, token):
        return self.klass.get(token, self.instance)

    def set(self, name, value):
        return self.klass.set(name, value)
