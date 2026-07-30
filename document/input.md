# MustDice 入力仕様および実装ドキュメント

本ドキュメントでは、MustDice における入力抽象化の仕様、現行シーンでの使い方、および `Input_Action` 定数について解説します。

---

## 1. 入力抽象化の概要

接続デバイスに関わらず、ゲーム側の入力は `InputManager`（`framework/input_manager.h` / `input_manager.cpp`）を介して抽象化する。

* **任天堂配列 (ABXY) 強制**: `Input_Initialize` 時に `GAMEPAD_LAYOUT_SWITCH_ABXY` を設定。右ボタンが A、下ボタンが B。
* **シーンからの直接インクルード排除**: シーン実装は `keyboard.h` / `gamepad.h` ではなく `input_manager.h` を使う（デバッグシーンの特殊キー操作は例外あり）。
* **決定 SE**: `INPUT_ACTION_DECIDE` のトリガー成功時に `asset/sound/se/kettei.mp3` を再生する。

追加 API:

```cpp
Input_Vector2 Input_GetMoveVector(void);  // WASD / DPad / LStick
Input_Vector2 Input_GetLookVector(void);  // 矢印 / RStick
void Input_SetRumble(float leftMotor, float rightMotor);
void Input_SetGamepadLayout(Gamepad_Layout layout);
```

---

## 2. アクション定数（Input_Action）とマッピング

現行の `Input_Action` は以下のみ。

| 定数名 | 主な用途 | キーボード | ゲームパッド |
| :--- | :--- | :--- | :--- |
| `INPUT_ACTION_DECIDE` | 決定 / 進行 | `Space`, `Enter` | `A` |
| `INPUT_ACTION_CANCEL` | キャンセル / 戻る | `BackSpace` | `B` |
| `INPUT_ACTION_MENU_UP` | メニュー上 | `W`, `↑` | `DPad-UP` / `LStick-UP` (※1) |
| `INPUT_ACTION_MENU_DOWN` | メニュー下 | `S`, `↓` | `DPad-DOWN` / `LStick-DOWN` (※1) |
| `INPUT_ACTION_MENU_LEFT` | メニュー左 | `A`, `←` | `DPad-LEFT` / `LStick-LEFT` (※1) |
| `INPUT_ACTION_MENU_RIGHT` | メニュー右 | `D`, `→` | `DPad-RIGHT` / `LStick-RIGHT` (※1) |
| `INPUT_ACTION_PAUSE` | ポーズ | `Escape` | `START` |

> **(※1) スティックのトリガー検出**  
> `Input_Update` 内で LStick の前フレーム比較を行い、閾値 `0.5f` を超えた最初の 1 フレームだけを `Input_IsActionTrigger` で返す。長押しによるメニュー誤作動を防ぐ。

判定 API:

```cpp
bool Input_IsActionDown(Input_Action action);     // 押し続け
bool Input_IsActionTrigger(Input_Action action);  // 押した瞬間
```

---

## 3. 各シーンにおける入力処理

現行の Title / Game / Result はプレースホルダ実装。

### SCENE_TITLE（`title.cpp`）
* Decide で `SetSceneFade(SCENE_GAME)`
* 使用アクション: `INPUT_ACTION_DECIDE`

### SCENE_GAME（`game.cpp`）
* Decide で `SetSceneFade(SCENE_RESULT)`
* 使用アクション: `INPUT_ACTION_DECIDE`

### SCENE_RESULT（`result.cpp`）
* Decide で `SetSceneFade(SCENE_TITLE)`
* 使用アクション: `INPUT_ACTION_DECIDE`

### SCENE_DEBUG（`debugscene/debugscene.cpp`）
* **Tab**: サブシーン切替（MODEL → LIGHTING → TOON）
* **Esc**: `UnLockMouse()`
* カメラ操作はデバッグカメラ側（WASD + マウス相対移動）
* 上記は `keyboard.h` / `mouse.h` を直接使用

### その他（メインループ）
* **F2**: スクリーンショット（`scene.cpp`）
* **F11**: ボーダレスウィンドウ切替（`main.cpp`）

---
