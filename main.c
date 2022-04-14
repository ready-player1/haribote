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

enum keyId {
  WildCard = 0,
  WildCardForExpr,
  WildCardForExpr0,
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
  PlusPlus,
  ShiftRight,
  Equal,
  NotEq,
  LesEq,
  GtrEq,
  Les,
  Gtr,
  Colon,
  Lparen,
  Rparen,
  Lbracket,
  Rbracket,
  Lbrace,
  Rbrace,
  Plus,
  Minus,
  Multi,
  Divi,
  Mod,
  Period,
  Comma,
  Semicolon,
  Assign,
  BitwiseAnd,

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

  Print,
  If,
  Goto,
  Time,

  EndOfKeys
};

String defaultTokens[] = {
  "!!*",
  "!!**",
  "!!***",
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
  "++",
  ">>",
  "==",
  "!=",
  "<=",
  ">=",
  "<",
  ">",
  ":",
  "(",
  ")",
  "[",
  "]",
  "{",
  "}",
  "+",
  "-",
  "*",
  "/",
  "%",
  ".",
  ",",
  ";",
  "=",
  "&",

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

  "print",
  "if",
  "goto",
  "time",
};

void initTokenCodes(String *defaultTokens, int len)
{
  if (len != EndOfKeys) {
    printf("warong number of default tokens: expected %d, got %d\n", EndOfKeys, len);
    exit(1);
  }
  for (int i = 0; i < len; ++i)
    tokenCodes[i] = getTokenCode(defaultTokens[i], strlen(defaultTokens[i]));
}

#define MAX_PHRASE_LEN 31
#define WPC_LEN 9
int tokenCodesForPhrase[ (MAX_PHRASE_LEN + 1) * 100 ]; // フレーズを字句解析して得たトークンコードを格納する
int nextPc, wpc[WPC_LEN]; // 一致したフレーズの次のトークンを指す, ワイルドカードのトークンの場所を指す
int wpcEnd[WPC_LEN]; // wpc[n]が式の場合、wpcEnd[n]はその式の直後のトークンを指す

// tokenCodes[pc]からのトークンコード列が、phraseで指定されたトークン列と一致するかどうか調べる
int match(int phraseId, String phrase, int pc)
{
  int head = phraseId * (MAX_PHRASE_LEN + 1); // フレーズを字句解析した結果を格納する（している）場所をphraseIdで指定
  int nTokens;
  if (tokenCodesForPhrase[ head + MAX_PHRASE_LEN ] == 0) { // 含まれるトークンの個数が0のフレーズは字句解析する
    nTokens = lexer(phrase, &tokenCodesForPhrase[head]);
    if (nTokens > MAX_PHRASE_LEN) {
      printf("too long phrase\n");
      exit(1);
    }
    tokenCodesForPhrase[ head + MAX_PHRASE_LEN ] = nTokens;
  }
  nTokens = tokenCodesForPhrase[ head + MAX_PHRASE_LEN ]; // フレーズに含まれるトークンの個数を取得
  int tokenCode;
  int depth;
  for (int i = 0, num; i < nTokens; ++i) {
    tokenCode = tokenCodesForPhrase[head + i];
    if (tokenCode == WildCard || tokenCode == WildCardForExpr || tokenCode == WildCardForExpr0) {
      /*
        WildCard（!!*#）
        任意の1トークンにマッチする（#は0～8までの数字）。

        WildCardForExpr（!!**#）
        任意の式にマッチする（#は0～8までの数字）。
        ただし、式は1トークン以上の長さでなければいけない）。

        WildCardForExpr0（!!***#）
        任意の式にマッチする（#は0～8までの数字）。
        ただし、式は長さゼロでもよい。
      */
      ++i;
      num = tokenCodesForPhrase[head + i] - Zero; // 後続の番号を取得
      wpc[num] = pc; // トークン・式の開始位置
      if (tokenCode == WildCard) {
        ++pc;
        continue;
      }
      depth = 0; // 括弧の深さ
      for (;;) {
        if (tokenCodes[pc] == Semicolon)
          break;
        if (tokenCodes[pc] == Comma && depth == 0)
          break;

        if (tokenCodes[pc] == Lparen || tokenCodes[pc] == Lbracket) // 手抜きで ( と [ を区別せずに数えている
          ++depth;
        if (tokenCodes[pc] == Rparen || tokenCodes[pc] == Rbracket)
          --depth;
        if (depth < 0)
          break;
        ++pc;
      }
      wpcEnd[num] = pc; // 式の終了位置
      if (tokenCode == WildCardForExpr && wpc[num] == pc)
        return 0;
      if (depth > 0)
        return 0;
      continue;
    }
    if (tokenCode != tokenCodes[pc])
      return 0; // マッチせず
    ++pc;
  }
  nextPc = pc;
  return 1; // マッチした
}

typedef int *IntPtr;

IntPtr internalCodes[10000]; // ソースコードを変換して生成した内部コードを格納する
IntPtr *icp;

enum opcode {
  OpCpy = 0,
  OpCeq,
  OpCne,
  OpCle,
  OpCge,
  OpClt,
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
  OpJle,
  OpJge,
  OpJlt,
  OpJgt,
  OpLop,
  OpPrint,
  OpTime,
  OpEnd
};

int correspondingTerms[128] = {-1}; // in token codes and opcodes for infix operators

void initCorrespondingTerms() {
  correspondingTerms[Multi]      = OpMul;
  correspondingTerms[Divi]       = OpDiv;
  correspondingTerms[Mod]        = OpMod;
  correspondingTerms[Plus]       = OpAdd;
  correspondingTerms[Minus]      = OpSub;
  correspondingTerms[ShiftRight] = OpShr;
  correspondingTerms[Les]        = OpClt;
  correspondingTerms[LesEq]      = OpCle;
  correspondingTerms[Gtr]        = OpCgt;
  correspondingTerms[GtrEq]      = OpCge;
  correspondingTerms[Equal]      = OpCeq;
  correspondingTerms[NotEq]      = OpCne;
  correspondingTerms[BitwiseAnd] = OpBand;
  correspondingTerms[Assign]     = OpCpy;
};

int getOpcode(int tokenCode) // for infix operators
{
  if (tokenCode < 0 ||  EndOfKeys <= tokenCode) {
    printf("tokenCode must be greater than 0 and less than %d, got %d\n", EndOfKeys, tokenCode);
    exit(1);
  }

  int op = correspondingTerms[tokenCode];
  if (op == -1) {
    printf("no corresponding opcode for %s token\n", tokenStrs[tokenCode]);
    exit(1);
  }
  return op;
}

void putIc(int op, IntPtr p1, IntPtr p2, IntPtr p3, IntPtr p4)
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

// 未使用の一時変数を確保する
int tmpAlloc()
{
  int i = 0;
  while (tmpFlags[i] != 0) {
    if (i >= N_TMPS)
      return -1;
    ++i;
  }
  tmpFlags[i] = 1;
  return Tmp0 + i;
}

// 一時変数を未使用の状態に戻す（ただし、指定されたトークンコードが一時変数でないときは何もしない）
void tmpFree(int tokenCode)
{
  if (Tmp0 <= tokenCode && tokenCode <= Tmp9)
    tmpFlags[ tokenCode - Tmp0 ] = 0;
}

#define LOWEST_PRECEDENCE 99
int epc, epcEnd; // exprのためのpc（式のどこを実行しているかを指す）, その式の直後のトークンを指す

int evalExpression(int precedenceLevel); // evalInfixExpression()が参照するので
int expression(int num);

enum notationStyle { Prefix = 0, Infix };

typedef struct precedence {
  int operator;
  int level;
} Precedence;

#define N_OPERATORS 14

Precedence precedenceTable[2][ N_OPERATORS + 1 ] = {
  { // Prefix
    {PlusPlus, 2},
    {Minus, 2},
    {.level = LOWEST_PRECEDENCE + 1}
  },
  { // Infix
    {Multi, 4},
    {Divi, 4},
    {Mod, 4},
    {Plus, 5},
    {Minus, 5},
    {ShiftRight, 6},
    {LesEq, 7},
    {GtrEq, 7},
    {Les, 7},
    {Gtr, 7},
    {Equal, 8},
    {NotEq, 8},
    {BitwiseAnd, 9},
    {Assign, 15},
    {.level = LOWEST_PRECEDENCE + 1}
  }
};

int getPrecedenceLevel(int notationStyle, int operator)
{
  int i = 0;
  Precedence *precedence;
  while ((precedence = &precedenceTable[notationStyle][i])->level != LOWEST_PRECEDENCE + 1) {
    if (operator == precedence->operator)
      break;
    ++i;
  }
  return precedence->level;
}

int evalInfixExpression(int i, int precedenceLevel, int op)
{
  int j, k;
  ++epc;
  j = evalExpression(precedenceLevel);
  k = tmpAlloc();
  putIc(op, &vars[k], &vars[i], &vars[j], 0);
  tmpFree(i);
  tmpFree(j);
  if (i < 0 || j < 0)
    return -1;
  return k;
}

int evalExpression(int precedenceLevel)
{
  int er = -1; // ここまでの計算結果が入っている変数のトークンコード（vars[er]で計算結果にアクセスできる）
  int e0 = 0;

  nextPc = 0;

  if (match(99, "( !!**0 )", epc)) { // 括弧
    er = expression(0);
  }
  else if (tokenCodes[epc] == PlusPlus) { // 前置インクリメント
    ++epc;
    er = evalExpression(getPrecedenceLevel(Prefix, PlusPlus));
    putIc(OpAdd1, &vars[er], 0, 0, 0);
  }
  else if (tokenCodes[epc] == Minus) { // 単項マイナス
    ++epc;
    e0 = evalExpression(getPrecedenceLevel(Prefix, Minus));
    er = tmpAlloc();
    putIc(OpNeg, &vars[er], &vars[e0], 0, 0);
  }
  else { // 変数もしくは定数
    er = tokenCodes[epc];
    ++epc;
  }
  if (nextPc > 0)
    epc = nextPc;

  int encountered; // ぶつかった演算子の優先順位を格納する
  int tokenCode;
  for (;;) {
    tmpFree(e0);
    if (er < 0 || e0 < 0) // ここまででエラーがあれば、処理を打ち切り
      return -1;
    if (epc >= epcEnd)
      break;

    e0 = 0;
    tokenCode = tokenCodes[epc];
    if (tokenCode == PlusPlus) { // 後置インクリメント
      ++epc;
      e0 = er;
      er = tmpAlloc();
      putIc(OpCpy, &vars[er], &vars[e0], 0, 0);
      putIc(OpAdd1, &vars[e0], 0, 0, 0);
    }
    else if (precedenceLevel >= (encountered = getPrecedenceLevel(Infix, tokenCode))) {
      /*
        「引数として渡された優先順位」が「ぶつかった演算子の優先順位」よりも低いか又は等しい
        (値が大きいか又は等しい)ときは、このブロックを実行して中置演算子を評価する。

        「引数として渡された優先順位」が「ぶつかった演算子の優先順位」よりも高い(値が小さい)
        ときは、このブロックを実行せずにこれまでに式を評価した結果を呼び出し元に返す。
      */
      switch (tokenCode) {
      // 左結合
      case Multi: case Divi: case Mod:
      case Plus: case Minus:
      case ShiftRight:
      case Les: case LesEq: case Gtr: case GtrEq:
      case Equal: case NotEq:
      case BitwiseAnd:
        er = evalInfixExpression(er, encountered - 1, getOpcode(tokenCode));
        break;
      // 右結合
      case Assign:
        ++epc;
        e0 = evalExpression(encountered);
        putIc(OpCpy, &vars[er], &vars[e0], 0, 0);
        break;
      }
    }
    else
      break;
  }
  return er;
}

// 引数として渡したワイルドカード番号にマッチした式をコンパイルしてinternalCodes[]に書き込む
int expression(int num)
{
  int expressionBegin = wpc   [num];
  int expressionEnd   = wpcEnd[num];
  if (expressionBegin == expressionEnd)
    return 0;

  // evalExpression()の中でmatch()やexpression()を呼び出すこともあり得るので、変数を退避しておく
  int oldEpc    = epc;
  int oldEpcEnd = epcEnd;
  int buf[WPC_LEN * 2 + 1];
  for (int i = 0; i < WPC_LEN; ++i) {
    buf[i]           = wpc   [i];
    buf[i + WPC_LEN] = wpcEnd[i];
  }
  buf[WPC_LEN * 2] = nextPc;

  epc    = expressionBegin;
  epcEnd = expressionEnd;
  int er = evalExpression(LOWEST_PRECEDENCE);
  if (epc < epcEnd) // 式を最後まで解釈できなかったらエラー
    return -1;

  // 保存しておいた変数を復元する
  epc    = oldEpc;
  epcEnd = oldEpcEnd;
  for (int i = 0; i < WPC_LEN; ++i) {
    wpc   [i] = buf[i];
    wpcEnd[i] = buf[i + WPC_LEN];
  }
  nextPc = buf[WPC_LEN * 2];
  return er;
}

int compile(String sourceCode)
{
  int nTokens = lexer(sourceCode, tokenCodes);

  String *ts = tokenStrs;  // 添字に指定したトークンコードに対応するトークン文字列のポインタを格納している配列
  int    *tc = tokenCodes; // トークンコードを格納している配列
  tc[nTokens++] = Semicolon; // 末尾に「;」を付け忘れることが多いので、付けてあげる
  tc[nTokens] = tc[nTokens + 1] = tc[nTokens + 2] = tc[nTokens + 3] = Period; // エラー表示用

  icp = internalCodes;

  for (int i = 0; i < N_TMPS; ++i)
    tmpFlags[i] = 0;

  int pc;
  for (pc = 0; pc < nTokens;) {
    int e0 = 0;
    if (match(1, "!!*0 = !!*1;", pc)) {
      putIc(OpCpy, &vars[tc[wpc[0]]], &vars[tc[wpc[1]]], 0, 0);
    }
    else if (match(10, "!!*0 = !!*1 + 1; if (!!*2 < !!*3) goto !!*4;", pc) && tc[wpc[0]] == tc[wpc[1]] && tc[wpc[0]] == tc[wpc[2]]) {
      putIc(OpLop, &vars[tc[wpc[4]]], &vars[tc[wpc[0]]], &vars[tc[wpc[3]]], 0);
    }
    else if (match(9, "!!*0 = !!*1 + 1;", pc) && tc[wpc[0]] == tc[wpc[1]]) { // +1専用の命令
      putIc(OpAdd1, &vars[tc[wpc[0]]], 0, 0, 0);
    }
    else if (match(2, "!!*0 = !!*1 + !!*2;", pc)) {
      putIc(OpAdd, &vars[tc[wpc[0]]], &vars[tc[wpc[1]]], &vars[tc[wpc[2]]], 0);
    }
    else if (match(3, "!!*0 = !!*1 - !!*2;", pc)) {
      putIc(OpSub, &vars[tc[wpc[0]]], &vars[tc[wpc[1]]], &vars[tc[wpc[2]]], 0);
    }
    else if (match(4, "print !!**0;", pc)) {
      e0 = expression(0);
      putIc(OpPrint, &vars[e0], 0, 0, 0);
    }
    else if (match(0, "!!*0:", pc)) { // ラベル定義命令
      vars[tc[wpc[0]]] = icp - internalCodes; // ラベル名の変数にその時のicpの相対位置を入れておく
    }
    else if (match(5, "goto !!*0;", pc)) {
      putIc(OpGoto, &vars[tc[wpc[0]]], 0, 0, 0);
    }
    else if (match(6, "if (!!*0 !!*1 !!*2) goto !!*3;", pc) && Equal <= tc[wpc[1]] && tc[wpc[1]] <= Gtr) {
      putIc(OpJeq + (tc[wpc[1]] - Equal), &vars[tc[wpc[3]]], &vars[tc[wpc[0]]], &vars[tc[wpc[2]]], 0);
    }
    else if (match(7, "time;", pc)) {
      putIc(OpTime, 0, 0, 0, 0);
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
  putIc(OpEnd, 0, 0, 0, 0);

  IntPtr *end = icp;
  int op;
  for (icp = internalCodes; icp < end; icp += 5) { // goto先の設定
    op = (int) icp[0];
    if (OpGoto <= op && op <= OpLop)
      icp[1] = (IntPtr) (internalCodes + *icp[1]);
  }
  return end - internalCodes;
err:
  printf("syntax error: %s %s %s %s\n", ts[tc[pc]], ts[tc[pc + 1]], ts[tc[pc + 2]], ts[tc[pc + 3]]);
  return -1;
}

void exec()
{
  clock_t t0 = clock();
  icp = internalCodes;
  int i;
  for (;;) {
    switch ((int) icp[0]) {
    case OpNeg:    *icp[1] = -*icp[2];           icp += 5; continue;
    case OpAdd1:   ++(*icp[1]);                  icp += 5; continue;
    case OpMul:    *icp[1] = *icp[2] *  *icp[3]; icp += 5; continue;
    case OpDiv:    *icp[1] = *icp[2] /  *icp[3]; icp += 5; continue;
    case OpMod:    *icp[1] = *icp[2] %  *icp[3]; icp += 5; continue;
    case OpAdd:    *icp[1] = *icp[2] +  *icp[3]; icp += 5; continue;
    case OpSub:    *icp[1] = *icp[2] -  *icp[3]; icp += 5; continue;
    case OpShr:    *icp[1] = *icp[2] >> *icp[3]; icp += 5; continue;
    case OpClt:    *icp[1] = *icp[2] <  *icp[3]; icp += 5; continue;
    case OpCle:    *icp[1] = *icp[2] <= *icp[3]; icp += 5; continue;
    case OpCgt:    *icp[1] = *icp[2] >  *icp[3]; icp += 5; continue;
    case OpCge:    *icp[1] = *icp[2] >= *icp[3]; icp += 5; continue;
    case OpCeq:    *icp[1] = *icp[2] == *icp[3]; icp += 5; continue;
    case OpCne:    *icp[1] = *icp[2] != *icp[3]; icp += 5; continue;
    case OpBand:   *icp[1] = *icp[2] &  *icp[3]; icp += 5; continue;
    case OpCpy:    *icp[1] = *icp[2];            icp += 5; continue;
    case OpPrint:
      printf("%d\n", *icp[1]);
      icp += 5;
      continue;
    case OpGoto:                            icp = (IntPtr *) icp[1]; continue;
    case OpJeq:   if (*icp[2] == *icp[3]) { icp = (IntPtr *) icp[1]; continue; } icp += 5; continue;
    case OpJne:   if (*icp[2] != *icp[3]) { icp = (IntPtr *) icp[1]; continue; } icp += 5; continue;
    case OpJle:   if (*icp[2] <= *icp[3]) { icp = (IntPtr *) icp[1]; continue; } icp += 5; continue;
    case OpJge:   if (*icp[2] >= *icp[3]) { icp = (IntPtr *) icp[1]; continue; } icp += 5; continue;
    case OpJlt:   if (*icp[2] <  *icp[3]) { icp = (IntPtr *) icp[1]; continue; } icp += 5; continue;
    case OpJgt:   if (*icp[2] >  *icp[3]) { icp = (IntPtr *) icp[1]; continue; } icp += 5; continue;
    case OpTime:
      printf("time: %.3f[sec]\n", (clock() - t0) / (double) CLOCKS_PER_SEC);
      icp += 5;
      continue;
    case OpEnd:
      return;
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

int run(String sourceCode)
{
  if (compile(sourceCode) < 0)
    return 1;
  exec();
  return 0;
}

int main(int argc, const char **argv)
{
  unsigned char text[10000]; // ソースコード

  initTokenCodes(defaultTokens, sizeof defaultTokens / sizeof defaultTokens[0]);
  initCorrespondingTerms();

  if (argc >= 2) {
    if (loadText((String) argv[1], text, 10000) != 0)
      exit(1);
    run(text);
    exit(0);
  }

  int inputLen;
  int hasRemovedSemicolon;
  char *semicolonPos;
  for (int i = 0;; ++i) { // Read-Eval-Print Loop
    printf("[%d]> ", i);
    fgets(text, 10000, stdin);
    inputLen = strlen(text);
    if (text[inputLen - 1] == '\n') // chomp
      text[inputLen - 1] = 0;

    hasRemovedSemicolon = 0;
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
