/*
  Copyright (c) 2023 Masahiro Oono

  This software is a translator for HL.
  HL (Haribote Language) is a programming language
  designed in 2021 by Hidemi Kawai.
  https://essen.osask.jp/?a21_txt01
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef unsigned char *String;

void loadText(int argc, const char **argv, String text, int size)
{
  if (argc < 2) {
    printf("Usage: %s program-file\n", argv[0]);
    exit(1);
  }

  FILE *fp = fopen(argv[1], "rt");
  if (fp == NULL) {
    printf("Failed to open %s\n", argv[1]);
    exit(1);
  }

  int nItems = fread(text, 1, size - 1, fp);
  fclose(fp);
  text[nItems] = 0;
}

#define MAX_TOKEN_CODE 1000 // トークンコードの最大値
String tokenStrs[MAX_TOKEN_CODE + 1];
int    tokenLens[MAX_TOKEN_CODE + 1];

int vars[MAX_TOKEN_CODE + 1];

int getTokenCode(String str, int len)
{
  static unsigned char tokenBuf[(MAX_TOKEN_CODE + 1) * 10];
  static int nTokens = 0, unusedHead = 0; // 登録済みのトークンの数, 未使用領域へのポインタ

  int i;
  for (i = 0; i < nTokens; ++i) { // 登録済みのトークンコードの中から探す
    if (len == tokenLens[i] && strncmp(str, tokenStrs[i], len) == 0)
      break;
  }
  if (i == nTokens) {
    if (nTokens >= MAX_TOKEN_CODE) {
      printf("Too many tokens\n");
      exit(1);
    }
    strncpy(&tokenBuf[ unusedHead ], str, len); // 見つからなければ新規登録
    tokenBuf[ unusedHead + len ] = 0;
    tokenStrs[i] = &tokenBuf[ unusedHead ];
    tokenLens[i] = len;
    unusedHead += len + 1;
    ++nTokens;

    vars[i] = strtol(tokenStrs[i], NULL, 0); // 定数であれば初期値を設定（定数でなければ0になる）
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

int lexer(String str, int *tc)
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
      printf("Lexing error: %.10s\n", &str[pos]);
      exit(1);
    }
    tc[nTokens] = getTokenCode(&str[pos], len);
    pos += len;
    ++nTokens;
  }
}

int tc[10000]; // トークンコードを格納する

int main(int argc, const char **argv)
{
  unsigned char text[10000];
  loadText(argc, argv, text, 10000);

  int plus      = getTokenCode("+", 1);
  int minus     = getTokenCode("-", 1);
  int period    = getTokenCode(".", 1);
  int semicolon = getTokenCode(";", 1);
  int assign    = getTokenCode("=", 1);
  int print     = getTokenCode("print", 5);

  int nTokens = lexer(text, tc);
  tc[nTokens] = tc[nTokens + 1] = tc[nTokens + 2] = tc[nTokens + 3] = period; // エラー表示用

  int pc;
  for (pc = 0; pc < nTokens; ++pc) {
    if (tc[pc + 1] == assign && tc[pc + 3] == semicolon)
      vars[tc[pc]] = vars[tc[pc + 2]];
    else if (tc[pc + 1] == assign && tc[pc + 3] == plus && tc[pc + 5] == semicolon)
      vars[tc[pc]] = vars[tc[pc + 2]] + vars[tc[pc + 4]];
    else if (tc[pc + 1] == assign && tc[pc + 3] == minus && tc[pc + 5] == semicolon)
      vars[tc[pc]] = vars[tc[pc + 2]] - vars[tc[pc + 4]];
    else if (tc[pc] == print && tc[pc + 2] == semicolon)
      printf("%d\n", vars[tc[pc + 1]]);
    else
      goto err;

    while (tc[pc] != semicolon)
      ++pc;
  }
  exit(0);
err:
  printf("Syntax error: %s %s %s %s\n", tokenStrs[tc[pc]], tokenStrs[tc[pc + 1]], tokenStrs[tc[pc + 2]], tokenStrs[tc[pc + 3]]);
  exit(1);
}
