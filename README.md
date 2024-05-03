# 概要

projectorは、シンボル置換を行いながら、ディレクトリツリーをコピーするコマンドラインツールです。

# 解説

```
$ projector HelloWorld:MyNew templates/HelloWorldApp work/
```

上記のように実行すると、`templates/HelloWorldApp`に含まれるディレクトリ名やファイル名、ファイルの内容に対して、シンボル置換を行いながら、`work/MyNewApp`としてコピーします。
第一引数で、変換元シンボルと変換先シンボルを指定します。大文字小文字は維持され、単語間にある '` `'、'`-`'、'`_`' もそのまま残ります。従って、以下のような置換が行われます。

- `HelloWorldApp` → `MyNewApp`
- `hello-world-app` → `my-new-app`
- `HELLO_WORLD_APP` → `MY_NEW_APP`
- `Hello world app` → `My new app`

このコマンドを用いると、ソースコードやドキュメントを含むテンプレートプロジェクトを元に、名前を変更した新しいプロジェクトを生成することができます。

# ビルド方法

ビルドツールとして`qmake`を採用しています。以下のようにしてビルドできます。

```
mkdir build
cd build
qmake ..
make
```

`qmake`を使いたくない場合、すべての `.cpp` ファイルを`g++`か`clang++`でコンパイルしてください。

# 既知の問題

テキストファイルとバイナリファイルの判別を行っていないため、バイナリファイル内に、変換対象と一致するシンボルが含まれる場合、文字列の長さが変わった結果、ファイルが破損することがあります。
