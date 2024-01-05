from .reporter import Reporter
from .token import Token, TokenType
from .ast import Binary, Unary, Literal, Grouping, Ternary
from .category import BINARY_OPERATORS, COMPARISON_OPERATORS, EQUALITY_OPERATORS


class ParserError(Exception):
    def __init__(self, token: Token, message: str) -> None:
        super().__init__(self, message, token)
        self.message = message
        self.token = token


class MissingExpr(ParserError):
    ...


class Parser:
    def __init__(self, tokens: list[Token], reporter: Reporter) -> None:
        self.reporter = reporter
        self.tokens = tokens
        self.current = 0

    def parse(self):
        try:
            if self.empty():
                return
            self.current = 0
            return self.expression()
        except ParserError as e:
            ...

    def expression(self):
        if self.peek().token_type in BINARY_OPERATORS:
            raise self.error(
                self.peek(),
                f"Left operand for binary operation ({self.peek().lexeme}) missing.",
            )
        return self.ternary()

    def ternary(self):
        expr = self.equality()
        if self.match(TokenType.QMARK):
            ontrue = self.equality()
            self.consume(
                TokenType.COLON,
                f"Expected a : to complete the ternary, got {self.peek().lexeme}",
            )
            onfalse = self.equality()
            return Ternary(expr, ontrue, onfalse)
        return expr

    def _binary(self, nexttoken, *optypes: TokenType):
        expr = nexttoken()
        while self.match(*optypes):
            operator = self.previous()
            try:
                right = nexttoken()
                expr = Binary(expr, operator, right)
            except MissingExpr:
                self.reporter.error(
                    operator.line,
                    f"Right operand for binary operation ({operator.lexeme}) missing.",
                )
                raise
        return expr

    def equality(self):
        return self._binary(self.comparison, *EQUALITY_OPERATORS)

    def comparison(self):
        return self._binary(self.term, *COMPARISON_OPERATORS)

    def term(self):
        return self._binary(self.factor, TokenType.MINUS, TokenType.PLUS)

    def factor(self):
        return self._binary(self.pow, TokenType.STAR, TokenType.SLASH)

    def pow(self):
        return self._binary(self.unary, TokenType.POW)

    def unary(self):
        if self.match(TokenType.MINUS, TokenType.BANG, TokenType.PLUS):
            operator = self.previous()
            try:
                right = self.unary()
                return Unary(operator, right)
            except MissingExpr:
                self.reporter.error(
                    operator.line,
                    f"Right operand for unary operator ({operator.lexeme}) missing.",
                )
                raise
        return self.primary()

    def primary(self):
        if self.match(TokenType.FALSE):
            return Literal(False)
        if self.match(TokenType.TRUE):
            return Literal(True)
        if self.match(TokenType.STRING, TokenType.NUMBER, TokenType.NIL):
            return Literal(self.previous().literal)
        if self.match(TokenType.LEFT_PAREN):
            expr = self.expression()
            self.consume(TokenType.RIGHT_PAREN, "Expected ')' after expression.")
            return Grouping(expr)
        raise self.error(
            self.peek(),
            f"Expected an expression, got {self.peek().lexeme!r}",
            MissingExpr,
        )

    def error(self, token: Token, message: str, Error: type[ParserError] = ParserError):
        self.reporter.error(token.line, message)
        return Error(token, message)

    def consume(self, token_type: TokenType, message: str):
        if self.check(token_type):
            return self.advance()
        raise self.error(self.peek(), message)

    def previous(self) -> Token:
        return self.tokens[self.current - 1]

    def peek(self) -> Token:
        return self.tokens[self.current]

    def empty(self) -> bool:
        return self.peek().token_type == TokenType.EOF

    def check(self, token_type: TokenType) -> bool:
        return not self.empty() and self.peek().token_type == token_type

    def advance(self):
        if not self.empty():
            self.current += 1
        return self.previous()

    def match(self, *token_types: TokenType) -> bool:
        for token_type in token_types:
            if self.check(token_type):
                self.advance()
                return True
        return False

    def synchronize(self):
        self.advance()
        while not self.empty():
            if self.previous().token_type == TokenType.SEMICOLON:
                return
            match self.peek().token_type:
                case (
                    TokenType.CLASS
                    | TokenType.FUN
                    | TokenType.VAR
                    | TokenType.FOR
                    | TokenType.IF
                    | TokenType.WHILE
                    | TokenType.PRINT
                    | TokenType.RETURN
                ):
                    return
            self.advance()
