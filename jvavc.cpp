/*
 * @Author: AMIRIOX無暝 
 * @Date: 2020-06-15 12:02:36 
 * @Last Modified by:   AMIRIOX無暝 
 * @Last Modified time: 2020-06-15 12:02:36 
 */

#include <stddef.h>

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "llvm/ADT/None.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"

using namespace llvm;
using namespace std;

static int returnNextTokenFromInput();
static int getTokPrecedence();
static int getTokPrecedence();

class exprAST;
class numExprAST;
class variableExprAST;
class binaryExprAST;
class callExprAST;
class prototypeAST;
class functionAST;

static unique_ptr<exprAST> parseNumberExpr();
static unique_ptr<exprAST> parseParenExpr();
static unique_ptr<exprAST> parseIdentifierExpr();
static unique_ptr<exprAST> parsePrimay();
static unique_ptr<exprAST> parseExpression();
static unique_ptr<exprAST> parseBinaryOperatorRHS(int exprPrec,
                                                  unique_ptr<exprAST> LHS);
static unique_ptr<prototypeAST> parsePrototype();
static unique_ptr<functionAST> parseDefinition();
static unique_ptr<prototypeAST> parseExtern();
static unique_ptr<functionAST> parseTopLevelExpr();

/**
 * * 词法分析
 * * Author: Amiriox
 * TODO : NULL
 */

enum Token {
    tokEof = -1,         //文件结束
    tokDef = -2,         // def关键字
    tokExtern = -3,      // extern关键字
    tokIdentifier = -4,  //标识符
    tokNum = -5,         //数字
};

static string identifierStr;  //标识符字符串
static double numValue;       //数字的值

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

/**
 * * 抽象语法树
 * * Author: Amiriox
 * TODO : NULL
 * ! remark:{
 *   * 使用匿名namespace同于static修饰
 *   * 作用是将名称作用域限制在当前文件中
 *   * static无法修饰class, 另外可能产生歧义
 *   * 目前没有进行作用域限制.
 * !}
*/
// exprAST - Base class for all expression nodes on AST
class exprAST {
   public:
    virtual ~exprAST() {}
    virtual Value* codegen() = 0;
};

// numExprAST - Expression class for numeric literals like "0.1"
class numExprAST : public exprAST {
   private:
    double Val;

   public:
    numExprAST(double value) : Val(value) {}
    virtual Value* codegen();
};

// variableExprAST - Expression class for referencing a variable, like "a" of
// "int a"
class variableExprAST : public exprAST {
   private:
    string name;

   public:
    variableExprAST(const string& varName) : name(varName) {}
    virtual Value* codegen();
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
    virtual Value* codegen();

};

// callExprAST - Expression class for function calls
class callExprAST : public exprAST {
   private:
    string callee;
    vector<unique_ptr<exprAST>> args;

   public:
    callExprAST(const string& funcCallee, vector<unique_ptr<exprAST>> funcArgs)
        : callee(funcCallee), args(move(funcArgs)) {}
    virtual Value* codegen();

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
    virtual Value* codegen();

};

// functionAST - represents a function definition itself
class functionAST {
   private:
    unique_ptr<prototypeAST> prototype;
    unique_ptr<exprAST> body;

   public:
    functionAST(unique_ptr<prototypeAST> proto, unique_ptr<exprAST> bod)
        : prototype(move(proto)), body(move(bod)) {}
    virtual Value* codegen();

};

/**
 * * 表达式语法分析
 * * Author: Amiriox
 * TODO : NULL
*/
// bin op precedence  - holds the precedence for each binary operator.
static map<char, int> BinOpPrecedence;

// curTok/getNextToken - provide a simpile token buffer.
static int curTok;    // CurTok is the current token the parser is looking at.
static int getNextToken() { 
    return curTok = returnNextTokenFromInput(); 
}  // getNextToken reads another token form the lexer and updates CurTok with its results

// GetTokPrecen - Get the precedence of the pending binary operator token.
static int getTokPrecedence() {
    if (!isascii(curTok)) return -1;

    // make sure it is a declared binary operator
    int tokPrec = BinOpPrecedence[curTok];
    if (tokPrec <= 0) return -1;
    return tokPrec;
}

// logError - help function for error handling
unique_ptr<exprAST> logError(const char* Str) {
    fprintf(stderr, "logError:%s\n", Str);
    return NULL;
}
unique_ptr<prototypeAST> prototypeError(const char* Str) {
    logError(Str);
    return NULL;
}

// base expression
static unique_ptr<exprAST> parseExpression() {
    auto LHS = parsePrimay();
    if (!LHS) return NULL;

    return parseBinaryOperatorRHS(0, move(LHS));
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
                return NULL;

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

// binary Operator
static unique_ptr<exprAST> parseBinaryOperatorRHS(int exprPrec,
                                                  unique_ptr<exprAST> LHS) {
    // if is a binary operator, find its precedence.
    while (1) {
        int tokPrec = getTokPrecedence();

        if (tokPrec < exprPrec) return LHS;

        int binOp = curTok;
        getNextToken();
        auto RHS = parsePrimay();
        if (!RHS) return NULL;

        int nextPrec = getTokPrecedence();
        if (tokPrec < nextPrec) {
            // // TODO : create AST node for a_b expresson
            RHS = parseBinaryOperatorRHS(tokPrec + 1, move(RHS));
            if (!RHS)
                return NULL;
        }

        // Merge LHS/RHS
        LHS = make_unique<binaryExprAST>(binOp, move(LHS), move(RHS));
    }
}

// prototype
static unique_ptr<prototypeAST> parsePrototype() {
    if (curTok != tokIdentifier)
        return prototypeError("expected function name in prototype");
    string functionName = identifierStr;
    getNextToken();

    if (curTok != '(') return prototypeError("expected '(' in prototype");

    // read the list of argument names
    vector<string> argNames;
    while (getNextToken() == tokIdentifier) argNames.push_back(identifierStr);
    if (curTok != ')') return prototypeError("expected ')' int prototype");

    // success
    getNextToken();

    return make_unique<prototypeAST>(functionName, move(argNames));
}

// def function
static unique_ptr<functionAST> parseDefinition() {
    getNextToken();  // def identifier
    auto prototype = parsePrototype();
    if (!prototype) return NULL;

    if (auto body = parseExpression())
        return make_unique<functionAST>(move(prototype), move(body));
    return NULL;
}
// extern definition
static unique_ptr<prototypeAST> parseExtern() {
    getNextToken();  // extern def
    return parsePrototype();
}

// high level expression
static unique_ptr<functionAST> parseTopLevelExpr() {
    if (auto body = parseExpression()) {
        auto prototype = make_unique<prototypeAST>("" /*anonymous function*/,
                                                   vector<string>());
        return make_unique<functionAST>(move(prototype), move(body));
    }
    return NULL;
}

/**
 * * 代码生成: AST to LLVM IR
 * * Author: Amiriox
 * TODO : NULL
*/
static LLVMContext theContext;
static IRBuilder<> builder(theContext);
static unique_ptr<Module> theModule;
static map<string, Value*> namedValues;

Value* valueLogError(const char* str) {
    logError(str);
    return nullptr;
}

Value* binaryExprAST::codegen() {
    Value *L = LHS->codegen();
    Value *R = RHS->codegen();
    if(!L || !R){
        return nullptr;
    }

    switch (op)
    {
    case '+':
        return builder.CreateFAdd(L, R, "addtmp");
    case '-':
        return builder.CreateFSub(L, R, "subtmp");
    case '*':
        return builder.CreateFMul(L, R, "multmp");
    case '<':
        L = builder.CreateFCmpULT(L, R, "cmptmp");
        return builder.CreateUIToFP(L, Type::getDoubleTy(theContext), "booltmp");
    default:
        return valueLogError("invalid binary operator");
    }
}
Value* callExprAST::codegen() {
    //! remark {
    // * 在LLVM模块的符号表中
    //* 执行函数名查找 如sin和cos
    //! }
    Function* CalleeF = theModule->getFunction(callee);
    if(!CalleeF){
        return valueLogError("unknown function referenced");
    }

    if(CalleeF->arg_size() != args.size()){
        return valueLogError("Incorrect # arguments passed");
    }

    vector<Value*> argsV;
    for(unsigned i = 0, e=args.size();i!=e;++i){
        argsV.push_back(args[i]->codegen());
        if(!argsV.back()) return NULL;
    }

    return builder.CreateCall(CalleeF,argsV,"calltmp");
}

/**
 * * 顶级语法分析
 * * Author: Amiriox
 * TODO : NULL
*/

static void HandleDefinition() {
    if (parseDefinition()) {
        fprintf(stderr, "Parsed a function definition.\n");
    } else {
        // Skip token for error recovery.
        getNextToken();
    }
}

static void HandleExtern() {
    if (parseExtern()) {
        fprintf(stderr, "Parsed an extern\n");
    } else {
        // Skip token for error recovery.
        getNextToken();
    }
}

static void HandleTopLevelExpression() {
    // Evaluate a top-level expression into an anonymous function.
    if (parseTopLevelExpr()) {
        fprintf(stderr, "Parsed a top-level expr\n");
    } else {
        // Skip token for error recovery.
        getNextToken();
    }
}

static void MainLoop() {
    while (true) {
        fprintf(stderr, "ready> ");
        switch (curTok) {
            case tokEof:
                return;
            case ';':  // ignore top-level semicolons.
                getNextToken();
                break;
            case tokDef:
                HandleDefinition();
                break;
            case tokExtern:
                HandleExtern();
                break;
            default:
                HandleTopLevelExpression();
                break;
        }
    }
}
/**
 * * 入口
 * * Author: Amiriox
 * TODO : NULL
*/
int main() {
    BinOpPrecedence['<'] = 10;
    BinOpPrecedence['+'] = 20;
    BinOpPrecedence['-'] = 30;
    BinOpPrecedence['*'] = 40;
    // BinOpPrecedence['>'] = 50;
    // BinOpPrecedence['/'] = 60;

    // Prime the first token.
    fprintf(stderr, "ready> ");
    getNextToken();

    // Run the main "interpreter loop" now.
    MainLoop();

    return 0;
}