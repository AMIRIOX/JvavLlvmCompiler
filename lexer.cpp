#include <cctype>
#include <memory>
#include <string>
#include <vector>

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
static int getNextToken() {
    return curTok = returnNextTokenFromInput();
}

//logError - help function for error handling
unique_ptr<exprAST> logError(const char* Str){
    fprintf(stderr,"logError:%s\n", Str);
    return nullptr;
}
unique_ptr<prototypeAST> prototypeLogError(const char* Str){
    logError(Str);
    return nullptr;
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