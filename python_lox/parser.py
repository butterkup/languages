from .expr_ast import (
    Binary,
    Unary,
    Literal,
    Grouping,
    Ternary,
    Variable,
    Assign,
    Logical,
    Call,
)
from .category import BINARY_OPERATORS, COMPARISON_OPERATORS, EQUALITY_OPERATORS
from .stmt_ast import Expression, Print, Var, Block, If, While, Function, Return
from .exc import ParserError, MissingExpr
from .token import Token, TokenType
from .reporter import Reporter


class Parser:
    def __init__(self, tokens: list[Token], reporter: Reporter) -> None:
        self.reporter = reporter
        self.tokens = tokens
        self.current = 0

    def parse(self):
        statements = []
        try:
            while not self.empty():
                decl = self.declaration()
                statements.append(decl)
        except ParserError:
            ...
        return statements

    def declaration(self):
        try:
            if self.match(TokenType.VAR):
                return self.var_stmt()
            if self.match(TokenType.FUN):
                return self.fun_stmt()
            return self.statement()
        except ParserError:
            self.synchronize()
            if self.empty():
                raise
        return self.declaration()

    def statement(self):
        if self.match(TokenType.PRINT):
            return self.print_stmt()
        if self.match(TokenType.LEFT_BRACE):
            return self.block_stmt()
        if self.match(TokenType.IF):
            return self.if_stmt()
        if self.match(TokenType.WHILE):
            return self.while_stmt()
        if self.match(TokenType.FOR):
            return self.for_stmt()
        if self.match(TokenType.RETURN):
            return self.return_stmt()
        return self.expr_stmt()

    def return_stmt(self):
        keyword = self.previous()
        value = Literal(None) if self.check(TokenType.SEMICOLON) else self.expression()
        self.consume(
            TokenType.SEMICOLON, "Expected ';' at the end of the return statement."
        )
        return Return(keyword, value)

    def fun_stmt(self):
        if not self.check(TokenType.IDENTIFIER):
            raise ParserError(self.peek(), "Expected an identifier after fun keyword.")
        fname = self.advance()
        self.consume(TokenType.LEFT_PAREN, "Expected '(' after function identifier.")
        parameters = []
        while self.match(TokenType.IDENTIFIER):
            parameters.append(self.previous())
            self.match(TokenType.COMMA)
        self.consume(TokenType.RIGHT_PAREN, "Expected ')' after function parameters.")
        self.consume(
            TokenType.LEFT_BRACE, "Expected '{' after function parameters."
        )
        return Function(fname, parameters, self.block_stmt())

    def block_stmt(self):
        statements = []
        while not self.check(TokenType.RIGHT_BRACE) and not self.empty():
            statements.append(self.declaration())
        self.consume(TokenType.RIGHT_BRACE, "Expected '}' to close opened block.")
        return Block(statements)

    def print_stmt(self):
        expr = self.expression()
        self.consume(TokenType.SEMICOLON, "Expected ';' after print statement.")
        return Print(expr)

    def if_stmt(self):
        self.consume(TokenType.LEFT_PAREN, "Expected '(' after the if keyword.")
        condition = self.expression()
        self.consume(TokenType.RIGHT_PAREN, "Expected ')' after if expression.")
        then_ = self.statement()
        else_ = self.statement() if self.match(TokenType.ELSE) else None
        return If(condition, then_, else_)

    def expr_stmt(self):
        expr = (
            Literal(None)
            if self.peek().token_type == TokenType.SEMICOLON
            else self.expression()
        )
        self.consume(TokenType.SEMICOLON, "Expected ';' after expression statement.")
        return Expression(expr)  # type: ignore

    def var_stmt(self):
        name = self.consume(TokenType.IDENTIFIER, "Expected a variable name.")
        init = self.expression() if self.match(TokenType.EQUAL) else Literal(None)
        self.consume(TokenType.SEMICOLON, "Expected ';' after var statement.")
        return Var(name, init)

    def while_stmt(self):
        self.consume(TokenType.LEFT_PAREN, "Expected '(' after the while statement.")
        condition = self.expression()
        self.consume(TokenType.RIGHT_PAREN, "Expected ')' after the while condition.")
        body = self.statement()
        return While(condition, body)

    def for_stmt(self):
        self.consume(TokenType.LEFT_PAREN, "Expected '(' after for keyword.")
        hasinit = not self.check(TokenType.SEMICOLON)
        init = self.var_stmt() if self.match(TokenType.VAR) else self.expr_stmt()
        cond = (
            Literal(True)
            if self.match(TokenType.SEMICOLON)
            else self.expr_stmt().expression
        )
        incr = None if self.check(TokenType.RIGHT_PAREN) else self.expression()
        self.consume(TokenType.RIGHT_PAREN, "Expected ')' after for condition.")
        body = self.statement()
        if incr is not None:
            incr = Expression(incr)
            if isinstance(body, Block):
                body.statements.append(incr)
            else:
                body = Block([body, incr])
        body = While(cond, body)
        if hasinit:
            body = Block([init, body])
        return body

    def expression(self):
        if self.peek().token_type in BINARY_OPERATORS:
            raise self.error(
                self.peek(),
                f"Left operand for binary operation ({self.peek().lexeme}) missing.",
            )
        return self.assignment()

    def assignment(self):
        expr = self.logic_or()
        if self.match(TokenType.EQUAL):
            equals = self.previous()
            if isinstance(expr, Variable):
                value = self.assignment()
                return Assign(expr.value, value)
            self.error(equals, "Expected an assignable target.")
        return expr

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

    def logic_or(self):
        expr = self.logic_and()
        while self.match(TokenType.OR):
            expr = Logical(expr, self.previous(), self.logic_or())
        return expr

    def logic_and(self):
        expr = self.ternary()
        while self.match(TokenType.AND):
            expr = Logical(expr, self.previous(), self.logic_and())
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
        return self.call()

    def call(self):
        callee = self.primary()
        if self.match(TokenType.LEFT_PAREN):
            open_paren = self.previous()
            arguments = []
            if not self.check(TokenType.RIGHT_PAREN):
                arguments.append(self.expression())
                while self.match(TokenType.COMMA):
                    arguments.append(self.expression())
            self.consume(TokenType.RIGHT_PAREN, "Expected ')' after arguments.")
            return Call(callee, open_paren, arguments, self.previous())
        return callee

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
        if self.match(TokenType.IDENTIFIER):
            return Variable(self.previous())
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
