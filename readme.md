# MustDice

DirectX 11 向けのゲームフレームワークです。旧 GravityNotes からゲーム固有ロジックを切り離し、描画・入力・シーン・アセット管理の土台として使える状態にしています。

## 概要

- ソリューション: `MustDice.sln`
- ウィンドウ名: `MustDice`
- 描画: DirectX 11（論理 UI 1280×720、描画解像度 3840×2160）
- シーン: Title → Game → Result（現状はプレースホルダ）
- 検証用: `SCENE_DEBUG`（モデル / ライティング / トゥーン）

詳細な API 説明は `markdown/` 配下を参照してください。

| ドキュメント | 内容 |
|---|---|
| [markdown/framework_usage.md](markdown/framework_usage.md) | フレームワーク全体の使い方 |
| [markdown/input.md](markdown/input.md) | 入力抽象化 |
| [markdown/anim_sprite3d_usage.md](markdown/anim_sprite3d_usage.md) | スケルタルアニメーション |
| [markdown/font_renderer_usage.md](markdown/font_renderer_usage.md) ほか | フォント / クリックテキスト |
| [markdown/direct3d_viewport_resize_spec.md](markdown/direct3d_viewport_resize_spec.md) | ビューポート / リサイズ |
| [markdown/mouse_camera_implementation_flow.md](markdown/mouse_camera_implementation_flow.md) | マウスとカメラ |
| [markdown/copylight.md](markdown/copylight.md) | 権利表記 |

## ビルド

1. Visual Studio で `MustDice.sln` を開く
2. 構成は **x64**（Debug / Release）を推奨
3. ビルドして実行

依存ライブラリ（Assimp / FreeType / DirectXTex など）は `framework/library` 配下を参照します。

## 操作方法

### 共通（Title / Game / Result）

| 入力 | 動作 |
|---|---|
| Space / Enter / ゲームパッド A | 決定（シーン進行） |
| Escape / START | ポーズ用アクション（現行シーンでは未使用） |
| BackSpace / B | キャンセル用アクション（現行シーンでは未使用） |

決定時は `asset/sound/se/kettei.mp3` が再生されます。

### その他のキー

| キー | 動作 |
|---|---|
| F2 | スクリーンショット（`screenshot/` に保存） |
| F11 | ボーダレスウィンドウ切替 |

### デバッグシーン（`SCENE_DEBUG`）

起動時のシーンは `SCENE_TITLE` です。デバッグを使う場合は `scene.cpp` の初期値を `SCENE_DEBUG` にするか、`SetScene(SCENE_DEBUG)` / `SetSceneFade(SCENE_DEBUG)` を呼んでください。

| キー | 動作 |
|---|---|
| Tab | サブシーン切替（MODEL → LIGHTING → TOON） |
| Esc | マウスアンロック |
| WASD + マウス | デバッグカメラ移動・視点（ロック時） |

## アセット構成（現状）

```
asset/
  font/     KaiseiDecol-Medium.ttf
  model/    cube.fbx, model.fbx
  sound/se/ kettei.mp3
  texture/  fade.png, grass.jpg, icon.ico, normal.png,
            notfound_thumbnail.png, tex.png, Toon2.png
```

権利表記は [markdown/copylight.md](markdown/copylight.md) を参照してください。
