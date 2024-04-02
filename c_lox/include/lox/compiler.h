#ifndef _CLOX_COMPILER_H
#define _CLOX_COMPILER_H

#include "common.h"
#include "scanner.h"
#include "chunk.h"
#include "object.h"

CLOX_BEG_DECLS

typedef enum {
  PREC_NONE,
  PREC_ASSIGNMENT, // =
  PREC_OR,         // or
  PREC_AND,        // and
  PREC_EQUALITY,   // == !=
  PREC_COMPARISON, // > < >= <=
  PREC_TERM,       // + -
  PREC_FACTOR,     // * /
  PREC_UNARY,      // - !
  PREC_CALL,       // . ()
  PREC_PRIMARY
} Precedence;

typedef void (*ParseFn)(bool);

typedef struct {
  ParseFn prefix;
  ParseFn infix;
  Precedence precedence;
} ParserRule;

typedef struct {
  Token current;
  Token previous;
  bool had_error; // Signifies a compilation error
  bool panic_mode; // Helps in resynchronizing after an error
} Parser;

typedef struct {
  Token name;
  int depth;
  bool is_captured;
} Local;

typedef enum {
  TYPE_FUNCTION,
  TYPE_SCRIPT
} FunctionType;

typedef struct {
  uint8_t index;
  bool is_local;
} Upvalue;

typedef struct Compiler Compiler;

struct Compiler {
  Compiler* enclosing;
  ObjectFunction* function;
  FunctionType type;
  Local locals[UINT8_COUNT];
  int local_count;
  int scope_depth;
  Upvalue upvalues[UINT8_COUNT];
};

Parser parser;
Compiler* current = NULL;

void comp_init(Compiler* comp, FunctionType type) {
  comp->function = new_function();
  comp->local_count = 0;
  comp->scope_depth = 0;
  comp->type = type;
  comp->enclosing = current;
  current = comp;
  if ( type != TYPE_SCRIPT )
    current->function->name = copy_string(
      parser.previous.start, parser.previous.length
    );
  Local* local = &current->locals[current->local_count++];
  local->depth = 0;
  local->name.start = "";
  local->name.length = 0;
  local->is_captured = false;
}

void stmt_var();
void expr_or(bool);
void literal(bool);
void expression();
void expr_dot(bool);
void expr_and(bool);
void patch_jump(int);
void expr_call(bool);
void compiler_sync();
void expr_unary(bool);
void stmt_statement();
int emit_jump(OpCode);
void expr_binary(bool);
void expr_number(bool);
void expr_string(bool);
void emit_byte(uint8_t);
void stmt_declaration();
void compiler_advance();
void emit_byte(uint8_t);
void expr_grouping(bool);
void expr_variable(bool);
void gc_mark_compiler_roots();
bool compiler_match(TokenType);
void emit_bytes(uint8_t, uint8_t);
ObjectFunction* compiler_delete();
bool compiler_check(TokenType);
uint8_t identifier_constant(Token*);
bool identifier_equal(Token*, Token*);
void compiler_consume(TokenType, const char*);

#define TKPREC_RULE(tktp, prefix, infix, precedence) [TOKEN##tktp] = { prefix, infix, PREC##precedence }

ParserRule tkprec_rules[] = {
  TKPREC_RULE(_LEFT_PAREN,       expr_grouping,      expr_call,      _CALL),
  TKPREC_RULE(_RIGHT_PAREN,      NULL,               NULL,           _NONE),
  TKPREC_RULE(_LEFT_BRACE,       NULL,               NULL,           _NONE),
  TKPREC_RULE(_RIGHT_BRACE,      NULL,               NULL,           _NONE),
  TKPREC_RULE(_COMMA,            NULL,               NULL,           _NONE),
  TKPREC_RULE(_DOT,              NULL,               expr_dot,       _CALL),
  TKPREC_RULE(_MINUS,            expr_unary,         expr_binary,    _TERM),
  TKPREC_RULE(_PLUS,             NULL,               expr_binary,    _TERM),
  TKPREC_RULE(_SEMICOLON,        NULL,               NULL,           _NONE),
  TKPREC_RULE(_SLASH,            NULL,               expr_binary,    _FACTOR),
  TKPREC_RULE(_STAR,             NULL,               expr_binary,    _FACTOR),
  TKPREC_RULE(_BANG,             expr_unary,         NULL,           _NONE),
  TKPREC_RULE(_BANG_EQUAL,       NULL,               expr_binary,    _EQUALITY),
  TKPREC_RULE(_EQUAL,            NULL,               NULL,           _NONE),
  TKPREC_RULE(_EQUAL_EQUAL,      NULL,               expr_binary,    _EQUALITY),
  TKPREC_RULE(_GREATER,          NULL,               expr_binary,    _COMPARISON),
  TKPREC_RULE(_GREATER_EQUAL,    NULL,               expr_binary,    _COMPARISON),
  TKPREC_RULE(_LESS,             NULL,               expr_binary,    _COMPARISON),
  TKPREC_RULE(_LESS_EQUAL,       NULL,               expr_binary,    _COMPARISON),
  TKPREC_RULE(_IDENTIFIER,       expr_variable,      NULL,           _NONE),
  TKPREC_RULE(_STRING,           expr_string,        NULL,           _NONE),
  TKPREC_RULE(_NUMBER,           expr_number,        NULL,           _NONE),
  TKPREC_RULE(_AND,              NULL,               expr_and,       _AND),
  TKPREC_RULE(_CLASS,            NULL,               NULL,           _NONE),
  TKPREC_RULE(_ELSE,             NULL,               NULL,           _NONE),
  TKPREC_RULE(_FALSE,            literal,            NULL,           _NONE),
  TKPREC_RULE(_FOR,              NULL,               NULL,           _NONE),
  TKPREC_RULE(_FUN,              NULL,               NULL,           _NONE),
  TKPREC_RULE(_IF,               NULL,               NULL,           _NONE),
  TKPREC_RULE(_NIL,              literal,            NULL,           _NONE),
  TKPREC_RULE(_OR,               NULL,               expr_or,        _OR),
  TKPREC_RULE(_PRINT,            NULL,               NULL,           _NONE),
  TKPREC_RULE(_RETURN,           NULL,               NULL,           _NONE),
  TKPREC_RULE(_SUPER,            NULL,               NULL,           _NONE),
  TKPREC_RULE(_THIS,             NULL,               NULL,           _NONE),
  TKPREC_RULE(_TRUE,             literal,            NULL,           _NONE),
  TKPREC_RULE(_VAR,              NULL,               NULL,           _NONE),
  TKPREC_RULE(_WHILE,            NULL,               NULL,           _NONE),
  TKPREC_RULE(_ERROR,            NULL,               NULL,           _NONE),
  TKPREC_RULE(_EOF,              NULL,               NULL,           _NONE)
};

#undef TKPREC_RULE

ParserRule* get_rule(TokenType tktype) {
  return &tkprec_rules[tktype];
}

void parse_precedence(Precedence);

void _error_at(Token* token, const char* message) {
  if ( parser.panic_mode ) return;
  parser.panic_mode = true;
  parser.had_error = true;
  fprintf(stderr, "[line %d] Error", token->line);
  switch ( token->type ) {
  case TOKEN_EOF: fprintf(stderr, " at end"); break;
  case TOKEN_ERROR: break;
  default: fprintf(stderr, " at '%.*s'", token->length, token->start);
  }
  fprintf(stderr, ": %s\n", message);
}

void error(const char* message) {
  _error_at(&parser.previous, message);
}

void error_at(const char* message) {
  _error_at(&parser.current, message);
}

void expr_and(bool) {
  int jump_end = emit_jump(OP_JUMP_IF_FALSE);
  emit_byte(OP_POP);
  parse_precedence(PREC_AND);
  patch_jump(jump_end);
}

void expr_or(bool) {
  int jump_else = emit_jump(OP_JUMP_IF_FALSE),
    jump_end = emit_jump(OP_JUMP);
  patch_jump(jump_else);
  emit_byte(OP_POP);
  parse_precedence(PREC_OR);
  patch_jump(jump_end);
}

void expr_dot(bool can_assign) {
  compiler_consume(TOKEN_IDENTIFIER, "Expect property name after '.'.");
  uint8_t property = identifier_constant(&parser.previous);
  if (can_assign && compiler_match(TOKEN_EQUAL)) {
  expression();
    emit_bytes(OP_SET_PROPERTY, property);
  } else emit_bytes(OP_GET_PROPERTY, property);
}

void compiler_sync() {
  parser.panic_mode = false;
  while ( !compiler_check(TOKEN_EOF) ) {
    if ( compiler_check(TOKEN_SEMICOLON) ) return;
    switch ( parser.current.type ) {
    case TOKEN_RETURN:
    case TOKEN_CLASS:
    case TOKEN_WHILE:
    case TOKEN_PRINT:
    case TOKEN_VAR:
    case TOKEN_FOR:
    case TOKEN_FUN:
    case TOKEN_IF: return;
    }
    compiler_advance();
  }
}

void compiler_advance() {
  parser.previous = parser.current;
  for ( ;;) {
    parser.current = scan();
#ifdef CLOX_SCAN_TRACE
    if ( !is_at_end() || parser.previous.type != TOKEN_EOF )
      token_print(&parser.current);
#endif
    if ( parser.current.type != TOKEN_ERROR ) break;
    error_at(parser.current.start);
  }
}

bool compiler_check(TokenType type) {
  return parser.current.type == type;
}

void compiler_consume(TokenType type, const char* message) {
  if ( compiler_check(type) ) compiler_advance();
  else error_at(message);
}

Chunk* current_chunk() { return &current->function->chunk; }

void emit_byte(uint8_t byte) {
  chunk_append(current_chunk(), byte, parser.previous.line);
}

void emit_bytes(uint8_t b1, uint8_t b2) {
  emit_byte(b1); emit_byte(b2);
}

void emit_return() {
  emit_bytes(OP_NIL, OP_RETURN);
}

uint8_t make_constant(Value constant) {
  int location = chunk_cappend(current_chunk(), constant);
  if ( location <= UINT8_MAX ) return (uint8_t)location;
  error("Too many constants in one chunk.");
  return 0;
}

void emit_constant(Value constant) {
  emit_bytes(OP_CONSTANT, make_constant(constant));
}

void literal(bool) {
  switch ( parser.previous.type ) {
  case TOKEN_NIL: emit_byte(OP_NIL);      break;
  case TOKEN_TRUE: emit_byte(OP_TRUE);    break;
  case TOKEN_FALSE: emit_byte(OP_FALSE);  break;
  default: printf("Unknown literal type[%d]: %s -> '%.*s'\n",
    parser.previous.type,
    strtokentype(parser.previous.type),
    parser.previous.length,
    parser.previous.start
  ); exit(1);
  }
}

void expr_number(bool) {
  double constant = strtod(parser.previous.start, NULL);
  emit_constant(NUMBER_VAL(constant));
}

void expr_string(bool) {
  emit_constant(OBJECT_VAL(copy_string(parser.previous.start + 1, parser.previous.length - 2)));
}

int resolve_local(Compiler* compiler, Token* name) {
  bool unintialized = false;
  for ( int i = compiler->local_count - 1; i >= 0; --i )
    if ( identifier_equal(name, &(compiler->locals + i)->name) )
      if ( (compiler->locals + i)->depth == -1 ) unintialized = true;
      else return i;
#ifndef CLOX_VAR_NO_SELF_INIT
  if ( unintialized ) error("Variable used in its own initializer.");
#endif
  return -1;
}

int add_upvalue(Compiler* compiler, uint8_t index, bool is_local) {
  int upvalue_count = compiler->function->upvalue_count;
  Upvalue* upvalue;
  for ( int i = 0; i < upvalue_count; ++i ) {
    upvalue = &compiler->upvalues[i];
    if ( upvalue->index == index && upvalue->is_local == is_local )
      return i;
  }
  if ( upvalue_count == UINT8_COUNT ) {
    error("Too many closure variables in function.");
    return 0;
  }
  compiler->upvalues[upvalue_count].is_local = is_local;
  compiler->upvalues[upvalue_count].index = index;
  return compiler->function->upvalue_count++;
}

int resolve_upvalue(Compiler* compiler, Token* name) {
  if ( compiler->enclosing == NULL ) return -1;
  int local = resolve_local(compiler->enclosing, name);
  if ( local != -1 ) return add_upvalue(compiler, (uint8_t)local,
    (compiler->enclosing->locals[local].is_captured = true));
  local = resolve_upvalue(compiler->enclosing, name);
  if ( local != -1 ) return add_upvalue(compiler, (uint8_t)local, false);
  return -1;
}

void named_variable(Token name, bool can_assign) {
  int arg;
  uint8_t set_op, get_op;
  if ( (arg = resolve_local(current, &name)) != -1 ) {
    set_op = OP_SET_LOCAL;
    get_op = OP_GET_LOCAL;
  } else if ( (arg = resolve_upvalue(current, &name)) != -1 ) {
    set_op = OP_SET_UPVALUE;
    get_op = OP_GET_UPVALUE;
  } else {
    arg = identifier_constant(&name);
    set_op = OP_SET_GLOBAL;
    get_op = OP_GET_GLOBAL;
  }

  if ( can_assign && compiler_match(TOKEN_EQUAL) ) {
    expression(); emit_bytes(set_op, (uint8_t)arg);
  } else emit_bytes(get_op, (uint8_t)arg);
}

void expr_variable(bool can_assign) {
  named_variable(parser.previous, can_assign);
}

uint8_t argument_list() {
  uint8_t arg_count = 0;
  if ( !compiler_check(TOKEN_RIGHT_PAREN) ) do {
    expression(); if ( arg_count++ == 255 )
      error("Can't have more than 255 arguments.");
  } while ( compiler_match(TOKEN_COMMA) );
  compiler_consume(TOKEN_RIGHT_PAREN, "Expect ')' after arguments.");
  return arg_count;
}

void expr_call(bool can_assign) {
  uint8_t arg_count = argument_list();
  emit_bytes(OP_CALL, arg_count);
}

void expr_grouping(bool) {
  expression();
  compiler_consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
}

void expr_binary(bool) {
  TokenType optype = parser.previous.type;
  ParserRule* rule = get_rule(optype);
  parse_precedence((Precedence)(rule->precedence + 1));
  switch ( optype ) {
  case TOKEN_PLUS:          emit_byte(OP_ADD);              break;
  case TOKEN_LESS:          emit_byte(OP_LESS);             break;
  case TOKEN_EQUAL_EQUAL:   emit_byte(OP_EQUAL);            break;
  case TOKEN_SLASH:         emit_byte(OP_DIVIDE);           break;
  case TOKEN_GREATER:       emit_byte(OP_GREATER);          break;
  case TOKEN_STAR:          emit_byte(OP_MULTIPLY);         break;
  case TOKEN_MINUS:         emit_byte(OP_SUBTRACT);         break;
  case TOKEN_GREATER_EQUAL: emit_bytes(OP_LESS, OP_NOT);    break;
  case TOKEN_BANG_EQUAL:    emit_bytes(OP_EQUAL, OP_NOT);   break;
  case TOKEN_LESS_EQUAL:    emit_bytes(OP_GREATER, OP_NOT); break;
  default:
    printf("Unrecognized expr binary token type[%d]: '%s'\n",
      optype, inst_print(optype));
    exit(1);
  }
}

void expr_unary(bool) {
  TokenType optype = parser.previous.type;
  parse_precedence(PREC_UNARY);
  switch ( optype ) {
  case TOKEN_BANG: emit_byte(OP_NOT);     break;
  case TOKEN_MINUS: emit_byte(OP_NEGATE); break;
  default:
    printf("Unrecognized expr unary token type[%d]: '%s'\n",
      optype, inst_print(optype));
    exit(1);
  }
}

void parse_precedence(Precedence precedence) {
  compiler_advance();
  ParseFn prefix_rule = get_rule(parser.previous.type)->prefix;
  bool can_assign = precedence <= PREC_ASSIGNMENT;
  if ( prefix_rule != NULL ) prefix_rule(can_assign);
  else error("Expect expression.");
  while ( precedence <= get_rule(parser.current.type)->precedence ) {
    compiler_advance();
    get_rule(parser.previous.type)->infix(can_assign);
  }
  if ( can_assign && compiler_match(TOKEN_EQUAL) )
    error("Invalid assignment target.");
}

uint8_t identifier_constant(Token* token) {
  return make_constant(OBJECT_VAL(copy_string(token->start, token->length)));
}

void add_local(Token name) {
  if ( current->local_count == UINT8_COUNT ) {
    error("Too many local variables in scope.");
    return;
  }
  Local* local = current->locals + (current->local_count++);
  local->depth = -1;
  local->name = name;
}

bool identifier_equal(Token* id1, Token* id2) {
  if ( id1->length != id2->length ) return false;
  return !memcmp(id1->start, id2->start, id1->length);
}

void declare_variable() {
  if ( current->scope_depth == 0 ) return;
  Token* name = &parser.previous;
  Local* local;
  for ( int i = current->local_count - 1; i >= 0; --i ) {
    local = current->locals + i;
    if ( local->depth != -1 && local->depth < current->scope_depth ) break;
    if ( identifier_equal(name, &local->name) )
      error("Local variable already exists in the current scope.");
  } add_local(*name);
}

uint8_t parse_variable(const char* error_message) {
  compiler_consume(TOKEN_IDENTIFIER, error_message);
  declare_variable();
  if ( current->scope_depth > 0 ) return 0;
  return identifier_constant(&parser.previous);
}

void mark_initialized() {
  if ( current->scope_depth == 0 ) return;
  current->locals[current->local_count - 1].depth = current->scope_depth;
}

void define_variable(uint8_t global) {
  if ( current->scope_depth > 0 ) mark_initialized();
  else emit_bytes(OP_DEFINE_GLOBAL, global);
}

void consume_eos() {
  compiler_consume(TOKEN_SEMICOLON,
    "Expected ';' at the end of a statement.");
}

bool compiler_match(TokenType type) {
  if ( !compiler_check(type) ) return false;
  compiler_advance();
  return true;
}

void expression() { parse_precedence(PREC_ASSIGNMENT); }

void stmt_expression() {
  expression();
  consume_eos();
  emit_byte(OP_POP);
}

void stmt_print() {
  expression();
  consume_eos();
  emit_byte(OP_PRINT);
}

void stmt_block() {
  while ( !compiler_check(TOKEN_RIGHT_BRACE) && !compiler_check(TOKEN_EOF) )
    stmt_declaration();
  compiler_consume(TOKEN_RIGHT_BRACE, "Expect '}' after block.");
}

void scope_begin() { ++current->scope_depth; }

void scope_end() {
  Local* local;
  while ( current->local_count > 0 ) {
    local = current->locals + current->local_count - 1;
    if ( local->depth != current->scope_depth ) break;
    --current->local_count;
    emit_byte(current->locals[current->local_count - 1]
      .is_captured? OP_CLOSE_UPVALUE: OP_POP);
  }
  --current->scope_depth;
}

void stmt_sblock() {
  scope_begin();
  stmt_block();
  scope_end();
}

int emit_jump(OpCode jtype) {
  emit_byte(jtype);
  emit_bytes(0xff, 0xff);
  return current_chunk()->count - 2;
}

void emit_loop(int loop_start) {
  emit_byte(OP_LOOP);
  int offset = current_chunk()->count - loop_start + 2;
  if ( offset > UINT16_MAX ) error("Loop Body too large.");
  emit_byte((offset >> 8) & 0xff);
  emit_byte(offset & 0xff);
}

void patch_jump(int offset) {
  int jump = current_chunk()->count - offset - 2;
  if ( jump > UINT16_MAX ) error("Too much code to jump over.");
  current_chunk()->code[offset] = (jump >> 8) & 0xff;
  current_chunk()->code[++offset] = jump & 0xff;
}

void stmt_if() {
  compiler_consume(TOKEN_LEFT_PAREN, "Expect '(' after keyword 'if'");
  expression();
  compiler_consume(TOKEN_RIGHT_PAREN, "Expect ')' after if-condition.");
  int jump_then = emit_jump(OP_JUMP_IF_FALSE);
  emit_byte(OP_POP);
  stmt_statement();
  int jump_else = emit_jump(OP_JUMP);
  patch_jump(jump_then);
  emit_byte(OP_POP);
  if ( compiler_match(TOKEN_ELSE) ) stmt_statement();
  patch_jump(jump_else);
}

void stmt_while() {
  int loop_start = current_chunk()->count;
  compiler_consume(TOKEN_LEFT_PAREN, "Expect '(' after keyword while.");
  expression();
  compiler_consume(TOKEN_RIGHT_PAREN, "Expect ')' after while-condition.");
  int jump_exit = emit_jump(OP_JUMP_IF_FALSE);
  emit_byte(OP_POP);
  stmt_statement();
  emit_loop(loop_start);
  patch_jump(jump_exit);
  emit_byte(OP_POP);
}

void stmt_for() {
  scope_begin();
  compiler_consume(TOKEN_LEFT_PAREN, "Expect '(' after keyword for.");
  if ( compiler_match(TOKEN_SEMICOLON) );
  else if ( compiler_match(TOKEN_VAR) ) stmt_var();
  else stmt_expression();
  int loop_start = current_chunk()->count;
  int jump_exit = -1;
  if ( !compiler_match(TOKEN_SEMICOLON) ) {
    expression();
    compiler_consume(TOKEN_SEMICOLON, "Expect ';' after loop condition.");
    jump_exit = emit_jump(OP_JUMP_IF_FALSE);
    emit_byte(OP_POP);
  }
  if ( !compiler_match(TOKEN_RIGHT_PAREN) ) {
    int jump_body = emit_jump(OP_JUMP);
    int start_increment = current_chunk()->count;
    expression();
    emit_byte(OP_POP);
    compiler_consume(TOKEN_RIGHT_PAREN, "Expect ')' after for-clauses.");
    emit_loop(loop_start);
    loop_start = start_increment;
    patch_jump(jump_body);
  }
  stmt_statement();
  emit_loop(loop_start);
  if ( jump_exit != -1 ) {
    patch_jump(jump_exit);
    emit_byte(OP_POP);
  }
  scope_end();
}

void stmt_return() {
  if ( current->type == TYPE_SCRIPT )
    error("Can't return from top level code.");
  if ( compiler_match(TOKEN_SEMICOLON) ) emit_return();
  else {
    expression();
    consume_eos();
    emit_byte(OP_RETURN);
  }
}

void stmt_statement() {
  if ( compiler_match(TOKEN_SEMICOLON) ); // Empty statement
  else if ( compiler_match(TOKEN_IF) )         stmt_if();
  else if ( compiler_match(TOKEN_FOR) )        stmt_for();
  else if ( compiler_match(TOKEN_PRINT) )      stmt_print();
  else if ( compiler_match(TOKEN_WHILE) )      stmt_while();
  else if ( compiler_match(TOKEN_RETURN) )     stmt_return();
  else if ( compiler_match(TOKEN_LEFT_BRACE) ) stmt_sblock();
  else                                         stmt_expression();
}

void stmt_var() {
  uint8_t global = parse_variable("Expect variable name.");
  if ( compiler_match(TOKEN_EQUAL) ) expression();
  else emit_byte(OP_NIL);
  consume_eos();
  define_variable(global);
}

void consume_function(FunctionType type) {
  Compiler compiler;
  comp_init(&compiler, type);
  scope_begin();

  compiler_consume(TOKEN_LEFT_PAREN, "Expect '(' after function name.");
  if ( !compiler_check(TOKEN_RIGHT_PAREN) ) {
    do {
      current->function->arity++;
      if ( current->function->arity > 255 )
        error_at("Can't have more than 255 parameters.");
      uint8_t param = parse_variable("Expect parameter name.");
      define_variable(param);
    } while ( compiler_match(TOKEN_COMMA) );
  }
  compiler_consume(TOKEN_RIGHT_PAREN, "Expect ')' after parameters.");

  compiler_consume(TOKEN_LEFT_BRACE, "Expect '{' before function body.");
  stmt_block();
  scope_end();

  ObjectFunction* function = compiler_delete();
  emit_bytes(OP_CLOSURE, make_constant(OBJECT_VAL(function)));
  for ( int i = 0; i < function->upvalue_count; ++i )
    emit_bytes(compiler.upvalues[i].is_local ? 1 : 0, compiler.upvalues[i].index);
}

void stmt_fun() {
  uint8_t global = parse_variable("Expect function name");
  mark_initialized();
  consume_function(TYPE_FUNCTION);
  define_variable(global);
}

void stmt_class() {
  compiler_consume(TOKEN_IDENTIFIER, "Expect class name.");
  uint8_t name_constant = identifier_constant(&parser.previous);
  declare_variable();
  emit_bytes(OP_CLASS, name_constant);
  define_variable(name_constant);
  compiler_consume(TOKEN_LEFT_BRACE, "Expect '{' before class body.");
  compiler_consume(TOKEN_RIGHT_BRACE, "Expect '}' after class body.");
}

void stmt_declaration() {
  if ( compiler_match(TOKEN_VAR) )        stmt_var();
  else if ( compiler_match(TOKEN_FUN) )   stmt_fun();
  else if ( compiler_match(TOKEN_CLASS) ) stmt_class();
  else stmt_statement();
  if ( parser.panic_mode ) compiler_sync();
}

void compiler_init() {
  parser.panic_mode = false;
  parser.had_error = false;
}

ObjectFunction* compiler_delete() {
  emit_return();
  ObjectFunction* function = current->function;
  current = current->enclosing;
  return function;
}

ObjectFunction* compile(const char* source) {
  // puts("----------------- 'COMPILER_START' ---------------");
  Compiler compiler;
  compiler_init();
  comp_init(&compiler, TYPE_SCRIPT);
  scanner_init(source);
  compiler_advance();
  while ( !compiler_match(TOKEN_EOF) ) stmt_declaration();
  compiler_consume(TOKEN_EOF, "Expect end of expression.");
  ObjectFunction* function = compiler_delete();
  // puts("----------------- 'COMPILER_END' -----------------");
  return parser.had_error ? NULL : function;
}

void gc_mark_object(Object *);

void gc_mark_compiler_roots() {
  Compiler* comp = current;
  while ( comp != NULL ) {
    gc_mark_object((Object *)comp->function);
    comp = comp->enclosing;
  }
}

CLOX_END_DECLS

#endif //_CLOX_COMPILER_H
