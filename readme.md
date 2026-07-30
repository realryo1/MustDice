# MustDice

DirectX 11のゲーム向けフレームワーク。描画・入力・シーン・アセット管理の土台として使える状態になってるはず。

詳細な API 説明は `document/` 内を参照。

[document/framework_usage.md](document/framework_usage.md) 
 [権利表記](document/copylight.md) 

## 操作方法


| 入力                       | 動作                           |
| ------------------------ | ---------------------------- |
| Space / Enter / ゲームパッド A | 決定（シーン進行）                    |
| Escape / START           | ポーズ用アクション（未使用）               |
| BackSpace / B            | キャンセル用アクション（未使用）             |
| F2                       | スクリーンショット（`screenshot/` に保存） |
| F11                      | ボーダレスウィンドウ切替                 |


## ビルド

1. Visual Studio で `MustDice.sln` を開く
2. 構成は **x64**（Debug / Release）を推奨
3. ビルドして実行

依存ライブラリ（Assimp / FreeType / DirectXTex など）は `framework/library` 配下を参照。

### デバッグシーン（`SCENE_DEBUG`）

`scene.cpp` の初期値を `SCENE_DEBUG` にするか、`SetSceneFade(SCENE_DEBUG)` を呼ぶ。
Releaseビルドでは除外され、Debugシーンには入れなくなる。


| キー         | 動作                               |
| ---------- | -------------------------------- |
| Tab        | サブシーン切替（MODEL → LIGHTING → TOON） |
| Esc        | マウスアンロック                         |
| WASD + マウス | デバッグカメラ移動・視点（ロック時）               |


