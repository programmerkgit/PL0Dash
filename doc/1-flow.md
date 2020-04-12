# globals
sourceFile
targetFile
ch
printed
charClassT
token
static int spaces;            /*　そのトークンの前のスペースの個数　*/
static int CR;                /*　その前のCRの個数(開業)　*/
TAB: 5 改行5個分

1. open source and targetFile file
sourceFile and targetFile are set
2. compile()
3. initSource()
4. ch = '\n': why ?? 最初に読み込んだ文字
5. printed = 1 why ?? Tokenは印字済み ??
6. initCharClassT(charClassT);
initialize charClassT table ['(' => 'left brace']
7. writeInitCodeToTargetFile(targetFile);
initialize targetFile by strings why ??
8. token = nextToken();
printCurrentToken()
i = currentToken.kind
printSpaces();                /*　トークンの前の空白や改行印字　*/

spaces = 0;
CR = 0
9. Block