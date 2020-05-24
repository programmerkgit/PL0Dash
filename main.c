
/********* main.c *********/

#include <stdio.h>
#include "getSource.h"
#include "compile.h"
#include <string.h>


int main() {
    char fileName[140] = "/Users/admin/CLionProjects/PL0Dash/test.md";        /*　ソースプログラムファイルの名前　*/
    printf("enter source file name\n");
    // scanf("%s", fileName);

    char objectFileName[140];
    strcpy(objectFileName, fileName);
    strcat(objectFileName, ".tex");
    /*　ソースプログラムファイルのopen　*/
    if (!openSourceAndTarget(fileName, objectFileName))
        return 0;
    /*　openに失敗すれば終わり　*/
    /*　コンパイルして　*/
    if (compile()) {
        /*　エラーがなければ実行　*/
        execute();
    }
    closeSourceAndTarget();            /*　ソースプログラムファイルのclose　*/
}
