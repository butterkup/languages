import enum


class TokenType(enum.IntFlag):
    # Single char tokens
    LEFT_PAREN = enum.auto()
    RIGHT_PAREN = enum.auto()
    LEFT_BRACE = enum.auto()
    RIGHT_BRACE = enum.auto()
    COMMA = enum.auto()
    DOT = enum.auto()
    MINUS = enum.auto()
    PLUS = enum.auto()
    SEMICOLON = enum.auto()
    SLASH = enum.auto()
    STAR = enum.auto()
    # One or two character tokens
    BANG = enum.auto()
    BANG_EQUAL = enum.auto()
    EQUAL = enum.auto()
    EQUAL_EQUAL = enum.auto()
    GREATER = enum.auto()
    GREATER_THAN = enum.auto()
    LESS = enum.auto()
    LESS_THAN = enum.auto()
    # Literals
    IDENTIFIER = enum.auto()
    STRING = enum.auto()
    NUMBER = enum.auto()
    # Keywords
    AND = enum.auto()
    CLASS = enum.auto()
    ELSE = enum.auto()
    FALSE = enum.auto()
    FUN = enum.auto()
    FOR = enum.auto()
    IF = enum.auto()
    NIL = enum.auto()
    OR = enum.auto()
    PRINT = enum.auto()
    RETURN = enum.auto()
    SUPER = enum.auto()
    THIS = enum.auto()
    TRUE = enum.auto()
    VAR = enum.auto()
    WHILE = enum.auto()

    EOF = enum.auto()


class Token:
    def __init__(
        self, token_type: TokenType, lexeme: str, literal: object, line: int
    ) -> None:
        self.token_type = token_type
        self.literal = literal
        self.lexeme = lexeme
        self.line = line

    def __str__(self) -> str:
        return (
            f"Token(type={self.token_type!r}, literal={self.literal!r}"
            f", lexeme={self.lexeme!r}, line={self.line})"
        )

    # For testing purposes.
    __repr__ = __str__

    def __eq__(self, o: "Token"):
        return (
            self.token_type == o.token_type
            and self.literal == o.literal
            and self.lexeme == o.lexeme
            and self.line == self.line
        )
