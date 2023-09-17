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

  int vars[256];
  for (int i = 0; i < 10; ++i)
    vars['0' + i] = i;

  int pc;
  for (pc = 0; text[pc] != 0; ++pc) {
    if (text[pc] == '\n' || text[pc] == '\r' || text[pc] == ' ' || text[pc] == '\t' || text[pc] == ';')
      continue;

    if (text[pc + 1] == '=' && text[pc + 3] == ';')
      vars[text[pc]] = vars[text[pc + 2]];
    else if (text[pc + 1] == '=' && text[pc + 3] == '+' && text[pc + 5] == ';')
      vars[text[pc]] = vars[text[pc + 2]] + vars[text[pc + 4]];
    else if (text[pc + 1] == '=' && text[pc + 3] == '-' && text[pc + 5] == ';')
      vars[text[pc]] = vars[text[pc + 2]] - vars[text[pc + 4]];
    else if (text[pc] == 'p' && text[pc + 1] == 'r' && text[pc + 5] == ' ' && text[pc + 7] == ';')
      printf("%d\n", vars[text[pc + 6]]);
    else
      goto err;

    while (text[pc] != ';')
      ++pc;
  }
  exit(0);
err:
  printf("Syntax error: %.10s\n", &text[pc]);
  exit(1);
}
