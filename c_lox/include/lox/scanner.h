#ifndef _CLOX_SCANNER_H
#define _CLOX_SCANNER_H

#include "common.h"

CLOX_BEG_DECLS

typedef enum {
  // Single character tokens
  TOKEN_LEFT_PAREN, TOKEN_RIGHT_PAREN,
  TOKEN_LEFT_BRACE, TOKEN_RIGHT_BRACE,
  TOKEN_DOT, TOKEN_COMMA, TOKEN_MINUS, TOKEN_PLUS,
  TOKEN_SEMICOLON, TOKEN_SLASH, TOKEN_STAR,

  // One or Two character tokens
  TOKEN_BANG, TOKEN_BANG_EQUAL,
  TOKEN_EQUAL, TOKEN_EQUAL_EQUAL,
  TOKEN_GREATER, TOKEN_GREATER_EQUAL,
  TOKEN_LESS, TOKEN_LESS_EQUAL,

  // Literals
  TOKEN_NUMBER, TOKEN_STRING, TOKEN_IDENTIFIER,

  // Keywords
  TOKEN_AND, TOKEN_OR, TOKEN_CLASS, TOKEN_ELSE, TOKEN_FALSE,
  TOKEN_TRUE, TOKEN_FOR, TOKEN_FUN, TOKEN_IF, TOKEN_NIL,
  TOKEN_PRINT, TOKEN_RETURN, TOKEN_SUPER, TOKEN_THIS, TOKEN_VAR,
  TOKEN_WHILE,

  // Special
  TOKEN_ERROR, TOKEN_EOF
} TokenType;

typedef struct {
  const char *start;
  TokenType type;
  int length;
  int line;
} Token;

typedef struct {
  const char *start;
  const char *current;
  int line;
} Scanner;

Scanner scanner;

void scanner_init(const char *source) {
  scanner.line = 1;
  scanner.start = source;
  scanner.current = source;
}

bool is_at_end() {
  return *scanner.current == '\0';
}

int lexlen();

Token make_token(TokenType type) {
  Token token;
  token.type = type;
  token.start = scanner.start;
  token.length = lexlen();
  token.line = scanner.line;
  return token;
}

Token error_token(const char *message) {
  Token token;
  token.type = TOKEN_ERROR;
  token.start = message;
  token.length = (int)strlen(message);
  token.line = scanner.line;
  return token;
}

char peek() {
  return *scanner.current;
}

char peek_next() {
  return *(scanner.current + !is_at_end());
}

char advance() {
  return *scanner.current++;
}

bool match(char expected) {
  if (is_at_end() || peek() != expected)
    return false;
  advance();
  return true;
}

Token string() {
  while (peek() != '"' && !is_at_end())
    if (advance() == '\n')
      scanner.line++;
  if (is_at_end())
    return error_token("Unterminated string.");
  advance(); // Consume the closing quote
  return make_token(TOKEN_STRING);
}

void skip_whitespace() {
  for (;;)
    switch (peek()) {
    case '\n':
      scanner.line++;
    case ' ':
    case '\t':
      advance();
      break;
    default:
      return;
    }
}

Token number() {
  while (isdigit(peek())) advance();
  if (peek() == '.' && isdigit(peek_next())) {
    advance();
    while (isdigit(peek())) advance();
  }
  return make_token(TOKEN_NUMBER);
}

TokenType check_keyword(int start, int length, const char *rest, TokenType type) {
  if (scanner.current - scanner.start == start + length
    && memcmp(scanner.start + start, rest, length))
    return type;
  return TOKEN_IDENTIFIER;
}

int lexlen() {
  return (int)(scanner.current - scanner.start);
}

TokenType identifier_type() {
  switch (*scanner.start) {
  case 'a': return check_keyword(1, 2, "nd", TOKEN_AND);
  case 'c': return check_keyword(1, 4, "lass", TOKEN_CLASS);
  case 'e': return check_keyword(1, 3, "lse", TOKEN_ELSE);
  case 'f':
    if (lexlen() > 1)
      switch (*(scanner.start + 1)) {
      case 'a': return check_keyword(2, 3, "lse", TOKEN_FALSE);
      case 'o': return check_keyword(2, 1, "r", TOKEN_FOR);
      case 'u': return check_keyword(2, 1, "n", TOKEN_FUN);
      }
    break;
  case 'i': return check_keyword(1, 1, "f", TOKEN_IF);
  case 'n': return check_keyword(1, 2, "il", TOKEN_NIL);
  case 'o': return check_keyword(1, 1, "r", TOKEN_OR);
  case 'p': return check_keyword(1, 4, "rint", TOKEN_PRINT);
  case 'r': return check_keyword(1, 5, "eturn", TOKEN_RETURN);
  case 's': return check_keyword(1, 4, "uper", TOKEN_SUPER);
  case 't':
    if (lexlen() > 1)
      switch (*(scanner.start + 1)) {
      case 'h': return check_keyword(2, 2, "is", TOKEN_THIS);
      case 'r': return check_keyword(2, 2, "ue", TOKEN_TRUE);
      }
    break;
  case 'v': return check_keyword(1, 2, "ar", TOKEN_VAR);
  case 'w': return check_keyword(1, 4, "hile", TOKEN_WHILE);
  }
  return TOKEN_IDENTIFIER;
}

Token identifier() {
  while (isalnum(peek())) advance();
  return make_token(identifier_type());
}

Token scan() {
  skip_whitespace();
  scanner.start = scanner.current;
  if (is_at_end())
    return make_token(TOKEN_EOF);
  char c = advance();
  if (isdigit(c)) return number();
  if (isalpha(c)) return identifier();
  switch (c) {
  case '(': return make_token(TOKEN_RIGHT_PAREN);
  case ')': return make_token(TOKEN_LEFT_PAREN);
  case '{': return make_token(TOKEN_RIGHT_BRACE);
  case '}': return make_token(TOKEN_LEFT_BRACE);
  case ';': return make_token(TOKEN_SEMICOLON);
  case ',': return make_token(TOKEN_COMMA);
  case '.': return make_token(TOKEN_DOT);
  case '-': return make_token(TOKEN_MINUS);
  case '+': return make_token(TOKEN_PLUS);
  case '*': return make_token(TOKEN_STAR);
  case '/':
    if (peek_next() == '/')
      while (peek() != '\n' && !is_at_end())
        advance();
    else
      return make_token(TOKEN_SLASH);
    return scan();
  case '!': return make_token(match('=') ? TOKEN_BANG_EQUAL : TOKEN_BANG);
  case '>': return make_token(match('=') ? TOKEN_GREATER_EQUAL : TOKEN_GREATER);
  case '<': return make_token(match('=') ? TOKEN_LESS_EQUAL : TOKEN_LESS);
  case '=': return make_token(match('=') ? TOKEN_EQUAL_EQUAL : TOKEN_EQUAL);
  case '"': return string();
  }
  return error_token("Unexpected character.");
}

#define CSTKTP(type) case TOKEN_##type: return #type

const char *tokentype_print(TokenType type) {
  switch (type) {
    CSTKTP(LEFT_PAREN);
    CSTKTP(RIGHT_PAREN);
    CSTKTP(LEFT_BRACE);
    CSTKTP(RIGHT_BRACE);
    CSTKTP(DOT);
    CSTKTP(COMMA);
    CSTKTP(MINUS);
    CSTKTP(PLUS);
    CSTKTP(SEMICOLON);
    CSTKTP(SLASH);
    CSTKTP(STAR);
    CSTKTP(BANG);
    CSTKTP(BANG_EQUAL);
    CSTKTP(EQUAL);
    CSTKTP(EQUAL_EQUAL);
    CSTKTP(GREATER);
    CSTKTP(GREATER_EQUAL);
    CSTKTP(LESS);
    CSTKTP(LESS_EQUAL);
    CSTKTP(NUMBER);
    CSTKTP(STRING);
    CSTKTP(IDENTIFIER);
    CSTKTP(AND);
    CSTKTP(OR);
    CSTKTP(CLASS);
    CSTKTP(ELSE);
    CSTKTP(FALSE);
    CSTKTP(TRUE);
    CSTKTP(FOR);
    CSTKTP(FUN);
    CSTKTP(IF);
    CSTKTP(NIL);
    CSTKTP(PRINT);
    CSTKTP(RETURN);
    CSTKTP(SUPER);
    CSTKTP(THIS);
    CSTKTP(VAR);
    CSTKTP(WHILE);
    CSTKTP(ERROR);
  case TOKEN_EOF: return "EOF";
  }
  return "<UNKNOWN>";
}

#undef CSTKTP

void token_print(Token *token) {
  printf("Token(%s, '%.*s', %d)\n",
    tokentype_print(token->type),
    token->length,
    token->start,
    token->line
  );
}

CLOX_END_DECLS

#endif //_CLOX_SCANNER_H
