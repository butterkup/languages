from .token import TokenType
import typing as ty

keywords: ty.Final = {
    "and": TokenType.AND,
    "class": TokenType.CLASS,
    "else": TokenType.ELSE,
    "false": TokenType.FALSE,
    "for": TokenType.FOR,
    "fun": TokenType.FUN,
    "if": TokenType.IF,
    "nil": TokenType.NIL,
    "or": TokenType.OR,
    "print": TokenType.PRINT,
    "return": TokenType.RETURN,
    "super": TokenType.SUPER,
    "this": TokenType.THIS,
    "true": TokenType.TRUE,
    "var": TokenType.VAR,
    "while": TokenType.WHILE,
    "break": TokenType.BREAK,
}

comparison_operators: ty.Final = {
    ">": TokenType.GREATER,
    ">=": TokenType.GREATER_THAN,
    "<": TokenType.LESS,
    "<=": TokenType.LESS_THAN,
}

equality_operators: ty.Final = {"==": TokenType.EQUAL_EQUAL, "!=": TokenType.BANG_EQUAL}

binary_operators: ty.Final = {
    "*": TokenType.STAR,
    "/": TokenType.SLASH,
    "**": TokenType.POW,
    **equality_operators,
    **comparison_operators,
}

hybrid_operators: ty.Final = {
    "+": TokenType.PLUS,
    "-": TokenType.MINUS,
}

unary_operators: ty.Final = {"!": TokenType.BANG}

COMPARISON_OPERATORS: ty.Final[set[TokenType]] = set(comparison_operators.values())
EQUALITY_OPERATORS: ty.Final[set[TokenType]] = set(equality_operators.values())
BINARY_OPERATORS: ty.Final[set[TokenType]] = set(binary_operators.values())
UNARY_OPERATORS: ty.Final[set[TokenType]] = set(unary_operators.values())
HYBRID_OPERATORS: ty.Final[set[TokenType]] = set(hybrid_operators.values())
ALL_BINARY_OPERATORS: ty.Final[set[TokenType]] = HYBRID_OPERATORS | BINARY_OPERATORS
ALL_UNARY_OPERATORS: ty.Final[set[TokenType]] = HYBRID_OPERATORS | UNARY_OPERATORS
