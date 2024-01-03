from python_lox import Lox


def main(argv: list[str]):
    lox = Lox()
    if not argv or argv[0] == "-":
        lox.repr(*argv[1:])
    else:
        lox.run_file(*argv)


if __name__ == "__main__":
    from sys import argv

    main(argv[1:])
