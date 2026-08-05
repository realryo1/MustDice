# MustDice

（概要考え中）

## 操作方法

| 入力                       | 動作                           |
| ------------------------ | ---------------------------- |
| Space / Enter / ゲームパッド A | 決定（シーン／フェーズ進行）               |
| Escape / START           | ポーズ用アクション（未使用）               |
| BackSpace / B            | キャンセル（賭け枝・奇偶選択など）         |
| F2                       | スクリーンショット（`screenshot/` に保存） |
| F11                      | ボーダレスウィンドウ切替                 |

## ドキュメントについて

| フォルダ | 用途 |
| --- | --- |
| [`document_framework/`](document_framework/) | フレームワーク本体の使い方・API・仕様。描画・入力・フォントなど共通基盤向け  |
| [`document_ingame/`](document_ingame/) | ゲーム側（インゲーム）の資料。権利表記やコンテンツ固有のメモなど |

### 初めに見るものは？
フレームワークを触るときは [framework_usage.md](document_framework\framework_usage.md)、

ゲームの要素にかかわるものは[about_game.md](document_ingame\about_game.md)を見る。

## デバッグシーン（`SCENE_DEBUG`）

`scene.cpp` の初期値を `SCENE_DEBUG` にするか、`SetSceneFade(SCENE_DEBUG)` を呼ぶ。
Releaseビルドでは除外され、Debugシーンには入れなくなる。


| キー         | 動作                               |
| ---------- | -------------------------------- |
| Tab        | サブシーン切替（MODEL → LIGHTING → TOON） |
| Esc        | マウスアンロック                         |
| WASD + マウス | デバッグカメラ移動・視点（ロック時）               |

## エンコーディング修正について

AIエージェントでの実装の完了後は必ず[encoding_converter.py](tool\encoding_converter.py)を実行し、エンコーディングを整える。