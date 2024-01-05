__all__ = "Class", "Module", "Import"

MODULE = """
import typing
{imports}
{exports}

if typing.TYPE_CHECKING:
{tyimports}
else:
{tyimportdefs}


{body}
""".lstrip()


def classdef(name: str, *bases: str):
    baseclasses = f'({", ".join(bases)})' if bases else ""
    return f"class {name}{baseclasses}:"


def createarg(argname: str, argtype: str | None = None):
    argtype = "object" if argtype is None else argtype
    typehint = f": {argtype!r}" if argtype else ""
    return argname if argtype == "" else f"{argname}{typehint}"


def createargs(*args: str, **kwargs: str | None):
    kwargs.update({arg: None for arg in args})
    return ", ".join(createarg(name, type) for name, type in kwargs.items())


def functiondef(name: str, return_: str | None = None, /, **kwargs: str | None):
    if return_ is None or return_:
        return_ = f" -> {return_}"
    argssig = createargs(**kwargs)
    return f"def {name}({argssig}){return_}:"


def classinit(*args: str, **kwargs: str | None):
    self = kwargs.pop('kwargs', 'typing.Self')
    kwargs.update({arg: None for arg in args})
    return functiondef("__init__", None, self=self, **kwargs)


def attrdef(
    name: str,
    value: str | None = None,
    typehint: str | None = None,
    target: str | None = None,
):
    typehint = f": {typehint}" if typehint else ""
    target = f"{target}." if target else ""
    return f"{target}{name}{typehint} = {value or name}"


def importdef(import_: str, from_: str | None = None, as_: str | None = None):
    ismt = f"import {import_}"
    ismt = f"from {from_} {ismt}" if from_ else ismt
    return f"{ismt} as {as_}" if as_ else ismt


class Class:
    def __init__(self, name: str, /, *bases: str, **args: str) -> None:
        self.args: dict[str, str] = args
        self.bases: tuple[str, ...] = bases
        self.name: str = name

    def with_args(self, **kwargs: str):
        self.args.update(**kwargs)

    def create(self, indent: str = "    "):
        clsdef = classdef(self.name, *self.bases)
        clsinit = indent + classinit(**self.args)
        initindent = indent * 2
        attrs = [initindent + attrdef(name, target="self") for name in self.args]
        attrs = attrs or (initindent + "pass",)
        func = indent + functiondef("accept", '', self='typing.Self', visitor="Visitor") + "\n"
        func += initindent + f"return visitor.visit_{self.name.lower()}(self)"
        return "\n".join((clsdef, clsinit, *attrs, '\n' + func))


class Import:
    def __init__(
        self,
        import_: str,
        from_: str | None = None,
        as_: str | None = None,
        typing: bool | None = None,
    ) -> None:
        if "." in import_:
            if from_ is not None:
                raise ValueError(
                    f"import {import_} has a '.' and from_ {from_} was provided"
                )
            from_, _, import_ = import_.rpartition(".")
        self._import = import_
        self._from = from_
        self._as = as_
        self.typing = typing or False

    def statement(self) -> str:
        return importdef(self._import, self._from, self._as)

    @property
    def name(self) -> str:
        return self._as if self._as else self._import


class Module:
    def __init__(
        self,
        module: str,
        *,
        exports: list[str] | None = None,
        imports: list[Import] | None = None,
        indent: str | None = None,
    ) -> None:
        self.module = module
        self.exports = exports or []
        self.imports = imports or []
        self.indent = indent or "    "

    def create(self, *classes: Class):
        tyimports, glimports = [], []
        for import_ in self.imports:
            imports = tyimports if import_.typing else glimports
            imports.append(import_)
        timp = (
            "\n".join(self.indent + imp.statement() for imp in tyimports)
            or self.indent + "pass"
        )
        impalias = (
            "\n".join(f"{self.indent + imp.name} = None" for imp in tyimports)
            or self.indent + "pass"
        )
        imports = "\n".join(imp.statement() for imp in glimports)
        body = [class_.create(self.indent) for class_ in classes]
        exports = f"\n__all__ = {str(self.exports)[1:-1]}" if self.exports else ""
        return MODULE.format(
            body="\n\n\n".join(body),
            imports=imports,
            exports=exports,
            tyimportdefs=impalias,
            tyimports=timp,
        )
