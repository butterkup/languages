from .line2py import strcodegen
from sys import argv
from pathlib import Path


CWD = Path(__file__).parent
TARGETS = CWD / 'targets'
MAIN = CWD.parent

if __name__ == "__main__":
    for target in argv[1:]:
        path = TARGETS / target
        if not path.exists():
            print(f"Codegen rule for {path!s} was not found.")
        else:
            strcodegen(str(path), MAIN)
