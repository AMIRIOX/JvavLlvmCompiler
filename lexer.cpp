#include <cctype>
#include <memory>
#include <string>
#include <vector>
#include <map>

using namespace std;
enum Token {
    tokEof = -1,         //文件结束
    tokDef = -2,         // def关键字
    tokExtern = -3,      // extern关键字
    tokIdentifier = -4,  //标识符
    tokNum = -5,         //数字
};

static string identifierStr;  //标识符字符串
static double numValue;       //数字的值

// // auto LHS = make_unique<variableExprAST>("x");
// // auto RHS = make_unique<variableExprAST>("y");
// // auto result = make_unique<binaryExprAST>('+',move(LHS),move(RHS));

// curTok/getNextToken - provide a simpile token buffer.
// CurTok is the current token the parser is looking at.
// getNextToken reads another token form the lexer and updates CurTok with its
// results
static int curTok;
static int getNextToken() { return curTok = returnNextTokenFromInput(); }

// logError - help function for error handling
unique_ptr<exprAST> logError(const char* Str) {
    fprintf(stderr, "logError:%s\n", Str);
    return nullptr;
}
unique_ptr<prototypeAST> prototypeError(const char* Str) {
    logError(Str);
    return nullptr;
}

// number expression
static unique_ptr<exprAST> parseNumberExpr() {
    auto result = make_unique<numExprAST>(numValue);
    getNextToken();  // consume the number
    return move(result);
}

// paren expression
static unique_ptr<exprAST> parseParenExpr() {
    getNextToken();  // eat (
    auto V = parseExpression();

    if (!V) return logError("expectrd ')'");
    getNextToken();  // eat )
    return V;
}

// identifier
static unique_ptr<exprAST> parseIdentifierExpr() {
    string idName = identifierStr;

    getNextToken();

    if (curTok != '(') return make_unique<variableExprAST>(idName);

    // call
    getNextToken();
    vector<unique_ptr<exprAST>> args;
    if (curTok != ')') {
        while (1) {
            if (auto Arg = parseExpression())
                args.push_back(move(Arg));
            else
                return nullptr;

            if (curTok == ')') break;

            if (curTok != ',')
                return logError("expected ')' or ','in argument list");
            getNextToken();
        }
    }
    getNextToken();
    return make_unique<callExprAST>(idName, move(args));
}
// primary
// identifier,numberexpr,parenexpr
static unique_ptr<exprAST> parsePrimay() {
    switch (curTok) {
        case tokIdentifier:
            return parseIdentifierExpr();
        case tokNum:
            return parseNumberExpr();
        case '(':
            return parseParenExpr();
        default:
            return logError("unkown token when expecting an expression");
    }
}

// exprAST - Base class for all expression nodes on AST
class exprAST {
   public:
    virtual ~exprAST() {}
};

// numExprAST - Expression class for numeric literals like "0.1"
class numExprAST : public exprAST {
   private:
    double Val;

   public:
    numExprAST(double value) : Val(value) {}
};

// variableExprAST - Expression class for referencing a variable, like "a" of
// "int a"
class variableExprAST : public exprAST {
   private:
    string name;

   public:
    variableExprAST(const string& varName) : name(varName) {}
};

// binaryExprAST - Expression class of a binary operator.
class binaryExprAST : public exprAST {
   private:
    char op;
    unique_ptr<exprAST> LHS, RHS;

   public:
    binaryExprAST(char astOp, unique_ptr<exprAST> astLHS,
                  unique_ptr<exprAST> astRHS)
        : op(astOp), LHS(move(astLHS)), RHS(move(astRHS)) {}
};

// callExprAST - Expression class for function calls
class callExprAST : public exprAST {
   private:
    string callee;
    vector<unique_ptr<exprAST>> args;

   public:
    callExprAST(const string& funcCallee, vector<unique_ptr<exprAST>> funcArgs)
        : callee(funcCallee), args(move(funcArgs)) {}
};

// prototypeAST - Represents the "prototype" for a function,
// which captures its name, and its argument names(thus implicitly the number of
// arguments the function takes)
class prototypeAST {
   private:
    string name;
    vector<string> args;

   public:
    prototypeAST(const string& Name, vector<string> Args)
        : name(move(Name)), args(move(Args)) {}
};

// functionAST - represents a function definition itself
class functionAST {
   private:
    unique_ptr<prototypeAST> prototype;
    unique_ptr<exprAST> body;

   public:
    functionAST(unique_ptr<prototypeAST> proto, unique_ptr<exprAST> bod)
        : prototype(move(proto)), body(move(bod)) {}
};

//bin op precedence  - holds the precedence for each binary operator.
static map<char, int> BinOpPrecedence;

//GetTokPrecen - Get the precedence of the pending binary operator token.
static int getTokPrecedence() {
    if(!isascii(curTok))
        return -1;
    
    //make sure it is a declared binary operator
    int tokPrec = BinOpPrecedence[curTok];
    if(tokPrec <= 0) return -1;
    return tokPrec;
}

// returnNextTokenFromInput - return next token form standard input
static int returnNextTokenFromInput() {
    static int lastChar = ' ';

    // delete the whitespace
    while (isspace(lastChar)) lastChar = getchar();

    if (isalpha(lastChar)) {
        // identifier of source
        identifierStr = lastChar;
        while (isalnum(lastChar = getchar())) identifierStr += lastChar;
        if (identifierStr == "def") return tokDef;
        if (identifierStr == "extern") return tokExtern;
        return tokIdentifier;
    }

    if (isdigit(lastChar) || lastChar == '.') {
        // digit of source
        string numStr;
        do {
            numStr += lastChar;
            lastChar = getchar();
        } while (isdigit(lastChar) || lastChar == '.');
        numValue = strtod(numStr.c_str(), 0);
        return tokNum;
    }

    if (lastChar == '#') {
        // process comment
        do {
            lastChar = getchar();
        } while (lastChar != EOF && lastChar != '\n' && lastChar != '\r');
        if (lastChar != EOF) return returnNextTokenFromInput();
    }

    // process EOF
    if (lastChar == EOF) return tokEof;
    int retChar = lastChar;
    lastChar = getchar();
    return retChar;
}

static unique_ptr<exprAST> parseExpression() {
    auto LHS = parsePrimay();
    if(!LHS)
        return nullptr;
    
    return parseBinaryOperatorRHS(0,move(LHS));
}

//binary Operator 
static unique_ptr<exprAST> parseBinaryOperatorRHS(int exprPrec, unique_ptr<exprAST> LHS){
    //if is a binary operator, find its precedence.
    while(1) {
        int tokPrec = getTokPrecedence();

        if(tokPrec < exprPrec)
            return LHS;
        
        int binOp = curTok;
        getNextToken();
        auto RHS = parsePrimay();
        if(!RHS) return nullptr;
        
        int nextPrec = getTokPrecedence();
        if(tokPrec < nextPrec) {
            // TODO : create AST node for a_b expresson
        }

        //Merge LHS/RHS
        LHS = make_unique<binaryExprAST>(binOp,move(LHS),move(RHS));
    }
}

int main() {
    BinOpPrecedence['<'] = 10;
    BinOpPrecedence['+'] = 20;
    BinOpPrecedence['-'] = 30;
    BinOpPrecedence['*'] = 40;
    // BinOpPrecedence['>'] = 50;
    // BinOpPrecedence['/'] = 60;
    
    return 0;
}