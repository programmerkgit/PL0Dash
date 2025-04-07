
/**************getSource.c************/

#include <stdio.h>
#include <string.h>
#include "getSource.h"

#include <stdlib.h>

#define MAXLINE 120        /*　１行の最大文字数　*/
#define MAXERROR 30        /*　これ以上のエラーがあったら終り　*/
#define MAXNUM  14        /*　定数の最大桁数　*/
#define TAB   5                /*　タブのスペース　*/

static FILE *sourceFile;                /*　ソースファイル　*/
static FILE *targetFile;            /*　LaTeX出力ファイル　*/
static char line[MAXLINE];    /*　１行分の入力バッファー　*/
static int lineIndex = -1;            /*　次に読む文字の位置　*/
static char ch;                /*　最後に読んだ文字　*/

static Token currentToken;            /*　最後に読んだトークン　*/
static KindT idKind;            /*　現トークン(Identifier)の種類　*/
static int spaces;            /*　そのトークンの前のスペースの個数　*/
static int CR;                /*　その前のCRの個数　*/
static int printed = 0;            /*　トークンは印字済みか　*/

static int errorNo = 0;            /*　出力したエラーの数　*/
static char nextChar();        /*　次の文字を読む関数　*/
static int isKeySym(KeyId k);    /*　tは記号か？　*/
static int isKeyWd(KeyId k);        /*　tは予約語か？　*/
static void printSpaces();        /*　トークンの前のスペースの印字　*/
static void printCurrentToken();        /*　トークンの印字　*/
static void writeInitCodeToTargetFile(FILE *targetFile);

struct keyWd {                /*　予約語や記号と名前(KeyId)　*/
    char *word;
    KeyId keyId;
};

static struct keyWd KeyWdT[] = {    /*　予約語や記号と名前(KeyId)の表　*/
        {"begin",    Begin},
        {"end",      End},
        {"if",       If},
        {"then",     Then},
        {"while",    While},
        {"do",       Do},
        {"return",   Ret},
        {"function", Func},
        {"var",      Var},
        {"const",    Const},
        {"odd",      Odd},
        {"write",    Write},
        {"writeln",  WriteLn},
        {"$dummy1",  end_of_KeyWd},
        /*　記号と名前(KeyId)の表　*/
        {"+",        Plus},
        {"-",        Minus},
        {"*",        Mult},
        {"/",        Div},
        {"(",        Lparen},
        {")",        Rparen},
        {"=",        Equal},
        {"<",        Lss},
        {">",        Gtr},
        {"<>",       NotEq},
        {"<=",       LssEq},
        {">=",       GtrEq},
        {",",        Comma},
        {".",        Period},
        {";",        Semicolon},
        {":=",       Assign},
        {"$dummy2",  end_of_KeySym}
};

int isKeyWd(KeyId k)            /*　キーkは予約語か？　*/
{
    return (k < end_of_KeyWd);
}

int isKeySym(KeyId k)        /*　キーkは記号か？　*/
{
    if (k < end_of_KeyWd)
        return 0;
    return (k < end_of_KeySym);
}

/*　文字の種類を示す表にする　*/
static KeyId charClassT[256];

static void initCharClassT(KeyId *charClassT)        /*　文字の種類を示す表を作る関数　*/
{
    int i;
    for (i = 0; i < 256; i++)
        charClassT[i] = others;
    for (i = '0'; i <= '9'; i++)
        charClassT[i] = digit;
    for (i = 'A'; i <= 'Z'; i++)
        charClassT[i] = letter;
    for (i = 'a'; i <= 'z'; i++)
        charClassT[i] = letter;
    charClassT['+'] = Plus;
    charClassT['-'] = Minus;
    charClassT['*'] = Mult;
    charClassT['/'] = Div;
    charClassT['('] = Lparen;
    charClassT[')'] = Rparen;
    charClassT['='] = Equal;
    charClassT['<'] = Lss;
    charClassT['>'] = Gtr;
    charClassT[','] = Comma;
    charClassT['.'] = Period;
    charClassT[';'] = Semicolon;
    charClassT[':'] = colon;
}

/*　Ref: ソースファイルのopen　*/
int openSourceAndTarget(char *sourceFileName, char *targetFileName) {
    /* open source file */
    if ((sourceFile = fopen(sourceFileName, "r")) == NULL) {
        printf("can't open %s\n", sourceFileName);
        return 0;
    }
    /* open target file */
    if ((targetFile = fopen(targetFileName, "w")) == NULL) {
        printf("can't open %s\n", targetFileName);
        return 0;
    }
    return 1;
}

/*　ソースファイルと.texファイルをclose　*/
void closeSourceAndTarget() {
    fclose(sourceFile);
    fclose(targetFile);
}

/* New: initialize env */
void writeInitCodeToTargetFile(FILE *targetFile) {
    fprintf(targetFile, "\\documentstyle[12pt]{article}\n");   /*　LaTeXコマンド　*/
    fprintf(targetFile, "\\begin{document}\n");
    fprintf(targetFile, "\\fboxsep=0pt\n");
    fprintf(targetFile, "\\def\\insert#1{$\\fbox{#1}$}\n");
    fprintf(targetFile, "\\def\\delete#1{$\\fboxrule=.5mm\\fbox{#1}$}\n");
    fprintf(targetFile, "\\rm\n");
}

/* Ret: initialize env */
void initSource() {
    ch = '\n';
    printed = 1;
    initCharClassT(charClassT);
    writeInitCodeToTargetFile(targetFile);
}


/* period ends file */
void finalSource() {
    if (currentToken.kind == Period)
        printCurrentToken();
    else
        errorInsert(Period);
    fprintf(targetFile, "\n\\end{document}\n");
}

/*　通常のエラーメッセージの出力の仕方（参考まで）　*/
/*
void error(char *m)
{
	if (lineIndex > 0)
		printf("%*s\n", lineIndex, "***^");
	else
		printf("^\n");
	printf("*** error *** %s\n", m);
	errorNo++;
	if (errorNo > MAXERROR){
		printf("too many errors\n");
		printf("abort compilation\n");
		exit (1);
	}
}
*/

void errorNoCheck()            /*　エラーの個数のカウント、多すぎたら終わり　*/
{
    if (errorNo++ > MAXERROR) {
        fprintf(targetFile, "too many errors\n\\end{document}\n");
        printf("abort compilation\n");
        exit(1);
    }
}

void errorType(char *m)        /*　型エラーを.texファイルに出力　*/
{
    printSpaces();
    fprintf(targetFile, "\\(\\stackrel{\\mbox{\\scriptsize %s}}{\\mbox{", m);
    printCurrentToken();
    fprintf(targetFile, "}}\\)");
    errorNoCheck();
}

void errorInsert(KeyId k)        /*　keyString(k)を.texファイルに挿入　*/
{
    if (k < end_of_KeyWd)    /*　予約語　*/
        fprintf(targetFile, "\\ \\insert{{\\bf %s}}", KeyWdT[k].word);
    else                    /*　演算子か区切り記号　*/
        fprintf(targetFile, "\\ \\insert{$%s$}", KeyWdT[k].word);
    errorNoCheck();
}

void errorMissingId()            /*　名前がないとのメッセージを.texファイルに挿入　*/
{
    fprintf(targetFile, "\\insert{Identifier}");
    errorNoCheck();
}

void errorMissingOp()        /*　演算子がないとのメッセージを.texファイルに挿入　*/
{
    fprintf(targetFile, "\\insert{$\\otimes$}");
    errorNoCheck();
}

void errorDelete()            /*　今読んだトークンを読み捨てる　*/
{
    int i = (int) currentToken.kind;
    printSpaces();
    printed = 1;
    if (i < end_of_KeyWd)                            /*　予約語　*/
        fprintf(targetFile, "\\delete{{\\bf %s}}", KeyWdT[i].word);
    else if (i < end_of_KeySym)                    /*　演算子か区切り記号　*/
        fprintf(targetFile, "\\delete{$%s$}", KeyWdT[i].word);
    else if (i == (int) Identifier)                                /*　Identfier　*/
        fprintf(targetFile, "\\delete{%s}", currentToken.u.id);
    else if (i == (int) Num)                                /*　Num　*/
        fprintf(targetFile, "\\delete{%d}", currentToken.u.value);
}

void errorMessage(char *m)    /*　エラーメッセージを.texファイルに出力　*/
{
    fprintf(targetFile, "$^{%s}$", m);
    errorNoCheck();
}

void errorF(char *m)            /*　エラーメッセージを出力し、コンパイル終了　*/
{
    errorMessage(m);
    fprintf(targetFile, "fatal errors\n\\end{document}\n");
    if (errorNo)
        printf("total %d errors\n", errorNo);
    printf("abort compilation\n");
    exit(1);
}

int errorN()                /*　エラーの個数を返す　*/
{
    return errorNo;
}

char nextChar()                /*　次の１文字を返す関数　*/
{
    char ch;
    if (lineIndex == -1) {
        if (fgets(line, MAXLINE, sourceFile) != NULL) {
/*			puts(line); */    /*　通常のエラーメッセージの出力の場合（参考まで）　*/
            lineIndex = 0;
        } else {
            errorF("end of file\n");      /* end of fileならコンパイル終了 */
        }
    }
    if ((ch = line[lineIndex++]) == '\n') {     /*　chに次の１文字　*/
        lineIndex = -1;                /*　それが改行文字なら次の行の入力準備　*/
        return '\n';                /*　文字としては改行文字を返す　*/
    }
    return ch;
}

/*　次のトークンを読んで返す関数　*/
Token nextToken() {
    int i = 0;
    int num;
    KeyId cc;
    Token temp;
    /* identifier */
    char identifierName[MAXNAME];
    printCurrentToken();            /*　前のトークンを印字　*/
    spaces = 0;
    CR = 0;
    /*　次のトークンまでの空白や改行をカウント　*/
    while (1) {
        if (ch == ' ')
            spaces++;
        else if (ch == '\t')
            spaces += TAB;
        else if (ch == '\n') {
            spaces = 0;
            CR++;
        } else break;
        ch = nextChar();
    }
    /* 空白出ないchに対して */
    switch (cc = charClassT[ch]) {
        case letter:                /* identifier */
            /* while letter or digit */
            do {
                if (i < MAXNAME)
                    identifierName[i] = ch;
                i++;
                ch = nextChar();
            } while (charClassT[ch] == letter || charClassT[ch] == digit);
            if (i >= MAXNAME) {
                errorMessage("too long");
                i = MAXNAME - 1;
            }
            /* end of identifier */
            identifierName[i] = '\0';
            /*　全ての予約語をチェック。　*/
            for (i = 0; i < end_of_KeyWd; i++)
                if (strcmp(identifierName, KeyWdT[i].word) == 0) {
                    temp.kind = KeyWdT[i].keyId;
                    currentToken = temp;
                    printed = 0;
                    return temp;
                }
            temp.kind = Identifier;        /*　ユーザの宣言した名前の場合　*/
            strcpy(temp.u.id, identifierName);
            break;
        case digit:                    /* number */
            num = 0;
            do {
                num = 10 * num + (ch - '0');
                i++;
                ch = nextChar();
            } while (charClassT[ch] == digit);
            if (i > MAXNUM)
                errorMessage("too large");
            temp.kind = Num;
            temp.u.value = num;
            break;
        case colon:
            if ((ch = nextChar()) == '=') {
                ch = nextChar();
                temp.kind = Assign;        /*　":="　*/
                break;
            } else {
                temp.kind = nul;
                break;
            }
        case Lss:
            if ((ch = nextChar()) == '=') {
                ch = nextChar();
                temp.kind = LssEq;        /*　"<="　*/
                break;
            } else if (ch == '>') {
                ch = nextChar();
                temp.kind = NotEq;        /*　"<>"　*/
                break;
            } else {
                temp.kind = Lss;
                break;
            }
        case Gtr:
            if ((ch = nextChar()) == '=') {
                ch = nextChar();
                temp.kind = GtrEq;        /*　">="　*/
                break;
            } else {
                temp.kind = Gtr;
                break;
            }
        default:
            temp.kind = cc;
            ch = nextChar();
            break;
    }
    currentToken = temp;
    printed = 0;
    return temp;
}

/*　t.kind == k のチェック　*/
/*　t.kind == k なら、次のトークンを読んで返す　*/
/*　t.kind != k ならエラーメッセージを出し、t と k が共に記号、または予約語なら　*/
/*　t を捨て、次のトークンを読んで返す（ t を k で置き換えたことになる）　*/
/*　それ以外の場合、k を挿入したことにして、t を返す　*/
Token checkTokenAndGetNextToken(Token t, KeyId k) {
    if (t.kind == k)
        return nextToken();
    if ((isKeyWd(k) && isKeyWd(t.kind)) ||
        (isKeySym(k) && isKeySym(t.kind))) {
        errorDelete();
        errorInsert(k);
        return nextToken();
    }
    errorInsert(k);
    return t;
}

static void printSpaces()            /*　空白や改行の印字　*/
{
    while (CR-- > 0)
        fprintf(targetFile, "\\ \\par\n");
    while (spaces-- > 0)
        fprintf(targetFile, "\\ ");
    CR = 0;
    spaces = 0;
}

/*　現在のトークンの印字　*/
void printCurrentToken() {
    int i = (int) currentToken.kind;
    if (printed) {
        printed = 0;
        return;
    }
    printed = 1;
    printSpaces();                /*　トークンの前の空白や改行印字　*/
    if (i < end_of_KeyWd)                        /*　予約語　*/
        fprintf(targetFile, "{\\bf %s}", KeyWdT[i].word);
    else if (i < end_of_KeySym)                    /*　演算子か区切り記号　*/
        fprintf(targetFile, "$%s$", KeyWdT[i].word);
    else if (i == (int) Identifier) {                            /*　Identfier　*/
        switch (idKind) {
            case varId:
                fprintf(targetFile, "%s", currentToken.u.id);
                return;
            case parId:
                fprintf(targetFile, "{\\sl %s}", currentToken.u.id);
                return;
            case funcId:
                fprintf(targetFile, "{\\it %s}", currentToken.u.id);
                return;
            case constId:
                fprintf(targetFile, "{\\sf %s}", currentToken.u.id);
                return;
        }
    } else if (i == (int) Num)            /*　Num　*/
        fprintf(targetFile, "%d", currentToken.u.value);
}

void setIdKind(KindT k)            /*　現トークン(Identifier)の種類をセット　*/
{
    idKind = k;
}


