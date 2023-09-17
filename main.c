/*
  Copyright (c) 2023 Masahiro Oono

  This software is a translator for HL.
  HL (Haribote Language) is a programming language
  designed in 2021 by Hidemi Kawai.
  https://essen.osask.jp/?a21_txt01
*/
#include <stdio.h>
#include <stdlib.h>

unsigned char text[] =
/* BEGIN */
"a=1;"
"b=2;"
"c=a+b;"
"print c;"
/* END */
;

int main(int argc, const char **argv)
{
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
