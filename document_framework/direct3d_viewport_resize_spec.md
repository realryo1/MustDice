# Direct3D のビューポート/リサイズ仕様

対象: `shader/renderer.cpp` / `shader/renderer.h`  
（旧 `framework/direct3d.cpp` は統合・削除済み。公開 API 名に `Direct3D_` プレフィックスが残る）

## 1. 結論

**ウィンドウサイズ変更時の 2D/3D の見せ方は、バックバッファ自動リサイズではなく、クライアントサイズに基づくビューポート再設定で制御**している。

- `Direct3D_SetViewport2D()` / `Direct3D_SetViewport3D()` は **static 内部関数**。描画先ビューポートのみ設定し、バッファの生成・破棄はしない
- これらは `SetDepthEnable(true/false)` から呼ばれる（公開 API ではない）
- バックバッファ / 深度バッファの再構築は `configureBackBuffer()` / `releaseBackBuffer()` 経由で、`Direct3D_Resize(...)` が担当
- `Direct3D_ResizeWindow()` は **クライアント領域サイズの記録のみ**（D3D リソースは変更しない）

## 2. 関数別仕様

### 2.1 `Direct3D_SetViewport2D()`（内部）

**役割**
- 2D 向けに `DRAW_SCREEN_X x DRAW_SCREEN_Y` の比率を保つビューポートを設定
- ウィンドウアスペクトに応じて中央寄せのレターボックス/ピラーボックスを作る

**挙動**
- `targetAspect = DRAW_SCREEN_X / DRAW_SCREEN_Y`
- `windowAspect = g_ClientWidth / g_ClientHeight`
- `windowAspect > targetAspect`（横長）:
  - 縦基準 → `vpH = g_ClientHeight`, `vpW = g_ClientHeight * targetAspect`（左右余白）
- それ以外（縦長または同等）:
  - 横基準 → `vpW = g_ClientWidth`, `vpH = g_ClientWidth / targetAspect`（上下余白）

**重要点**
- 最終 `D3D11_VIEWPORT` は `g_BackBufferDesc.Width/Height` とクライアントサイズの比率でスケーリングされる
- **バッファ再作成はしない**

### 2.2 `Direct3D_SetViewport3D()`（内部）

**役割**
- 3D 向けに同じターゲットアスペクトを保つが、2D と逆方向に「はみ出し」を作る

**挙動**
- 横長: 横いっぱい、縦がはみ出す（`vpY` で中央寄せ）
- 縦長: 縦いっぱい、横がはみ出す（`vpX` で中央寄せ）

**重要点**
- ビューポート設定のみ
- `SetDepthEnable(true)` 時に呼ばれる。3D のアスペクト調整は Depth ON とセット

### 2.3 `Direct3D_ResizeWindow(unsigned int clientW, unsigned int clientH)`（公開）

**役割**
- クライアント領域サイズを内部変数へ保存

**挙動**
- `g_ClientWidth` / `g_ClientHeight` を更新。0 以下相当なら `1.0f` に補正

**重要点**
- **D3D バッファはリサイズしない**
- `main.cpp` の初期化時と `WM_SIZE` 処理から呼ばれる想定

### 2.4 `Direct3D_GetClientWidth()` / `Direct3D_GetClientHeight()`（公開）

- 戻り値は `float`
- `Direct3D_ResizeWindow()` で更新される
- 初期値はそれぞれ `DRAW_SCREEN_X` / `DRAW_SCREEN_Y`

## 3. バックバッファと深度バッファのリサイズ仕様

### 3.1 初期化時（`InitRenderer`）

スワップチェーンのバックバッファ解像度は次に固定される。

- `BufferDesc.Width = DRAW_SCREEN_X`（3840）
- `BufferDesc.Height = DRAW_SCREEN_Y`（2160）

**ウィンドウサイズに追従してバックバッファが自動変更される設計ではない。**

その後 `configureBackBuffer()` で以下を生成する。

- バックバッファの `RenderTargetView`
- `g_BackBufferDesc`
- 同解像度の深度ステンシルバッファ / ビュー
- 基準ビューポート

### 3.2 `Direct3D_Resize(UINT width, UINT height)`（公開）

バックバッファ / 深度バッファの再構築を担当する。

**手順**
1. `releaseBackBuffer()` で既存 RTV / DSV 等を解放
2. `g_SwapChain->ResizeBuffers(...)`
3. `configureBackBuffer()` で再生成

**重要点**
- 呼ばれない限り描画先バッファサイズは変わらない
- `Direct3D_ResizeWindow()` だけでは実バッファは変わらない

### 3.3 `configureBackBuffer()` / `releaseBackBuffer()`（内部）

生成: `g_pRenderTargetView`, `g_BackBufferDesc`, 深度バッファ / ビュー, `g_Viewport`  
解放: RTV / 深度バッファ / DSV

## 4. ウィンドウサイズ変更時の流れ

### 4.1 クライアントサイズ通知

- `Direct3D_ResizeWindow(newClientW, newClientH)`
- `g_ClientWidth` / `g_ClientHeight` 更新 → 以降の 2D/3D ビューポート計算に反映

### 4.2 必要に応じた D3D リソース再構築

- `Direct3D_Resize(newWidth, newHeight)`
- バックバッファと深度バッファを作り直す

`WM_SIZE` ではクライアント通知に加え、実装によってはバッファ再構築も行う（`main.cpp` / `WndProc` を参照）。

## 5. 2D/3D の使い分け

### 2D
- `SetDepthEnable(false)` → 内部で `Direct3D_SetViewport2D()`
- アスペクト維持の中央領域（レター/ピラーボックス）

### 3D
- `SetDepthEnable(true)` → 内部で `Direct3D_SetViewport3D()`
- ボックスの向きが 2D と逆

※ 旧ドキュメントの `SetDepthTest` は現行では `SetDepthEnable`。

## 6. 実装上の注意点

- ビューポート計算は `g_ClientWidth` / `g_ClientHeight` 前提 → `Direct3D_ResizeWindow()` が必須
- クライアントサイズは最低 `1.0f` に丸められる
- バックバッファ実サイズとクライアントサイズは別概念
- UI 配置は常に `SCREEN_X/HEIGHT`（1280×720）基準。描画解像度 `DRAW_SCREEN_*` を位置計算に使わない

## 7. まとめ

| 層 | API |
|---|---|
| クライアントサイズ管理 | `Direct3D_ResizeWindow` / `GetClientWidth` / `GetClientHeight` |
| 描画領域調整 | `SetDepthEnable` → 内部 `SetViewport2D/3D` |
| 実バッファ再生成 | `Direct3D_Resize` + `configureBackBuffer` |

特に重要なのは、**ウィンドウサイズ変更だけでは実バッファは変わらず、`Direct3D_Resize()` を呼んだ場合にのみバックバッファと深度バッファが再構築される**点である（クライアント通知だけではビューポート計算の基準値だけが更新される）。
