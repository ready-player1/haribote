#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef unsigned char *String;

int loadText(String path, String text, int size)
{
  unsigned char buf[1000];

  int startPos = path[0] == '"'; // ダブルクォートがあれば外す
  int i = 0;
  while (path[ startPos + i ] != 0 && path[ startPos + i ] != '"') {
    buf[i] = path[ startPos + i ];
    ++i;
  }
  buf[i] = 0;

  FILE *fp = fopen(buf, "rt");
  if (fp == NULL) {
    printf("failed to open %s\n", path);
    return 1;
  }

  int nItems = fread(text, 1, size - 1, fp);
  fclose(fp);
  text[nItems] = 0;
  return 0;
}

#define MAX_TOKEN_CODE 1000 // 格納できるトークンコードの最大値
String tokenStrs[ MAX_TOKEN_CODE + 1 ]; // 添字に指定したトークンコードに対応するトークン文字列のポインタを格納する
int    tokenLens[ MAX_TOKEN_CODE + 1 ]; // トークン文字列の長さを格納する
unsigned char tokenBuf[ (MAX_TOKEN_CODE + 1) * 10 ]; // トークン文字列の実体を格納する

int vars[ MAX_TOKEN_CODE + 1 ]; // 変数

int getTokenCode(String str, int len)
{
  static int nTokens = 0, unusedHead = 0; // 登録済みのトークンの数, 未使用領域へのポインタ

  int i;
  for (i = 0; i < nTokens; ++i) { // 登録済みのトークンコードの中から探す
    if (len == tokenLens[i] && strncmp(str, tokenStrs[i], len) == 0)
      break;
  }
  if (i == nTokens) {
    if (nTokens >= MAX_TOKEN_CODE) {
      printf("too many tokens\n");
      exit(1);
    }
    strncpy(&tokenBuf[ unusedHead ], str, len); // 見つからなかった場合は新規登録する
    tokenBuf[ unusedHead + len ] = 0;
    tokenStrs[i] = &tokenBuf[ unusedHead ];
    tokenLens[i] = len;
    unusedHead += len + 1;
    ++nTokens;
    vars[i] = strtol(tokenStrs[i], NULL, 0); // 定数だった場合に初期値を設定（定数ではないときは0になる）
  }
  return i;
}

inline static int isAlphabet(unsigned char ch)
{
  return ('a' <= ch && ch <= 'z') || ('A' <= ch && ch <= 'Z') || ch == '_';
}

inline static int isNumber(unsigned char ch)
{
  return '0' <= ch && ch <= '9';
}

int lexer(String str, int *tokenCodes)
{
  int pos = 0, nTokens = 0; // 現在読んでいる位置, これまでに変換したトークンの数
  int len;
  for (;;) {
    while (str[pos] == ' ' || str[pos] == '\t' || str[pos] == '\n' || str[pos] == '\r')
      ++pos;
    if (str[pos] == 0)
      return nTokens;

    len = 0;
    if (strchr("(){}[];,", str[pos]) != NULL)
      len = 1;
    else if (isAlphabet(str[pos]) || isNumber(str[pos])) {
      while (isAlphabet(str[pos + len]) || isNumber(str[pos + len]))
        ++len;
    }
    else if (strchr("=+-*/!%&~|<>?:.#", str[pos]) != NULL) {
      while (strchr("=+-*/!%&~|<>?:.#", str[pos + len]) != NULL && str[pos + len] != 0)
        ++len;
    }
    else {
      printf("syntax error: %.10s\n", &str[pos]);
      exit(1);
    }
    tokenCodes[nTokens] = getTokenCode(&str[pos], len);
    pos += len;
    ++nTokens;
  }
}

int tokenCodes[10000]; // トークンコードを格納する

int run(String sourceCode)
{
  clock_t t0 = clock();

  int nTokens = lexer(sourceCode, tokenCodes);

  int equal     = getTokenCode("==", 2);
  int notEq     = getTokenCode("!=", 2);
  int lesEq     = getTokenCode("<=", 2);
  int gtrEq     = getTokenCode(">=", 2);
  int les       = getTokenCode("<", 1);
  int gtr       = getTokenCode(">", 1);
  int colon     = getTokenCode(":", 1);
  int lparen    = getTokenCode("(", 1);
  int rparen    = getTokenCode(")", 1);
  int plus      = getTokenCode("+", 1);
  int minus     = getTokenCode("-", 1);
  int period    = getTokenCode(".", 1);
  int semicolon = getTokenCode(";", 1);
  int assign    = getTokenCode("=", 1);
  int print     = getTokenCode("print", 5);
  int _if       = getTokenCode("if", 2);
  int _goto     = getTokenCode("goto", 4);
  int time      = getTokenCode("time", 4);

  String *ts = tokenStrs;  // 添字に指定したトークンコードに対応するトークン文字列のポインタを格納している配列
  int    *tc = tokenCodes; // トークンコードを格納している配列
  tc[nTokens++] = semicolon; // 末尾に「;」を付け忘れることが多いので、付けてあげる
  tc[nTokens] = tc[nTokens + 1] = tc[nTokens + 2] = tc[nTokens + 3] = period; // エラー表示用
  int pc;
  for (pc = 0; pc < nTokens; ++pc) { // ラベル定義命令を探して位置を登録
    if (tc[pc + 1] == colon)
      vars[tc[pc]] = pc + 2; // ラベル定義命令の次のpc値を変数に記憶させておく
  }
  for (pc = 0; pc < nTokens;) {
    if (tc[pc + 1] == assign && tc[pc + 3] == semicolon)
      vars[tc[pc]] = vars[tc[pc + 2]];
    else if (tc[pc + 1] == assign && tc[pc + 3] == plus && tc[pc + 5] == semicolon)
      vars[tc[pc]] = vars[tc[pc + 2]] + vars[tc[pc + 4]];
    else if (tc[pc + 1] == assign && tc[pc + 3] == minus && tc[pc + 5] == semicolon)
      vars[tc[pc]] = vars[tc[pc + 2]] - vars[tc[pc + 4]];
    else if (tc[pc] == print && tc[pc + 2] == semicolon)
      printf("%d\n", vars[tc[pc + 1]]);
    else if (tc[pc + 1] == colon) { // ラベル定義命令
      pc += 2; // 読み飛ばす
      continue;
    }
    else if (tc[pc] == _goto && tc[pc + 2] == semicolon) {
      pc = vars[tc[pc + 1]];
      continue;
    }
    else if (tc[pc] == _if && tc[pc + 1] == lparen && tc[pc + 5] == rparen && tc[pc + 6] == _goto && tc[pc + 8] == semicolon) {
      int dest = vars[tc[pc + 7]], ope1 = vars[tc[pc + 2]], ope2 = vars[tc[pc + 4]];
      if (tc[pc + 3] == equal && ope1 == ope2) { pc = dest; continue; }
      if (tc[pc + 3] == notEq && ope1 != ope2) { pc = dest; continue; }
      if (tc[pc + 3] == lesEq && ope1 <= ope2) { pc = dest; continue; }
      if (tc[pc + 3] == gtrEq && ope1 >= ope2) { pc = dest; continue; }
      if (tc[pc + 3] == les   && ope1 < ope2)  { pc = dest; continue; }
      if (tc[pc + 3] == gtr   && ope1 > ope2)  { pc = dest; continue; }
    }
    else if (tc[pc] == time && tc[pc + 1] == semicolon)
      printf("time: %.3f[sec]\n", (clock() - t0) / (double) CLOCKS_PER_SEC);
    else if (tc[pc] == semicolon)
      ;
    else
      goto err;

    while (tc[pc] != semicolon)
      ++pc;
    ++pc; // セミコロンを読み飛ばす
  }
  return 0;
err:
  printf("syntax error: %s %s %s %s\n", ts[tc[pc]], ts[tc[pc + 1]], ts[tc[pc + 2]], ts[tc[pc + 3]]);
  return 1;
}

int main(int argc, const char **argv)
{
  unsigned char text[10000]; // ソースコード
  if (argc >= 2) {
    if (loadText((String) argv[1], text, 10000) != 0)
      exit(1);
    run(text);
    exit(0);
  }

  for (int nLines = 1;; ++nLines) { // Read-Eval-Print Loop
    printf("[%d]> ", nLines);
    fgets(text, 10000, stdin);
    int inputLen = strlen(text);
    if (text[inputLen - 1] == '\n') // chomp
      text[inputLen - 1] = 0;

    int hasRemovedSemicolon = 0;
    char *semicolonPos;
    for (int i = strlen(text) - 1; i >= 0; --i) {
      if (text[i] == ';') {
        text[i] = 0;
        semicolonPos = &text[i];
        hasRemovedSemicolon = 1;
        break;
      }
    }

    if (strcmp(text, "exit") == 0)
      exit(0);
    if (strncmp(text, "run ", 4) == 0) {
      if (loadText(&text[4], text, 10000) == 0)
        run(text);
      continue;
    }
    if (hasRemovedSemicolon)
      *semicolonPos = ';';
    run(text);
  }
}
