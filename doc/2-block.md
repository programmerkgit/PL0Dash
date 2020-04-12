1. block(0)


constDecl
    - setIdKind
    - temp = token;
    - token = checkTokenAndGetNextToken(nextToken(), Equal)
    - enterTconst(temp.u.id, token.u.value)
    - token = nextToken();


<!-- 文脈自由文法のコンパイルプロセス -->
Tokinize Sytnax Analyzer compile

Statement