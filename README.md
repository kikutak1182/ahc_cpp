# ahc_cpp

AtCoder Heuristic Contest (AHC) 向けの C++ + AtCoder Library + pahcer 開発環境を Docker で再現します。 
Pythonも使えます。
VS Code Dev Containers / Docker Compose でそのまま使えます。

AHCのページからtoolsをダウンロードし、以下のようなディレクトリ構成で使用してください。（pahcer参照）
```bash
workspace       (プロジェクトルート)
├ main.cpp   (解答プログラムのコード)
└ tools      (AtCoder提供の公式ローカルテストツール)
  ├ src
  └ in
```

あとでoptunaとかも入れたいですね
eijirouさんのビームサーチライブラリを追加しました
https://eijirou-kyopro.hatenablog.com/entry/2024/02/01/115639

---

## 特徴

- **C++ (gcc/clang)** ビルド環境完備  
- **AtCoder Library (ACL)** を `/usr/local/include/atcoder` に配置済み → `#include <atcoder/all>`  
- **pahcer**（提出用展開ツール）導入済み  
- Python3 / pip など補助ツールも同梱
- すべて root ユーザー前提のシンプル構成

---

## 使い方

### 0. 前提

- Docker / Docker Compose v2  
- （任意）VS Code + Dev Containers 拡張機能

### 1. ビルド

```bash
cd ahc_cpp
docker compose -f .devcontainer/docker-compose.yml build
```

### 2. 起動
```bash
docker compose -f .devcontainer/docker-compose.yml run --rm dev bash
```

### 3. pahcerの設定
```bash
pahcer init -p ahc000 -o max -l cpp [-i]
```
詳細はpahcerのREADMEを参照してください。
(https://github.com/terry-u16/pahcer/blob/main/README.md)
