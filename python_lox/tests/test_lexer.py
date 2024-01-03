from ..lexer import Lexer
from ..lox import Lox
from ..token import Token, TokenType


def test_function_lexing():
    source = """
fun hello(name) {
    print "Hello " + name;
}
    """.strip()
    tokens = [
        Token(TokenType.FUN, "fun", None, 1),
        Token(TokenType.IDENTIFIER, "hello", None, 1),
        Token(TokenType.LEFT_PAREN, "(", None, 1),
        Token(TokenType.IDENTIFIER, "name", None, 1),
        Token(TokenType.RIGHT_PAREN, ")", None, 1),
        Token(TokenType.LEFT_BRACE, "{", None, 1),
        Token(TokenType.PRINT, "print", None, 2),
        Token(TokenType.STRING, '"Hello "', "Hello ", 2),
        Token(TokenType.PLUS, "+", None, 2),
        Token(TokenType.IDENTIFIER, "name", None, 2),
        Token(TokenType.SEMICOLON, ";", None, 2),
        Token(TokenType.RIGHT_BRACE, "}", None, 3),
        Token(TokenType.EOF, "", None, 3),
    ]
    lexer = Lexer(source, Lox())
    assert tokens == lexer.scan_tokens()


def test_class_lexing():
    source = """
class Person {
    init(name) {
        this.name = name;
    }
    hello(name) {
        print "Hello " + name + ", my name is " + this.name;
    }
}
    """.strip()
    tokens = [
        Token(TokenType.CLASS, "class", None, 1),
        Token(TokenType.IDENTIFIER, "Person", None, 1),
        Token(TokenType.LEFT_BRACE, "{", None, 1),
        Token(TokenType.IDENTIFIER, "init", None, 2),
        Token(TokenType.LEFT_PAREN, "(", None, 2),
        Token(TokenType.IDENTIFIER, "name", None, 2),
        Token(TokenType.RIGHT_PAREN, ")", None, 2),
        Token(TokenType.LEFT_BRACE, "{", None, 2),
        Token(TokenType.THIS, "this", None, 3),
        Token(TokenType.DOT, ".", None, 3),
        Token(TokenType.IDENTIFIER, "name", None, 3),
        Token(TokenType.EQUAL, "=", None, 3),
        Token(TokenType.IDENTIFIER, "name", None, 3),
        Token(TokenType.SEMICOLON, ";", None, 3),
        Token(TokenType.RIGHT_BRACE, "}", None, 4),
        Token(TokenType.IDENTIFIER, "hello", None, 5),
        Token(TokenType.LEFT_PAREN, "(", None, 5),
        Token(TokenType.IDENTIFIER, "name", None, 5),
        Token(TokenType.RIGHT_PAREN, ")", None, 5),
        Token(TokenType.LEFT_BRACE, "{", None, 5),
        Token(TokenType.PRINT, "print", None, 6),
        Token(TokenType.STRING, '"Hello "', "Hello ", 6),
        Token(TokenType.PLUS, "+", None, 6),
        Token(TokenType.IDENTIFIER, "name", None, 6),
        Token(TokenType.PLUS, "+", None, 6),
        Token(TokenType.STRING, '", my name is "', ", my name is ", 6),
        Token(TokenType.PLUS, "+", None, 6),
        Token(TokenType.THIS, "this", None, 6),
        Token(TokenType.DOT, ".", None, 6),
        Token(TokenType.IDENTIFIER, "name", None, 6),
        Token(TokenType.SEMICOLON, ";", None, 6),
        Token(TokenType.RIGHT_BRACE, "}", None, 7),
        Token(TokenType.RIGHT_BRACE, "}", None, 8),
        Token(TokenType.EOF, "", None, 8),
    ]
    lexer = Lexer(source, Lox())
    assert tokens == lexer.scan_tokens()


def test_comment_variable_lexing():
    source = """
// This is a comment.
// They should be ignored. Whole line.
var name = "Simon Nganga";    
var age = 21; // Even this one.
var weight=90.81;
var nothing=nil;//Set to nothing
// Some more comment here.

var desc = "
I am tall.
I am handsome.
I have two eyes.
"; // This was a multiline string.
    """
    tokens = [
        Token(TokenType.VAR, "var", None, 4),
        Token(TokenType.IDENTIFIER, "name", None, 4),
        Token(TokenType.EQUAL, "=", None, 4),
        Token(TokenType.STRING, '"Simon Nganga"', "Simon Nganga", 4),
        Token(TokenType.SEMICOLON, ";", None, 4),
        Token(TokenType.VAR, "var", None, 5),
        Token(TokenType.IDENTIFIER, "age", None, 5),
        Token(TokenType.EQUAL, "=", None, 5),
        Token(TokenType.NUMBER, "21", 21, 5),
        Token(TokenType.SEMICOLON, ";", None, 5),
        Token(TokenType.VAR, "var", None, 6),
        Token(TokenType.IDENTIFIER, "weight", None, 6),
        Token(TokenType.EQUAL, "=", None, 6),
        Token(TokenType.NUMBER, "90.81", 90.81, 6),
        Token(TokenType.SEMICOLON, ";", None, 6),
        Token(TokenType.VAR, "var", None, 7),
        Token(TokenType.IDENTIFIER, "nothing", None, 7),
        Token(TokenType.EQUAL, "=", None, 7),
        Token(TokenType.NIL, "nil", None, 7),
        Token(TokenType.SEMICOLON, ";", None, 7),
        Token(TokenType.VAR, "var", None, 10),
        Token(TokenType.IDENTIFIER, "desc", None, 10),
        Token(TokenType.EQUAL, "=", None, 10),
        Token(
            TokenType.STRING,
            '"\nI am tall.\nI am handsome.\nI have two eyes.\n"',
            "\nI am tall.\nI am handsome.\nI have two eyes.\n",
            10,
        ),
        Token(TokenType.SEMICOLON, ";", None, 14),
        Token(TokenType.EOF, "", None, 15),
    ]
    lexer = Lexer(source, Lox())
    assert tokens == lexer.scan_tokens()


def test_control_flow_lexing():
    source = """// Tricky comment.
var age = 20;
if (age >= 30) {
    print "Too old";
} else if (age<20) {
    print "Too young";
} else {
    print "perfect";
}
while (age>=20) {
    age = age - 1;
}
for (var cnt=0; cnt < 10 and age<=20; cnt = cnt+1) {
    print "Counting";
    if (cnt == 5 or age==15) {
        print "booya!!";
    }
} // last comment
    """
    tokens = [
        Token(TokenType.VAR, "var", None, 2),
        Token(TokenType.IDENTIFIER, "age", None, 2),
        Token(TokenType.EQUAL, "=", None, 2),
        Token(TokenType.NUMBER, "20", 20, 2),
        Token(TokenType.SEMICOLON, ";", None, 2),
        Token(TokenType.IF, "if", None, 3),
        Token(TokenType.LEFT_PAREN, "(", None, 3),
        Token(TokenType.IDENTIFIER, "age", None, 3),
        Token(TokenType.GREATER_THAN, ">=", None, 3),
        Token(TokenType.NUMBER, "30", 30, 3),
        Token(TokenType.RIGHT_PAREN, ")", None, 3),
        Token(TokenType.LEFT_BRACE, "{", None, 3),
        Token(TokenType.PRINT, "print", None, 4),
        Token(TokenType.STRING, '"Too old"', "Too old", 4),
        Token(TokenType.SEMICOLON, ";", None, 4),
        Token(TokenType.RIGHT_BRACE, "}", None, 5),
        Token(TokenType.ELSE, "else", None, 5),
        Token(TokenType.IF, "if", None, 5),
        Token(TokenType.LEFT_PAREN, "(", None, 5),
        Token(TokenType.IDENTIFIER, "age", None, 5),
        Token(TokenType.LESS, "<", None, 5),
        Token(TokenType.NUMBER, "20", 20, 5),
        Token(TokenType.RIGHT_PAREN, ")", None, 5),
        Token(TokenType.LEFT_BRACE, "{", None, 5),
        Token(TokenType.PRINT, "print", None, 6),
        Token(TokenType.STRING, '"Too young"', "Too young", 6),
        Token(TokenType.SEMICOLON, ";", None, 6),
        Token(TokenType.RIGHT_BRACE, "}", None, 7),
        Token(TokenType.ELSE, "else", None, 7),
        Token(TokenType.LEFT_BRACE, "{", None, 7),
        Token(TokenType.PRINT, "print", None, 8),
        Token(TokenType.STRING, '"perfect"', "perfect", 8),
        Token(TokenType.SEMICOLON, ";", None, 8),
        Token(TokenType.RIGHT_BRACE, "}", None, 9),
        Token(TokenType.WHILE, "while", None, 10),
        Token(TokenType.LEFT_PAREN, "(", None, 10),
        Token(TokenType.IDENTIFIER, "age", None, 10),
        Token(TokenType.GREATER_THAN, ">=", None, 10),
        Token(TokenType.NUMBER, "20", 20, 10),
        Token(TokenType.RIGHT_PAREN, ")", None, 10),
        Token(TokenType.LEFT_BRACE, "{", None, 10),
        Token(TokenType.IDENTIFIER, "age", None, 11),
        Token(TokenType.EQUAL, "=", None, 11),
        Token(TokenType.IDENTIFIER, "age", None, 11),
        Token(TokenType.MINUS, "-", None, 11),
        Token(TokenType.NUMBER, "1", 1, 11),
        Token(TokenType.SEMICOLON, ";", None, 11),
        Token(TokenType.RIGHT_BRACE, "}", None, 12),
        Token(TokenType.FOR, "for", None, 13),
        Token(TokenType.LEFT_PAREN, "(", None, 13),
        Token(TokenType.VAR, "var", None, 13),
        Token(TokenType.IDENTIFIER, "cnt", None, 13),
        Token(TokenType.EQUAL, "=", None, 13),
        Token(TokenType.NUMBER, "0", 0, 13),
        Token(TokenType.SEMICOLON, ";", None, 13),
        Token(TokenType.IDENTIFIER, "cnt", None, 13),
        Token(TokenType.LESS, "<", None, 13),
        Token(TokenType.NUMBER, "10", 10, 13),
        Token(TokenType.AND, "and", None, 13),
        Token(TokenType.IDENTIFIER, "age", None, 13),
        Token(TokenType.LESS_THAN, "<=", None, 13),
        Token(TokenType.NUMBER, "20", 20, 13),
        Token(TokenType.SEMICOLON, ";", None, 13),
        Token(TokenType.IDENTIFIER, "cnt", None, 13),
        Token(TokenType.EQUAL, "=", None, 13),
        Token(TokenType.IDENTIFIER, "cnt", None, 13),
        Token(TokenType.PLUS, "+", None, 13),
        Token(TokenType.NUMBER, "1", 1, 13),
        Token(TokenType.RIGHT_PAREN, ")", None, 13),
        Token(TokenType.LEFT_BRACE, "{", None, 13),
        Token(TokenType.PRINT, "print", None, 14),
        Token(TokenType.STRING, '"Counting"', "Counting", 14),
        Token(TokenType.SEMICOLON, ";", None, 14),
        Token(TokenType.IF, "if", None, 15),
        Token(TokenType.LEFT_PAREN, "(", None, 15),
        Token(TokenType.IDENTIFIER, "cnt", None, 15),
        Token(TokenType.EQUAL_EQUAL, "==", None, 15),
        Token(TokenType.NUMBER, "5", 5, 15),
        Token(TokenType.OR, "or", None, 15),
        Token(TokenType.IDENTIFIER, "age", None, 15),
        Token(TokenType.EQUAL_EQUAL, "==", None, 15),
        Token(TokenType.NUMBER, "15", 15, 15),
        Token(TokenType.RIGHT_PAREN, ")", None, 15),
        Token(TokenType.LEFT_BRACE, "{", None, 15),
        Token(TokenType.PRINT, "print", None, 16),
        Token(TokenType.STRING, '"booya!!"', "booya!!", 16),
        Token(TokenType.SEMICOLON, ";", None, 16),
        Token(TokenType.RIGHT_BRACE, "}", None, 17),
        Token(TokenType.RIGHT_BRACE, "}", None, 18),
        Token(TokenType.EOF, "", None, 19),
    ]
    lexer = Lexer(source, Lox())
    assert tokens == lexer.scan_tokens()
