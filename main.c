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
#include <time.h>
#include <assert.h>
#if defined(__APPLE__) || defined(__linux__)
#include <unistd.h>
#include <termios.h>
#endif

typedef unsigned char *String;

int loadText(String path, String text, int size)
{
  unsigned char buf[1000];

  int startPos = path[0] == '"'; // ダブルクォートがあれば外す
  int i = 0;
  while (path[startPos + i] != 0 && path[startPos + i] != '"') {
    buf[i] = path[startPos + i];
    ++i;
  }
  buf[i] = 0;

  FILE *fp = fopen(buf, "rt");
  if (fp == NULL) {
    printf("Failed to open %s\n", path);
    return 1;
  }

  int nItems = fread(text, 1, size - 1, fp);
  fclose(fp);
  text[nItems] = 0;
  return 0;
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

int run(String src)
{
  clock_t begin = clock();

  int equal     = getTokenCode("==", 2);
  int notEq     = getTokenCode("!=", 2);
  int lesEq     = getTokenCode("<=", 2);
  int gtrEq     = getTokenCode(">=", 2);
  int les       = getTokenCode("<", 1);
  int gtr       = getTokenCode(">", 1);
  int lparen    = getTokenCode("(", 1);
  int rparen    = getTokenCode(")", 1);
  int plus      = getTokenCode("+", 1);
  int minus     = getTokenCode("-", 1);
  int period    = getTokenCode(".", 1);
  int semicolon = getTokenCode(";", 1);
  int colon     = getTokenCode(":", 1);
  int assign    = getTokenCode("=", 1);
  int print     = getTokenCode("print", 5);
  int time      = getTokenCode("time", 4);
  int _goto     = getTokenCode("goto", 4);
  int _if       = getTokenCode("if", 2);

  int nTokens = lexer(src, tc);
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
      int lhs = vars[tc[pc + 2]], op = tc[pc + 3], rhs = vars[tc[pc + 4]];
      int dest = vars[tc[pc + 7]];
      if (op == equal && lhs == rhs) { pc = dest; continue; }
      if (op == notEq && lhs != rhs) { pc = dest; continue; }
      if (op == lesEq && lhs <= rhs) { pc = dest; continue; }
      if (op == gtrEq && lhs >= rhs) { pc = dest; continue; }
      if (op == les   && lhs <  rhs) { pc = dest; continue; }
      if (op == gtr   && lhs >  rhs) { pc = dest; continue; }
    }
    else if (tc[pc] == time && tc[pc + 1] == semicolon)
      printf("time: %.3f[sec]\n", (clock() - begin) / (double) CLOCKS_PER_SEC);
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
  printf("Syntax error: %s %s %s %s\n", tokenStrs[tc[pc]], tokenStrs[tc[pc + 1]], tokenStrs[tc[pc + 2]], tokenStrs[tc[pc + 3]]);
  return 1;
}

String removeTrailingSemicolon(String str, size_t len) {
  for (int i = len - 1; i >= 0; --i) {
    if (str[i] != ';')
      continue;
    str[i] = 0;
    return &str[i];
  }
  return NULL;
}

#if defined(__APPLE__) || defined(__linux__)
struct termios initial_term;

void loadHistory();

void initTerm()
{
  tcgetattr(0, &initial_term);
  loadHistory();
}

void saveHistory();

void destroyTerm()
{
  saveHistory();
}

inline static void setNonCanonicalMode()
{
  struct termios new_term;
  new_term = initial_term;
  new_term.c_oflag |= ONOCR;
  new_term.c_lflag &= ~(ICANON | ECHO);
  new_term.c_cc[VMIN] = 1;
  new_term.c_cc[VTIME] = 0;
  tcsetattr(0, TCSANOW, &new_term);
}

inline static void setCanonicalMode()
{
  tcsetattr(0, TCSANOW, &initial_term);
}

int cursorX;

inline static void eraseLine()
{
  printf("\e[2K\r");
  cursorX = 0;
}

inline static void eraseAll()
{
  printf("\e[2J\e[H");
  cursorX = 0;
}
#else
void initTerm() {}
void destroyTerm() {}
#endif

#define LINE_SIZE 100
#if defined(__APPLE__) || defined(__linux__)
#define HISTORY_SIZE 100

typedef struct { char str[LINE_SIZE]; int len; } Command;

typedef struct {
  Command buf[HISTORY_SIZE];
  int head, tail, count, pos;
} CmdHistory;

CmdHistory cmdHist;

void setHistory(char *cmd, int len)
{
  if (len == 0)
    return;

  strncpy((char *) &cmdHist.buf[cmdHist.tail].str, cmd, len);
  cmdHist.buf[cmdHist.tail].str[len] = 0;
  cmdHist.buf[cmdHist.tail].len = len;
  if (cmdHist.count < HISTORY_SIZE)
    ++cmdHist.count;
  else
    cmdHist.head = (cmdHist.head + 1) % HISTORY_SIZE;
  cmdHist.tail = (cmdHist.tail + 1) % HISTORY_SIZE;
}

enum { Prev, Next };

void showHistory(int dir, char *buf)
{
  if (cmdHist.count == 0)
    return;

  if (dir == Prev && cmdHist.pos < cmdHist.count)
    ++cmdHist.pos;
  else if (dir == Prev && cmdHist.pos >= cmdHist.count)
    cmdHist.pos = cmdHist.count;
  else if (dir == Next && cmdHist.pos > 1)
    --cmdHist.pos;
  else if (dir == Next && cmdHist.pos <= 1) {
    cmdHist.pos = 0;
    return;
  }

  int i = (cmdHist.head + cmdHist.count - cmdHist.pos) % cmdHist.count;
  Command *cmd = &cmdHist.buf[i];
  printf("%s", cmd->str);
  strncpy(buf, cmd->str, cmd->len + 1);
  cursorX = cmd->len;
}

void saveHistory()
{
  FILE *fp;
  if (cmdHist.count == 0 || (fp = fopen(".haribote_history", "wt")) == NULL)
    return;

  int counter = cmdHist.count;
  while (counter > 0) {
    int i = (cmdHist.head + cmdHist.count - counter) % cmdHist.count;
    fprintf(fp, "%s\n", cmdHist.buf[i].str);
    --counter;
  }

  fclose(fp);
}

void loadHistory()
{
  FILE *fp;
  if ((fp = fopen(".haribote_history", "rt")) == NULL)
    return;

  for (int i = 0; i < HISTORY_SIZE; ++i) {
    char line[LINE_SIZE];
    if (fgets(line, LINE_SIZE, fp) == NULL)
      break;
    setHistory(line, strlen(line) - 1);
  }

  fclose(fp);
}

char *readLine(char *str, int size, FILE *stream)
{
  assert(size > 0);
  setNonCanonicalMode();

  int i = cursorX ? cursorX : 0, end = size - 1, ch, nDeleted = 0;
  while ((i < end) && ((ch = fgetc(stream)) != EOF)) {
    if (ch == 8 || ch == 127) { // Backspace or Delete
      if (cursorX == 0)
        continue;

      write(0, "\e[D\e[K", 6);
      if (cursorX < i) {
        str[i] = 0;
        printf("\e7%s\e8", &str[cursorX + nDeleted]);
      }
      --cursorX;
      ++nDeleted;
      continue;
    }
    else if (nDeleted > 0) {
      memmove(&str[cursorX], &str[cursorX + nDeleted], i - cursorX - nDeleted);
      i -= nDeleted;
      str[i] = nDeleted = 0;
    }

    if (ch == '\n') {
      putchar(ch);
      str[i] = ch; str[i + 1] = 0;
      setHistory(str, i);
      cmdHist.pos = 0;
      cursorX = 0;
      setCanonicalMode();
      return str;
    }
    else if (ch == 27) { // escape sequence
      if ((ch = fgetc(stream)) == EOF)
        break;
      else if (ch != 91) {
        ungetc(ch, stream);
        continue;
      }
      if ((ch = fgetc(stream)) == EOF)
        break;
      switch (ch) {
      case 67: if (cursorX < i) { write(0, "\e[C", 3); ++cursorX; } continue; // RightArrow
      case 68: if (cursorX > 0) { write(0, "\e[D", 3); --cursorX; } continue; // LeftArrow
      case 65: strncpy(str, "prevhist", 9); break; // UpArrow
      case 66: strncpy(str, "nexthist", 9); break; // DownArrow
      }
      setCanonicalMode();
      return str;
    }
    else if (ch == 6) { // Control-F
      if (cursorX < i) {
        write(0, "\e[C", 3);
        ++cursorX;
      }
    }
    else if (ch == 2) { // Control-B
      if (cursorX > 0) {
        write(0, "\e[D", 3);
        --cursorX;
      }
    }
    else if (ch == 1) { // Control-A
      if (cursorX <= 0)
        continue;
      printf("\e[%dD", cursorX);
      cursorX = 0;
    }
    else if (ch == 5) { // Control-E
      if (cursorX >= i)
        continue;
      printf("\e[%dC", i - cursorX);
      cursorX = i;
    }
    else if (ch == 11) { // Control-K
      write(0, "\e[K", 3);
      str[cursorX] = 0;
      i -= i - cursorX;
    }
    else if (ch == 12) { // Control-L
      strncpy(str, "clear", 6);
      setCanonicalMode();
      return str;
    }
    else if (ch < 32) {
      ;
    }
    else {
      putchar(ch);
      if (cursorX < i) {
        str[i] = 0;
        printf("\e7%s\e8", &str[cursorX]);
        memmove(&str[cursorX + 1], &str[cursorX], i - cursorX);
      }
      str[cursorX] = ch;
      ++cursorX;
      ++i;
    }
  }
  str[i] = 0;
  cursorX = 0;
  setCanonicalMode();
  return NULL;
}
#else
#define readLine fgets
#endif

int main(int argc, const char **argv)
{
  unsigned char text[10000];
  if (argc >= 2) {
    if (loadText((String) argv[1], text, 10000) != 0)
      exit(1);
    run(text);
    exit(0);
  }

  initTerm();
  for (int next = 1, nLines = 0;;) {
    if (next)
      printf("[%d]> ", ++nLines);
    readLine(text, LINE_SIZE, stdin);
    int inputLen = strlen(text);
    if (text[inputLen - 1] == '\n')
      text[inputLen - 1] = 0;

    next = 1;
    String semicolonPos = removeTrailingSemicolon(text, inputLen - 1);
    if (strcmp(text, "exit") == 0) {
      destroyTerm();
      exit(0);
    }
#if defined(__APPLE__) || defined(__linux__)
    else if (strcmp(text, "prevhist") == 0) {
      eraseLine();
      printf("[%d]> ", nLines);
      showHistory(Prev, text);
      next = 0;
      continue;
    }
    else if (strcmp(text, "nexthist") == 0) {
      eraseLine();
      printf("[%d]> ", nLines);
      showHistory(Next, text);
      next = 0;
      continue;
    }
#endif
    else if (strncmp(text, "run ", 4) == 0) {
      if (loadText(&text[4], text, 10000) != 0)
        continue;
      run(text);
    }
#if defined(__APPLE__) || defined(__linux__)
    else if (strcmp(text, "clear") == 0) {
      eraseAll();
      printf("[%d]> ", nLines);
      next = 0;
      continue;
    }
#endif
    else {
      if (semicolonPos)
        *semicolonPos = ';';
      run(text);
    }
  }
}
