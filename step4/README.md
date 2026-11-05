# step4：実際に buffer を溢れさせて壊す

> ここで「短い入力は動く／長すぎる入力は壊れる」を自分の目で見た。

## やること

16箱しかない buf に、それより長い入力を流し込んで、プログラムをクラッシュさせる。

## コード（step4.c）

```c
#include <stdio.h>
#include <string.h>
void vulnerable(char *input){
    char buf[16];          // 16箱だけの入れ物
    strcpy(buf, input);    // 長さを確認せず buf にコピー（←ここが危ない）
    printf("bufに入った: %s\n", buf);
}
int main(int argc, char **argv){
    vulnerable(argv[1]);
    printf("main: 無事に戻ってこられた\n");   // 戻り先メモが無事ならここに来る
    return 0;
}
```

## ビルドと実行

```bash
gcc -g -O0 -fno-stack-protector -o step4 step4.c   # カナリアOFF
# Mac なら cc -g -O0 -fno-stack-protector -o step4 step4.c

./step4 hello                              # ① 短い入力：収まる
./step4 AAAAAAAAAAAAAAAAAAAAAAAAAAAAAA     # ② 長すぎる入力：溢れる
```

## 結果

```
# ① 短い入力
bufに入った: hello
main: 無事に戻ってこられた        ← 両方出る＝正常

# ② 長すぎる入力
（Linux）  Segmentation fault
（Mac/ARM）SIGTRAP (Trace/breakpoint trap)
          → "main: 無事に戻ってこられた" が出ない＝関数から戻る瞬間に落ちた
```

何が起きたか：長い A が buf(16箱)を溢れ、隣の**戻り先メモを A で上書き**した。
関数終了時、CPUは戻り先として A（＝数値 0x41）だらけの番地へ飛ぼうとして、存在しない番地なので墜落。

> 補足：Mac(Apple Silicon) が `SIGTRAP` で止まるのは、
> 「戻り先が壊された！」とチップが気づいて、変な番地に飛ぶ前に止めてくれる安全機構のため。
> 止まり方はチップで違うが「長い入力で壊れた」事実は同じ。

## 用語メモ

- **strcpy**：文字列を丸ごとコピーする関数。コピー先が足りるか確認せず、
  文字列の終わり印 `\0` が来るまでコピーし続ける。だから溢れる。バッファオーバーフローの定番の元凶。
- **`char *input`**：input 自身がポインタ（文字列の先頭の番地を持つ箱）。
- **`char **argv`**：ポインタのポインタ。`./step4 hello` のとき argv[0]="./step4"、argv[1]="hello"。
  `vulnerable(argv[1])` は "hello"（の住所）を渡している。
  - 型につく `*` は「ポインタですよ」というラベル（何もしない）。
  - 式の中の `*` は「その住所へ行って中身を取り出せ」という動作。別物。

## この回の意味

**行き先はもう自分で書き換えられる**。今はたまたまゴミ(AAAA)を書いて墜落しただけ。
ゴミを「狙った関数の本物の住所」に変えれば、そこへ飛ばせる → step5。
