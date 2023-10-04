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

int tc[10000]; // トークンコード列を格納する

enum {
  PlusPlus,
  Equal,
  NotEq,
  Les,
  GtrEq,
  LesEq,
  Gtr,
  Plus,
  Minus,
  Multi,
  Divi,
  Mod,
  And,
  ShiftRight,
  Assign,

  Lparen,
  Rparen,
  Lbracket,
  Rbracket,
  Lbrace,
  Rbrace,
  Period,
  Comma,
  Semicolon,
  Colon,

  Print,
  Time,
  Goto,
  If,
  Else,

  Wildcard,
  Expr,
  Expr0,

  Zero,
  One,
  Two,
  Three,
  Four,
  Five,
  Six,
  Seven,
  Eight,
  Nine,

  Tmp0,
  Tmp1,
  Tmp2,
  Tmp3,
  Tmp4,
  Tmp5,
  Tmp6,
  Tmp7,
  Tmp8,
  Tmp9,

  EndOfKeys
};

String defaultTokens[] = {
  "++",
  "==",
  "!=",
  "<",
  ">=",
  "<=",
  ">",
  "+",
  "-",
  "*",
  "/",
  "%",
  "&",
  ">>",
  "=",

  "(",
  ")",
  "[",
  "]",
  "{",
  "}",
  ".",
  ",",
  ";",
  ":",

  "print",
  "time",
  "goto",
  "if",
  "else",

  "!!*",
  "!!**",
  "!!***",

  "0",
  "1",
  "2",
  "3",
  "4",
  "5",
  "6",
  "7",
  "8",
  "9",

  "_t0",
  "_t1",
  "_t2",
  "_t3",
  "_t4",
  "_t5",
  "_t6",
  "_t7",
  "_t8",
  "_t9",
};

void initTc(String *defaultTokens, int len)
{
  assert(len == EndOfKeys);
  for (int i = 0; i < len; ++i)
    tc[i] = getTokenCode(defaultTokens[i], strlen(defaultTokens[i]));
}

typedef enum {
  Prefix_PlusPlus = 2,
  Prefix_Minus = 2,
  Infix_Multi = 4,
  Infix_Divi = 4,
  Infix_Mod = 4,
  Infix_Plus = 5,
  Infix_Minus = 5,
  Infix_ShiftRight = 6,
  Infix_LesEq = 7,
  Infix_GtrEq = 7,
  Infix_Les = 7,
  Infix_Gtr = 7,
  Infix_Equal = 8,
  Infix_NotEq = 8,
  Infix_And = 9,
  Infix_Assign = 15,
  LowestPrecedence = 99,
  NoPrecedence = 100
} Precedence;

Precedence precedenceTable[][2] = {
  [PlusPlus]   = {NoPrecedence,     Prefix_PlusPlus},
  [Equal]      = {Infix_Equal,      NoPrecedence},
  [NotEq]      = {Infix_NotEq,      NoPrecedence},
  [LesEq]      = {Infix_LesEq,      NoPrecedence},
  [GtrEq]      = {Infix_GtrEq,      NoPrecedence},
  [Les]        = {Infix_Les,        NoPrecedence},
  [Gtr]        = {Infix_Gtr,        NoPrecedence},
  [Plus]       = {Infix_Plus,       NoPrecedence},
  [Minus]      = {Infix_Minus,      Prefix_Minus},
  [Multi]      = {Infix_Multi,      NoPrecedence},
  [Divi]       = {Infix_Divi,       NoPrecedence},
  [Mod]        = {Infix_Mod,        NoPrecedence},
  [And]        = {Infix_And,        NoPrecedence},
  [ShiftRight] = {Infix_ShiftRight, NoPrecedence},
  [Assign]     = {Infix_Assign,     NoPrecedence},
};

enum { Infix, Prefix, EndOfStyles };

inline static Precedence getPrecedence(int style, int operator)
{
  assert(0 <= style && style < EndOfStyles);
  return 0 <= operator && operator < Lparen
    ? precedenceTable[operator][style]
    : NoPrecedence;
}

#define MAX_PHRASE_LEN 31
#define N_WILDCARDS 10
int phraseTc[(MAX_PHRASE_LEN + 1) * 100]; // フレーズを字句解析して得たトークンコード列を格納する
int wpc[N_WILDCARDS * 2]; // ワイルドカードにマッチしたトークンを指す
int nextPc; // マッチしたフレーズの末尾の次のトークンを指す

inline static int _end(int num)
{
  return N_WILDCARDS + num;
}

int match(int id, String phrase, int pc)
{
  int head = id * (MAX_PHRASE_LEN + 1), phraseLen;

  if (phraseTc[head + MAX_PHRASE_LEN] == 0) {
    phraseLen = lexer(phrase, &phraseTc[head]);
    assert(phraseLen <= MAX_PHRASE_LEN);
    phraseTc[head + MAX_PHRASE_LEN] = phraseLen;
  }

  phraseLen = phraseTc[head + MAX_PHRASE_LEN];
  for (int pos = 0; pos < phraseLen; ++pos) {
    int phraTc = phraseTc[head + pos];
    if (phraTc == Wildcard || phraTc == Expr || phraTc == Expr0) {
      ++pos;
      int num = phraseTc[head + pos] - Zero;
      wpc[num] = pc; // トークンの位置（式の場合は式の開始位置）
      if (phraTc == Wildcard) {
        ++pc;
        continue;
      }
      int depth = 0; // 括弧の深さ
      for (;;) {
        if (tc[pc] == Semicolon)
          break;
        if (tc[pc] == Comma && depth == 0)
          break;

        if (tc[pc] == Lparen || tc[pc] == Lbracket)
          ++depth;
        if (tc[pc] == Rparen || tc[pc] == Rbracket)
          --depth;
        if (depth < 0)
          break;
        ++pc;
      }
      wpc[_end(num)] = pc; // 式の終了位置
      if (phraTc == Expr && wpc[num] == pc)
        return 0;
      if (depth > 0)
        return 0;
      continue;
    }
    if (phraTc != tc[pc])
      return 0;
    ++pc;
  }
  nextPc = pc;
  return 1;
}

typedef int *IntPtr;

IntPtr internalCodes[10000]; // ソースコードをコンパイルして生成した内部コードを格納する
IntPtr *icp;

typedef enum {
  OpEnd,
  OpCpy,
  OpCeq,
  OpCne,
  OpClt,
  OpCge,
  OpCle,
  OpCgt,
  OpAdd,
  OpSub,
  OpMul,
  OpDiv,
  OpMod,
  OpBand,
  OpShr,
  OpAdd1,
  OpNeg,
  OpGoto,
  OpJeq,
  OpJne,
  OpJlt,
  OpJge,
  OpJle,
  OpJgt,
  OpLop,
  OpPrint,
  OpTime,
} Opcode;

void putIc(Opcode op, IntPtr p1, IntPtr p2, IntPtr p3, IntPtr p4)
{
  icp[0] = (IntPtr) op;
  icp[1] = p1;
  icp[2] = p2;
  icp[3] = p3;
  icp[4] = p4;
  icp += 5;
}

#define N_TMPS 10
char tmpFlags[N_TMPS];

int tmpAlloc()
{
  for (int i = 0; i < N_TMPS; ++i) {
    if (!tmpFlags[i]) {
      tmpFlags[i] = 1;
      return Tmp0 + i;
    }
  }
  printf("Register allocation failed\n");
  return -1;
}

void tmpFree(int i)
{
  if (Tmp0 <= i && i <= Tmp9)
    tmpFlags[i - Tmp0] = 0;
}

int epc, epcEnd; // expression()のためのpc, その式の直後のトークンを指す

int evalExpression(Precedence precedence);
int expression(int num);

inline static Opcode getOpcode(int i) // for infix operators
{
  assert(Equal <= i && i < Assign);
  return OpCeq + i - Equal;
}

int evalInfixExpression(int lhs, Precedence precedence, int op)
{
  ++epc;
  int rhs = evalExpression(precedence);
  int res = tmpAlloc();
  putIc(getOpcode(op), &vars[res], &vars[lhs], &vars[rhs], 0);
  tmpFree(lhs);
  tmpFree(rhs);
  if (lhs < 0 || rhs < 0)
    return -1;
  return res;
}

int evalExpression(Precedence precedence)
{
  int res = -1, e0 = 0;
  nextPc = 0;

  if (match(99, "( !!**0 )", epc)) { // 括弧
    res = expression(0);
  }
  else if (tc[epc] == PlusPlus) { // 前置インクリメント
    ++epc;
    res = evalExpression(Prefix_PlusPlus);
    putIc(OpAdd1, &vars[res], 0, 0, 0);
  }
  else if (tc[epc] == Minus) { // 単項マイナス
    ++epc;
    e0 = evalExpression(Prefix_Minus);
    res = tmpAlloc();
    putIc(OpNeg, &vars[res], &vars[e0], 0, 0);
  }
  else { // 変数もしくは定数
    res = tc[epc];
    ++epc;
  }
  if (nextPc > 0)
    epc = nextPc;
  for (;;) {
    tmpFree(e0);
    if (res < 0 || e0 < 0) // ここまででエラーがあれば、処理を打ち切り
      return -1;
    if (epc >= epcEnd)
      break;

    Precedence encountered; // ぶつかった演算子の優先順位を格納する
    e0 = 0;
    if (tc[epc] == PlusPlus) { // 後置インクリメント
      ++epc;
      e0 = res;
      res = tmpAlloc();
      putIc(OpCpy, &vars[res], &vars[e0], 0, 0);
      putIc(OpAdd1, &vars[e0], 0, 0, 0);
    }
    else if (precedence >= (encountered = getPrecedence(Infix, tc[epc]))) {
      /*
        「引数として渡された優先順位」が「ぶつかった演算子の優先順位」よりも
        低いか又は等しい(値が大きいか又は等しい)ときは、このブロックを実行して
        中置演算子を評価する。

        「引数として渡された優先順位」が「ぶつかった演算子の優先順位」よりも
        高い(値が小さい)ときは、このブロックを実行せずにこれまでに式を評価した
        結果を呼び出し元に返す。
      */
      switch (tc[epc]) {
      case Multi: case Divi: case Mod:
      case Plus: case Minus:
      case ShiftRight:
      case Les: case LesEq: case Gtr: case GtrEq:
      case Equal: case NotEq:
      case And:
        res = evalInfixExpression(res, encountered - 1, tc[epc]);
        break;
      case Assign:
        ++epc;
        e0 = evalExpression(encountered);
        putIc(OpCpy, &vars[res], &vars[e0], 0, 0);
        break;
      }
    }
    else
      break;
  }
  return res;
}

// 引数として渡したワイルドカード番号にマッチした式をコンパイルしてinternalCodes[]に書き込む
int expression(int num)
{
  if (wpc[num] == wpc[_end(num)])
    return 0;

  int i, end = N_WILDCARDS * 2, buf[end + 1];
  for (i = 0; i < end; ++i)
    buf[i] = wpc[i];
  buf[i] = nextPc;
  int oldEpc = epc, oldEpcEnd = epcEnd;

  epc = wpc[num]; epcEnd = wpc[_end(num)];
  int res = evalExpression(LowestPrecedence);
  if (epc < epcEnd)
    return -1;

  for (i = 0; i < end; ++i)
    wpc[i] = buf[i];
  nextPc = buf[i];
  epc = oldEpc; epcEnd = oldEpcEnd;
  return res;
}

enum { ConditionIsTrue, ConditionIsFalse };

// 条件式wpc[i]を評価して、その結果に応じてlabelに分岐する内部コードを生成する
void ifgoto(int i, int not, int label)
{
  int begin = wpc[i];

  if (begin + 3 == wpc[_end(i)] && Equal <= tc[begin + 1] && tc[begin + 1] <= Gtr) {
    Opcode op = OpJeq + ((tc[begin + 1] - Equal) ^ not);
    putIc(op, &vars[label], &vars[tc[begin]], &vars[tc[begin + 2]], 0);
  }
  else {
    i = expression(i);
    putIc(OpJne - not, &vars[label], &vars[i], &vars[Zero], 0);
    tmpFree(i);
  }
}

int tmpLabelNo;

int tmpLabelAlloc()
{
  char str[10];
  sprintf(str, "_l%d", tmpLabelNo);
  ++tmpLabelNo;
  return getTokenCode(str, strlen(str));
}

#define BLOCK_INFO_SIZE 10
int blockInfo[BLOCK_INFO_SIZE * 100], blockDepth;

enum { BlockType, IfBlock };
enum { IfLabel0 = 1, IfLabel1 };

inline static int *initBlockInfo()
{
  blockDepth = 0;
  return blockInfo;
}

inline static int *beginBlock()
{
  blockDepth += BLOCK_INFO_SIZE;
  return &blockInfo[blockDepth];
}

inline static int *endBlock()
{
  blockDepth -= BLOCK_INFO_SIZE;
  return &blockInfo[blockDepth];
}

int compile(String src)
{
  int nTokens = lexer(src, tc);
  tc[nTokens++] = Semicolon; // 末尾に「;」を付け忘れることが多いので、付けてあげる
  tc[nTokens] = tc[nTokens + 1] = tc[nTokens + 2] = tc[nTokens + 3] = Period; // エラー表示用

  icp = internalCodes;

  for (int i = 0; i < N_TMPS; ++i)
    tmpFlags[i] = 0;
  tmpLabelNo = 0;
  int *curBlock = initBlockInfo();

  int pc;
  for (pc = 0; pc < nTokens;) {
    int e0 = 0;
    if (match(0, "!!*0 = !!*1;", pc)) {
      putIc(OpCpy, &vars[tc[wpc[0]]], &vars[tc[wpc[1]]], 0, 0);
    }
    else if (match(10, "!!*0 = !!*1 + 1; if (!!*2 < !!*3) goto !!*4;", pc) && tc[wpc[0]] == tc[wpc[1]] && tc[wpc[0]] == tc[wpc[2]]) {
      putIc(OpLop, &vars[tc[wpc[4]]], &vars[tc[wpc[0]]], &vars[tc[wpc[3]]], 0);
    }
    else if (match(9, "!!*0 = !!*1 + 1;", pc) && tc[wpc[0]] == tc[wpc[1]]) { // +1専用の命令
      putIc(OpAdd1, &vars[tc[wpc[0]]], 0, 0, 0);
    }
    else if (match(1, "!!*0 = !!*1 + !!*2;", pc)) {
      putIc(OpAdd, &vars[tc[wpc[0]]], &vars[tc[wpc[1]]], &vars[tc[wpc[2]]], 0);
    }
    else if (match(2, "!!*0 = !!*1 - !!*2;", pc)) {
      putIc(OpSub, &vars[tc[wpc[0]]], &vars[tc[wpc[1]]], &vars[tc[wpc[2]]], 0);
    }
    else if (match(3, "print !!**0;", pc)) {
      e0 = expression(0);
      putIc(OpPrint, &vars[e0], 0, 0, 0);
    }
    else if (match(4, "!!*0:", pc)) { // ラベル定義命令
      vars[tc[wpc[0]]] = icp - internalCodes; // ラベル名の変数にその時のicpの相対位置を入れておく
    }
    else if (match(5, "goto !!*0;", pc)) {
      putIc(OpGoto, &vars[tc[wpc[0]]], 0, 0, 0);
    }
    else if (match(6, "if (!!**0) goto !!*1;", pc)) {
      ifgoto(0, ConditionIsTrue, tc[wpc[1]]);
    }
    else if (match(7, "time;", pc)) {
      putIc(OpTime, 0, 0, 0, 0);
    }
    else if (match(11, "if (!!**0) {", pc)) { // if文
      curBlock = beginBlock();
      curBlock[ BlockType ] = IfBlock;
      curBlock[ IfLabel0  ] = tmpLabelAlloc(); // 条件不成立のときの飛び先
      curBlock[ IfLabel1  ] = 0;
      ifgoto(0, ConditionIsFalse, curBlock[IfLabel0]);
    }
    else if (match(13, "} else {", pc) && curBlock[BlockType] == IfBlock) {
      curBlock[IfLabel1] = tmpLabelAlloc(); // else節の終端
      putIc(OpGoto, &vars[curBlock[IfLabel1]], 0, 0, 0);
      vars[curBlock[IfLabel0]] = icp - internalCodes;
    }
    else if (match(12, "}", pc) && curBlock[BlockType] == IfBlock) {
      int ifLabel = curBlock[IfLabel1] ? IfLabel1 : IfLabel0;
      vars[curBlock[ifLabel]] = icp - internalCodes;
      curBlock = endBlock();
    }
    else if (match(8, "!!***0;", pc)) {
      e0 = expression(0);
    }
    else {
      goto err;
    }
    tmpFree(e0);
    if (e0 < 0)
      goto err;
    pc = nextPc;
  }
  if (blockDepth > 0) {
    printf("Block nesting error: blockDepth=%d", blockDepth);
    return -1;
  }
  putIc(OpEnd, 0, 0, 0, 0);

  IntPtr *end = icp;
  Opcode op;
  for (icp = internalCodes; icp < end; icp += 5) { // goto先の設定
    op = (Opcode) icp[0];
    if (OpGoto <= op && op <= OpLop)
      icp[1] = (IntPtr) (internalCodes + *icp[1]);
  }
  return end - internalCodes;
err:
  printf("Syntax error: %s %s %s %s\n", tokenStrs[tc[pc]], tokenStrs[tc[pc + 1]], tokenStrs[tc[pc + 2]], tokenStrs[tc[pc + 3]]);
  return -1;
}

void exec()
{
  clock_t begin = clock();
  icp = internalCodes;
  int i;
  for (;;) {
    switch ((Opcode) icp[0]) {
    case OpEnd:
      return;
    case OpNeg:   *icp[1] = -*icp[2];           icp += 5; continue;
    case OpAdd1:  ++(*icp[1]);                  icp += 5; continue;
    case OpMul:   *icp[1] = *icp[2] *  *icp[3]; icp += 5; continue;
    case OpDiv:   *icp[1] = *icp[2] /  *icp[3]; icp += 5; continue;
    case OpMod:   *icp[1] = *icp[2] %  *icp[3]; icp += 5; continue;
    case OpAdd:   *icp[1] = *icp[2] +  *icp[3]; icp += 5; continue;
    case OpSub:   *icp[1] = *icp[2] -  *icp[3]; icp += 5; continue;
    case OpShr:   *icp[1] = *icp[2] >> *icp[3]; icp += 5; continue;
    case OpClt:   *icp[1] = *icp[2] <  *icp[3]; icp += 5; continue;
    case OpCle:   *icp[1] = *icp[2] <= *icp[3]; icp += 5; continue;
    case OpCgt:   *icp[1] = *icp[2] >  *icp[3]; icp += 5; continue;
    case OpCge:   *icp[1] = *icp[2] >= *icp[3]; icp += 5; continue;
    case OpCeq:   *icp[1] = *icp[2] == *icp[3]; icp += 5; continue;
    case OpCne:   *icp[1] = *icp[2] != *icp[3]; icp += 5; continue;
    case OpBand:  *icp[1] = *icp[2] &  *icp[3]; icp += 5; continue;
    case OpCpy:   *icp[1] = *icp[2];            icp += 5; continue;
    case OpPrint:
      printf("%d\n", *icp[1]);
      icp += 5;
      continue;
    case OpGoto:                           icp = (IntPtr *) icp[1]; continue;
    case OpJeq:  if (*icp[2] == *icp[3]) { icp = (IntPtr *) icp[1]; continue; } icp += 5; continue;
    case OpJne:  if (*icp[2] != *icp[3]) { icp = (IntPtr *) icp[1]; continue; } icp += 5; continue;
    case OpJle:  if (*icp[2] <= *icp[3]) { icp = (IntPtr *) icp[1]; continue; } icp += 5; continue;
    case OpJge:  if (*icp[2] >= *icp[3]) { icp = (IntPtr *) icp[1]; continue; } icp += 5; continue;
    case OpJlt:  if (*icp[2] <  *icp[3]) { icp = (IntPtr *) icp[1]; continue; } icp += 5; continue;
    case OpJgt:  if (*icp[2] >  *icp[3]) { icp = (IntPtr *) icp[1]; continue; } icp += 5; continue;
    case OpTime:
      printf("time: %.3f[sec]\n", (clock() - begin) / (double) CLOCKS_PER_SEC);
      icp += 5;
      continue;
    case OpLop:
      i = *icp[2];
      ++i;
      *icp[2] = i;
      if (i < *icp[3]) {
        icp = (IntPtr *) icp[1];
        continue;
      }
      icp += 5;
      continue;
    }
  }
}

int run(String src)
{
  if (compile(src) < 0)
    return 1;
  exec();
  return 0;
}

String removeTrailingSemicolon(String str, size_t len)
{
  String rv = NULL;
  for (int i = len - 1; i >= 0; --i) {
    if (str[i] != ';')
      break;
    str[i] = 0;
    rv = &str[i];
  }
  return rv;
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

  int i = cursorX ? cursorX : 0, end = size - 1, ch, nDeleted = 0, nInserted = 0;
  char insertBuf[LINE_SIZE + 1] = "";

  while ((i < end) && ((ch = fgetc(stream)) != EOF)) {
    if (ch == 4) // Control-D
      break;

    if (ch >= 32 && ch != 127) { // printable characters
      while (ch >= 32 && ch != 127) {
        putchar(ch);
        if (cursorX < i) {
          if (nInserted == 0) {
            insertBuf[LINE_SIZE] = i - cursorX;
            str[i] = 0;
          }
          printf("\e7%s\e8", &str[cursorX - nInserted]);
          insertBuf[nInserted] = ch;
          ++nInserted;
        }
        else
          str[cursorX] = ch;
        ++cursorX;
        ++i;

        if ((i >= end) || ((ch = fgetc(stream)) == EOF))
          break;
      }
      if (i >= end || ch == EOF)
        break;

      if (nInserted > 0) {
        memmove(&str[cursorX], &str[cursorX - nInserted], insertBuf[LINE_SIZE]);
        strncpy(&str[cursorX - nInserted], insertBuf, nInserted);
        insertBuf[0] = nInserted = 0;
      }
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
    else if (ch == 8 || ch == 127) { // Backspace or Delete
      while (ch == 8 || ch == 127) {
        if (cursorX == 0)
          break;

        write(1, "\e[D\e[K", 6);
        if (cursorX < i) {
          str[i] = 0;
          printf("\e7%s\e8", &str[cursorX + nDeleted]);
        }
        --cursorX;
        ++nDeleted;

        if ((ch = fgetc(stream)) == EOF)
          break;
      }
      if (ch == EOF)
        break;

      if (nDeleted > 0) {
        memmove(&str[cursorX], &str[cursorX + nDeleted], i - cursorX - nDeleted);
        i -= nDeleted;
        str[i] = nDeleted = 0;
      }
      if (cursorX == 0 && (ch == 8 || ch == 127))
        ;
      else
        ungetc(ch, stream);
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
      if (ch == 51) { // Forward Delete
        if ((ch = fgetc(stream)) == EOF)
          break;
        else if (ch != 126) // ~
          break;
        if (cursorX == i)
          continue;
        // Implement if needed
        continue;
      }
      switch (ch) {
      case 67: if (cursorX < i) { write(1, "\e[C", 3); ++cursorX; } continue; // RightArrow
      case 68: if (cursorX > 0) { write(1, "\e[D", 3); --cursorX; } continue; // LeftArrow
      case 65: strncpy(str, "__PREV_HIST", 12); break; // UpArrow
      case 66: strncpy(str, "__NEXT_HIST", 12); break; // DownArrow
      }
      setCanonicalMode();
      return str;
    }
    else if (ch == 6) { // Control-F
      if (cursorX < i) {
        write(1, "\e[C", 3);
        ++cursorX;
      }
    }
    else if (ch == 2) { // Control-B
      if (cursorX > 0) {
        write(1, "\e[D", 3);
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
    else if (ch == 21) { // Control-U
      if (cursorX <= 0)
        continue;
      str[i] = 0;
      printf("\e[%dD\e[K\e7%s\e8", cursorX, &str[cursorX]);
      memmove(str, &str[cursorX], i - cursorX);
      i -= cursorX;
      cursorX = 0;
    }
    else if (ch == 11) { // Control-K
      write(1, "\e[K", 3);
      str[cursorX] = 0;
      i -= i - cursorX;
    }
    else if (ch == 12) { // Control-L
      strncpy(str, "clear", 6);
      setCanonicalMode();
      return str;
    }
  }
  if (nInserted > 0) {
    memmove(&str[cursorX], &str[cursorX - nInserted], insertBuf[LINE_SIZE]);
    strncpy(&str[cursorX - nInserted], insertBuf, nInserted);
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
  initTc(defaultTokens, sizeof defaultTokens / sizeof defaultTokens[0]);
  if (argc >= 2) {
    if (loadText((String) argv[1], text, 10000) != 0)
      exit(1);
    run(text);
    exit(0);
  }

  int status = 0;
  initTerm();
  for (int next = 1, nLines = 0;;) {
    if (next)
      printf("[%d]> ", ++nLines);
    if (readLine(text, LINE_SIZE, stdin) == NULL) {
      printf("\n");
      goto exit;
    }
    int inputLen = strlen(text);
    if (text[inputLen - 1] == '\n')
      text[--inputLen] = 0;

    next = 1;
    String semicolonPos = removeTrailingSemicolon(text, inputLen);
    if (strcmp(text, "exit") == 0)
      goto exit;
#if defined(__APPLE__) || defined(__linux__)
    else if (strcmp(text, "__PREV_HIST") == 0) {
      eraseLine();
      printf("[%d]> ", nLines);
      showHistory(Prev, text);
      next = 0;
    }
    else if (strcmp(text, "__NEXT_HIST") == 0) {
      eraseLine();
      printf("[%d]> ", nLines);
      showHistory(Next, text);
      next = 0;
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
    }
#endif
    else {
      if (semicolonPos)
        *semicolonPos = ';';
      run(text);
    }
  }
exit:
  destroyTerm();
  exit(status);
}
