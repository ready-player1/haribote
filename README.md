# haribote

[![License: MIT](https://img.shields.io/badge/License-MIT-brightgreen.svg)](https://github.com/ready-player1/haribote/blob/main/LICENSE)

hariboteは、はりぼて言語の処理系です。

はりぼて言語は、川合秀実さんが開発しているプログラミング言語です。2021年に「[10日くらいでできる！プログラミング言語自作入門](http://essen.osask.jp/?a21_txt01)」で発表されました。

> 私はプログラミング言語は（世間の人が思っているよりは）ずっと簡単に作れるものだと思っています。・・・今からここにそのためのテキストを置きます。これを読んで是非作れるようになってください。そうすれば「作りたいけど、作り方が分からない」を卒業することができます。そして今後は「じゃあどんな言語を作ろうか」をメインで考えられるようになるのです。それはとても素敵なことだと思います。

ライセンスについて川合秀実さんに質問したところ、「私はHLシリーズの派生物に対して一切の権利を主張しません」という、うれしいお返事をいただきました（[a21_bbs01](http://essen.osask.jp/?a21_bbs01)）。

はりぼて言語の公式処理系は、作者の川合秀実さんが処理系を可能な限り小さく作ってみて、残ったものを「本質」とみなしてみよう、という趣旨で実装されているため非常にコンパクトな実装になっています。

それに対してhariboteは、HL-4, HL-7, HL-8を中心に「改造のしやすさ」を意識して変更を加えた実装になっています。そのためコード量は増えていますが、公式処理系と同じ構造を持っています。

## Dependencies

- `gcc` or `clang`

HL-9, HL-9aをコンパイルする際は`aclib`が必要です（See [aclライブラリの入手方法](https://essen.osask.jp/?a21_txt01_9#content_1_4)）。

## Building

1. gitを使ってソースコードをクローンします

```
$ git clone https://github.com/ready-player1/haribote
$ cd haribote
```

2. ビルドします

### Building HL-1...HL-8c

with `gcc`:

```
$ gcc -O3 -Wno-unused-result -o haribote main.c
```

with `clang`:

```
$ gcc -O3 -Wno-pointer-sign -o haribote main.c
```

#### Note

macOSでXcodeをインストールした際にあわせてインストールされる`gcc`コマンドは、`clang`を指しています。

```
$ gcc --version
Apple clang version 14.0.0 (clang-1400.0.29.102)
...
```

### Building HL-9, HL-9a (merged into demo branch)

with `gcc`:

```
$ gcc -I/path/to/acl_sdl2 -DAARCH_X64 -O3 -Wno-unused-result -o haribote main.c `sdl2-config --cflags --libs` -lm
```

with `clang`:

```
$ gcc -I/path/to/acl_sdl2 -DAARCH_X64 -O3 -Wno-pointer-sign -o haribote main.c `sdl2-config --cflags --libs`
```

#### Note

`gcc`の例は、Ubuntuに`libsdl2-2.0-0`を`apt`コマンドでインストールして確認しました（aclライブラリはSDL2.0版を使用）。

`clang`の例は、macOS x86_64に`sdl2`を`brew`コマンドでインストールして確認しました（aclライブラリはSDL2.0版を使用）。

## 履歴確認用ブランチ

`main`ブランチや`demo`ブランチは、ソースコードの変更をブランチの先頭にコミットします。バグ修正をおこなうと履歴が残ります。

それに対して`hist`ブランチは、`git rebase -i`コマンドを使って、直接、バグが発生したコミットを書き換えます。バグの痕跡は残りません。

`hist`ブランチは、`main`ブランチや`demo`ブランチにマージされることのない履歴確認用ブランチです。

写経をおこなう際など、差分にバグ修正履歴を含む必要がない場合は、`hist`ブランチに切り替えて`git log -p`コマンドを使うとよいでしょう。

### Switching to hist branch

1. リモートから変更を取得し、`hist`ブランチをチェックアウトします

```
$ git fetch origin hist
$ git checkout hist
```

2. 差分をパッチ形式で表示します

```
$ git log -p
```

### Commits on Jun 24, 2024

gitを使わなくても利用できるように、各バージョンのソースコードを[ブログ](https://masahiro-oono.hatenadiary.com/archive/category/haribote)に置いています。
