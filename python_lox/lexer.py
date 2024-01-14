import typing as ty
from .token import Token, TokenType
from .category import keywords

if ty.TYPE_CHECKING:
    from .reporter import Reporter
else:
    Reporter = None


class Lexer:
    def __init__(self, source: str, repoter: Reporter) -> None:
        self._scanned: bool = False
        self.tokens: list[Token] = []
        self.stop = len(source)
        self.repoter = repoter
        self._src = source
        self._current = 0
        self._start = 0
        self._line = 1

    @property
    def src(self) -> str:
        return self._src
    
    @property
    def scanned(self) -> bool:
        return self._scanned

    def empty(self) -> bool:
        return self._current >= self.stop

    def scan_tokens(self) -> list[Token]:
        if self._scanned:
            return self.tokens
        while not self.empty():
            self._start = self._current
            self._scantoken()
        self.tokens.append(Token(TokenType.EOF, "", None, self._line))
        self._scanned = True
        return self.tokens

    def _scantoken(self):
        char = self.advance()
        match char:
            case '(': self.add_token(TokenType.LEFT_PAREN)
            case ')': self.add_token(TokenType.RIGHT_PAREN)
            case '{': self.add_token(TokenType.LEFT_BRACE)
            case '}': self.add_token(TokenType.RIGHT_BRACE)
            case ',': self.add_token(TokenType.COMMA)
            case '?': self.add_token(TokenType.QMARK)
            case ':': self.add_token(TokenType.COLON)
            case '.': self.add_token(TokenType.DOT)
            case '-': self.add_token(TokenType.MINUS)
            case '+': self.add_token(TokenType.PLUS)
            case ';': self.add_token(TokenType.SEMICOLON)
            case '*': self.add_token(TokenType.POW if self.match('*') else TokenType.STAR)
            case '!': self.add_token(TokenType.BANG_EQUAL if self.match('=') else TokenType.BANG)
            case '<': self.add_token(TokenType.LESS_THAN if self.match('=') else TokenType.LESS)
            case '>': self.add_token(TokenType.GREATER_THAN if self.match('=') else TokenType.GREATER)
            case '=': self.add_token(TokenType.EQUAL_EQUAL if self.match('=') else TokenType.EQUAL)
            case '"': self.string()
            case '/':
                if self.match('/'):
                    while self.peek() != '\n':
                        if self.empty():
                            break
                        self.advance()
                elif self.match('*'):
                    start = self._line
                    while True:
                        if self.empty():
                            self.repoter.error(start, "Multiline comment was never closed.")
                            break
                        if self.peek() == '\n':
                            self._line += 1
                        if self.peek() == '*' and self.peek_next() == '/':
                            self.advance()
                            self.advance()
                            break
                        self.advance()
                else:
                    self.add_token(TokenType.SLASH)
            case '\r' | '\t' | ' ' | '\v' | '\f': pass
            case '\n': self._line += 1
            case c:
                if self.isdigit(c):
                    self.number()
                elif self.isalpha(c):
                    self.identifier()
                else:
                    self.repoter.error(self._line, f"Unexpected character {c!r}")

    def advance(self):
        if not self.empty(): self._current += 1
        return self.src[self._current - 1]

    def add_token(self, token_type: TokenType, literal: object = None):
        text = self.src[self._start : self._current]
        token = Token(token_type, text, literal, self._line)
        self.tokens.append(token)

    def match(self, expected: str):
        if self.empty() or self.src[self._current] != expected:
            return False
        self._current += 1
        return True

    def peek(self) -> str | None:
        return None if self.empty() else self.src[self._current]

    def string(self):
        while self.peek() != '"' and not self.empty():
            if self.peek() == "\n":
                self._line += 1
            self.advance()
        if self.empty():
            self.repoter.error(self._line, "Unterminated string.")
        else:
            self.advance()
        string = self.src[self._start + 1 : self._current - 1]
        self.add_token(TokenType.STRING, string)

    def isdigit(self, digit: str):
        return ord("0") <= ord(digit) <= ord("9")

    def number(self):
        while self.isdigit(self.peek() or "\0"):
            self.advance()
        if self.peek() == "." and self.isdigit(self.peek_next() or "\0"):
            self.advance()
            while self.isdigit(self.peek() or "\0"):
                self.advance()
        if self.isalpha(self.peek() or '\0'):
            self.repoter.error(self._line, "Invalid Identifier or characters in number literal.")
        number = self.src[self._start : self._current]
        number = float(number) if "." in number else int(number)
        self.add_token(TokenType.NUMBER, number)

    def peek_next(self):
        return None if self._current + 1 >= self.stop else self.src[self._current + 1]

    def identifier(self):
        while self.isalphanumeric(self.peek() or "\0"):
            self.advance()
        identifier = self.src[self._start : self._current]
        token_type = keywords.get(identifier, TokenType.IDENTIFIER)
        self.add_token(token_type)

    def isalpha(self, char):
        return (
            ord("a") <= ord(char) <= ord("z")
            or ord("A") <= ord(char) <= ord("Z")
            or char == "_"
        )

    def isalphanumeric(self, char):
        return self.isdigit(char) or self.isalpha(char)
