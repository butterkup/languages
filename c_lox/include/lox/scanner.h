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

bool is_at_end() { return *scanner.current == '\0'; }
bool isalphanum(char c) { return c == '_' || isalnum(c); }

int lexlen();

Token _make_token_impl(TokenType type, const char *start, int length) {
  Token token;
  token.type = type;
  token.start = start;
  token.length = length;
  token.line = scanner.line;
  return token;
}

Token make_token(TokenType type) {
  return _make_token_impl(type, scanner.start, lexlen());
}

Token error_token(const char *message) {
  return _make_token_impl(TOKEN_ERROR, message, (int)strlen(message));
}

char peek() { return *scanner.current; }
char advance() { return *scanner.current++; }
char peek_next() { return *(scanner.current + 1); }

bool match(char expected) {
  if (peek() != expected)
    return false;
  advance(); return true;
}

Token string() {
  while (peek() != '"')
    if (advance() == '\n') scanner.line++;
    else if (is_at_end())
      return error_token("Unterminated string.");
  advance(); // Consume the closing quote
  return make_token(TOKEN_STRING);
}

void skip_whitespace() {
  for (;;)
    switch (peek()) {
    default: return;
    case '\n': scanner.line++;
    case ' ':
    case '\t': advance();
    }
}

Token number() {
  while (isdigit(peek())) advance();
  if (match('.') && isdigit(peek_next()))
    while (isdigit(peek())) advance();
  return make_token(TOKEN_NUMBER);
}

TokenType check_keyword(int start, int length, const char *rest, TokenType type) {
  if (lexlen() == start + length &&
    !memcmp(scanner.start + start, rest, length))
    return type;
  return TOKEN_IDENTIFIER;
}

int lexlen() { return (int)(scanner.current - scanner.start); }

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
  while (isalphanum(peek())) advance();
  return make_token(identifier_type());
}

Token scan();

bool _consume_multiline_comment() {
  for (;;)
    if (is_at_end()) return false;
    else switch (advance()) {
    case '\n': scanner.line++;
    case '*': if (match('/')) return true;
    }
}

void _consume_oneline_comment() {
  while (peek() != '\n' && !is_at_end()) advance();
}

bool match_comment() {
  switch (peek()) {
  case '/':
  case '*': return true;
  default: return false;
  }
}

Token consume_tk_comment() {
  switch (advance()) {
  case '/': _consume_oneline_comment(); break;
  case '*':
    if (!_consume_multiline_comment())
      return error_token("Unterminated multiline comment.");
    else break;
  default: return error_token("Expected a comment then a token.");
  }
  return scan();
}

Token scan() {
  skip_whitespace();
  scanner.start = scanner.current;
  if (is_at_end()) return make_token(TOKEN_EOF);
  char c = advance();
  if (isdigit(c)) return number();
  if (isalphanum(c)) return identifier();
  switch (c) {
  case '"': return string();
  case '.': return make_token(TOKEN_DOT);
  case '+': return make_token(TOKEN_PLUS);
  case '*': return make_token(TOKEN_STAR);
  case ',': return make_token(TOKEN_COMMA);
  case '-': return make_token(TOKEN_MINUS);
  case ';': return make_token(TOKEN_SEMICOLON);
  case '(': return make_token(TOKEN_LEFT_PAREN);
  case '{': return make_token(TOKEN_LEFT_BRACE);
  case ')': return make_token(TOKEN_RIGHT_PAREN);
  case '}': return make_token(TOKEN_RIGHT_BRACE);
  case '!': return make_token(match('=') ? TOKEN_BANG_EQUAL : TOKEN_BANG);
  case '<': return make_token(match('=') ? TOKEN_LESS_EQUAL : TOKEN_LESS);
  case '=': return make_token(match('=') ? TOKEN_EQUAL_EQUAL : TOKEN_EQUAL);
  case '>': return make_token(match('=') ? TOKEN_GREATER_EQUAL : TOKEN_GREATER);
  case '/': return match_comment() ? consume_tk_comment() : make_token(TOKEN_SLASH);
  default:  return error_token("Unexpected character.");
  }
}

#define CSTKTP(type) case TOKEN##type: return #type + 1

const char *strtokentype(TokenType type) {
  switch (type) {
    CSTKTP(_LEFT_PAREN);
    CSTKTP(_RIGHT_PAREN);
    CSTKTP(_LEFT_BRACE);
    CSTKTP(_RIGHT_BRACE);
    CSTKTP(_COMMA);
    CSTKTP(_DOT);
    CSTKTP(_MINUS);
    CSTKTP(_PLUS);
    CSTKTP(_SEMICOLON);
    CSTKTP(_SLASH);
    CSTKTP(_STAR);
    CSTKTP(_BANG);
    CSTKTP(_BANG_EQUAL);
    CSTKTP(_EQUAL);
    CSTKTP(_EQUAL_EQUAL);
    CSTKTP(_GREATER);
    CSTKTP(_GREATER_EQUAL);
    CSTKTP(_LESS);
    CSTKTP(_LESS_EQUAL);
    CSTKTP(_IDENTIFIER);
    CSTKTP(_STRING);
    CSTKTP(_NUMBER);
    CSTKTP(_AND);
    CSTKTP(_CLASS);
    CSTKTP(_ELSE);
    CSTKTP(_FALSE);
    CSTKTP(_FOR);
    CSTKTP(_FUN);
    CSTKTP(_IF);
    CSTKTP(_NIL);
    CSTKTP(_OR);
    CSTKTP(_PRINT);
    CSTKTP(_RETURN);
    CSTKTP(_SUPER);
    CSTKTP(_THIS);
    CSTKTP(_TRUE);
    CSTKTP(_VAR);
    CSTKTP(_WHILE);
    CSTKTP(_ERROR);
    CSTKTP(_EOF);
  }
  return "<UNKNOWN>";
}

#undef CSTKTP

void token_print(Token *token) {
  printf("Token(%s, '%.*s', %d)\n",
    strtokentype(token->type),
    token->length,
    token->start,
    token->line
  );
}

CLOX_END_DECLS

#endif //_CLOX_SCANNER_H
