from .core import Class, Module, Import

__all__ = 'strcodegen', 'parseline'

def parseline(line: str) -> Class:
    targets = line.split(":")
    _properties = targets.pop().strip()
    classname = targets.pop(0).strip()
    bases = [base.strip() for base in targets.pop(0).split(",")] if targets else []
    properties = {}
    for property in _properties.split(","):
        if not property:
            continue
        attr = property.split(" ")
        properties[attr[1].strip()] = attr[0].strip()
    return Class(classname, *bases, **properties)


def strcodegen(file: str, MAIN):
    with open(file) as f:
        lines = f.readlines()
    output = lines.pop(0).split(":")
    module = output.pop(0)
    imports = []
    for imp in output:
        imps = imp.split(",")
        typed = False
        if len(imps) == 4:
            typed = True
        elif len(imps) == 2:
            imps.append(None)  # type: ignore
        elif len(imps) == 1:
            imps.insert(0, None)  # type: ignore
            imps.append(None)  # type: ignore
        imports.append(Import(imps[1], imps[0], imps[2], typing=typed))
    classes = [parseline(line) for line in lines]
    exports = [class_.name for class_ in classes]
    mod = Module(module, imports=imports, exports=exports)
    with open(MAIN / module, "w") as f:
        f.write(mod.create(*classes))
