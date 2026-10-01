# ahc_cpp

AtCoder Heuristic Contest（AHC）向けの開発環境です。DockerでC++のビルドとローカルテストを実行できます。

## 含まれるツール

- GCC / Clang：C++のビルド・デバッグ用
- AtCoder Library（ACL）：`#include <atcoder/all>` で利用可能
- pahcer：ローカルテストを並列実行するツール
- Rust / Cargo：公式ローカルテストツールのビルド用
- Python 3 / pip：スクリプトの実行や補助ツールの導入用

コンテナ内ではrootユーザーで作業します。

## 使い方

### 前提

- Docker / Docker Compose v2
- （任意）VS Code + Dev Containers 拡張機能

### 1. 解答とテストツールを用意する

解答コードを `work/main.cpp` に置き、コンテストのページからダウンロードした公式ローカルテストツールのRustソースを `work/tools/` に配置してください。
入力ファイルの生成方法は、公式ツールに付属するREADMEを参照してください。

```text
ahc_cpp/
├ .devcontainer/
└ work/
  ├ main.cpp              # 解答コード
  └ tools/                # 公式ローカルテストツール
```

ホスト側の `work/` は、コンテナ内の `/work` にマウントされます。コンテナは `/work` を作業ディレクトリとして起動します。

### 2. コンテナをビルドして起動する

ホスト側で `ahc_cpp` ディレクトリに移動し、コンテナをビルドして起動します。

```bash
cd ahc_cpp
docker compose -f .devcontainer/docker-compose.yml build
docker compose -f .devcontainer/docker-compose.yml run --rm dev bash
```

Dockerfileやpahcerのラッパーを変更した場合も、上のコマンドで再ビルドし、新しいコンテナを起動してください。
VS Code Dev Containersを利用している場合は、`Dev Containers: Rebuild Container` を実行してください。

### 3. pahcerを初期化してテストを実行する

ここからはコンテナ内の `/work` で実行します。`ahc000` は対象のコンテスト名に置き換えてください。

```bash
pahcer init -p ahc000 -o max -l cpp
pahcer run
```

スコアを最大化する問題は `-o max`、最小化する問題は `-o min` を指定します。
インタラクティブ問題では、初期化時に `-i` を追加してください。

`pahcer init` は `/work/pahcer_config.toml` を生成します。既存の設定ファイルがある場合は初期化に失敗するため、設定を変えるときはそのファイルを編集してください。
デフォルトではseed 0〜99の100ケースを実行します。入力ファイルの数に合わせて、設定の `start_seed` と `end_seed` を調整してください。`end_seed` 自体は実行範囲に含まれません。

## pahcerの自動設定

C++の非インタラクティブ問題では、`pahcer init` 後にラッパーが次のビルド処理を自動追加します。
これにより、`pahcer run` は解答をコンパイルした後、全ケースの実行前に `vis` を一度ビルドします。

```toml
[[test.compile_steps]]
program = "cargo"
args = ["build", "--release", "--bin", "vis"]
current_dir = "./tools"
```

スコア抽出式も、`Score = 12345` のような整数スコアを読み取る形式に設定します。問題名と最大化・最小化の指定には、`init` の引数がそのまま反映されます。

`-i` 指定時とC++以外の言語では、公式pahcerの設定をそのまま使います。自動設定は新規の初期化時に適用され、既存の設定ファイルは変更しません。

## 参考資料

- [pahcerのREADME](https://github.com/terry-u16/pahcer/blob/main/README.md)：コマンドと設定項目の詳細
- [eijirouさんのビームサーチライブラリの解説](https://eijirou-kyopro.hatenablog.com/entry/2024/02/01/115639)：[ビームサーチのテンプレート](work/beamsearch_template.cpp) を同梱
