from .exc import LoxUndefinedVariable
import typing as ty

sentinel = object()

if ty.TYPE_CHECKING:
    from .token import Token
else:
    Token = None

_T = ty.TypeVar("_T")


class Environment(dict[str, object]):
    def __init__(self, outer: ty.Union["Environment", None] = None):
        self.outer = outer

    def define(self, name: Token, value: object, new=False):
        if new or (name.lexeme in self):
            self[name.lexeme] = value
        elif self.outer is not None:
            self.outer.define(name, value)
        else:
            self.undefined(name)

    def getdef(self, name: Token) -> object:
        value = self.get(name.lexeme, sentinel)
        if value is sentinel:
            if self.outer is not None:
                return self.outer.getdef(name)
            self.undefined(name)
        return value

    def undefined(self, name: Token):
        raise LoxUndefinedVariable(name, f"Undefined variable {name.lexeme!r}")

    def getAt(self, distance: int, name: Token, other):
        env = self.ancestor(distance)
        if name.lexeme in env:
            return env[name.lexeme]
        if other is not None and name.lexeme in other:
            return other[name.lexeme]
        self.undefined(name)

    def assignAt(self, distance: int, name: Token, value: ty.Any):
        env = self.ancestor(distance)
        if name.lexeme not in env:
            self.undefined(name)
        env[name.lexeme] = value

    def ancestor(self, distance: int) -> "Environment":
        env = self
        while distance > 0 and env.outer is not None:
            env, distance = env.outer, distance - 1
        return env


sentinel = object()


class Bucket:
    def __init__(self, init=sentinel) -> None:
        self._value = init

    def get(self):
        return self._value

    def set(self, value):
        self._value = value

    def empty(self):
        return self._value is sentinel

    def clear(self):
        self._value = sentinel


class Env:
    def __init__(self, outer: "Env | None" = None) -> None:
        self._outer = outer
        self._env: dict[str, Bucket] = dict()

    def declare(self, name: str, bucket: None | Bucket = None):
        bucket = bucket or Bucket()
        if name in self._env:
            raise Exception(f"Env.declare: {name}")
        self._env[name] = bucket
        return bucket

    def resolve_bucket(self, name: str) -> Bucket:
        if name in self._env:
            return self._env[name]
        if self._outer is None:
            raise Exception(f"Env.resolve_bucket: {name}")
        return self._outer.resolve_bucket(name)

    def set(self, name: str, value: ty.Any):
        bucket = self.resolve_bucket(name)
        bucket.set(value)

    def get(self, name: str) -> ty.Any:
        bucket = self.resolve_bucket(name)
        return bucket.get()
