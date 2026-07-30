# フレームワーク使い方ガイド

プロジェクト名: **MustDice**（DirectX 11 フレームワーク）

---

## 目次

1. [初期化・終了の全体フロー](#初期化・終了の全体フロー)
2. [シーン構成](#シーン構成)
3. [Renderer（描画エンジン）](#renderer描画エンジン)
4. [Camera（カメラ）](#cameraカメラ)
5. [Sprite2D（2Dスプライト）](#sprite2d2dスプライト)
6. [Sprite3D（3Dモデル）](#sprite3d3dモデル)
7. [AnimSprite3D（スケルタルアニメーション）](#animsprite3dスケルタルアニメーション)
8. [Billboard（3D板ポリゴン）](#billboard3d板ポリゴン)
9. [Movie（動画）](#movie動画)
10. [Sound（音声）](#sound音声)
11. [DrawFont（テキスト描画）](#DrawFontテキスト描画)
12. [Fade（フェード遷移）](#fadeフェード遷移)
13. [Texture / Light / Transform](#texture--light--transform)
14. [Input / Keyboard / Mouse / Gamepad](#input--keyboard--mouse--gamepad)
15. [デバッグ用ユーティリティ](#デバッグ用ユーティリティ)
16. [tool（開発用スクリプト）](#tool開発用スクリプト)
17. [よくある注意点](#よくある注意点)

---

## 初期化・終了の全体フロー

`framework/main.cpp` での実際の順序は以下の通り。

```cpp
// === 初期化 ===
InitRenderer(hInstance, hWnd, TRUE);
// ImGui 初期化
Keyboard_Initialize();
Mouse_Initialize(hWnd);
InitSound();
Input_Initialize();
InitShader();
Font_InitializeGlobalData();
Sprite_Initialize();
Fade_Initialize();
Gamepad_Initialize();
Init();                     // シーン初期化（既定は SCENE_TITLE）

// === 終了 ===
Finalize();                 // シーン終了
ReleaseAllTextures();
ReleaseAllSounds();
Gamepad_Finalize();
Input_Finalize();
UninitSound();
Fade_Finalize();
Font_FinalizeGlobalData();
Sprite_Finalize();
FinalizeShader();
FinalizeRenderer();
```

- `Camera_Initialize()` はメインループでは呼ばない。必要なシーンで自前初期化する（デバッグは `DebugCamera_Initialize`）。
- 毎フレームの論理更新は固定ステップ `1/FPS`。`Fade_Update` → `Update()` → `Draw()` → `Fade_Draw`。
- F11 でボーダレス切替、F2 でスクリーンショット（`TakeScreenshot`）。

---

## シーン構成


| ID             | ファイル           | 現状                                         |
| -------------- | -------------- | ------------------------------------------ |
| `SCENE_TITLE`  | `title.cpp`    | `"TITLE"` 表示。Decide で `SCENE_GAME` へフェード遷移 |
| `SCENE_GAME`   | `game.cpp`     | `"GAME"` 表示。Decide で `SCENE_RESULT` へ      |
| `SCENE_RESULT` | `result.cpp`   | `"RESULT"` 表示。Decide で `SCENE_TITLE` へ     |
| `SCENE_DEBUG`  | `debugscene/*` | モデル／ライティング／トゥーン検証。起動時は未使用                  |


デバッグへ入るには `scene.cpp` の初期 `scene` を `SCENE_DEBUG` にするか、`SetScene(SCENE_DEBUG)` / `SetSceneFade(SCENE_DEBUG)` を呼ぶ。

`SCENE_DEBUG` 内は **Tab** でサブシーン循環（MODEL → LIGHTING → TOON）、**Esc** でマウスアンロック。

---

## Renderer（描画エンジン）

ヘッダ: `shader/renderer.h`

### 毎フレームの描画フロー

```cpp
Clear();

// --- 3D描画 ---
SetDepthEnable(true);   // 内部で 3D ビューポートも設定
// モデルの Draw()

// --- 2D描画 ---
SetDepthEnable(false);  // 内部で 2D ビューポートも設定
// スプライト / Font の Draw()（各 Draw が行列をセットアップ）

Present();
```

`main.cpp` ではシーン `Draw()` の後に `SetDepthEnable(false)` → `Fade_Draw()` → `Present()` を行う。

### 行列・ライト設定

```cpp
SetWorldMatrix(worldMat);
SetViewMatrix(viewMat);
SetProjectionMatrix(projMat);
SetCameraPosition(XMFLOAT3(x, y, z));
SetLight(light);
SetPlayerLights(lights);   // PBR 用 3 点照明
SetParameter(XMFLOAT4(...)); // Toon1 閾値などシェーダー任意パラメータ
```

### ブレンドステート

```cpp
SetBlendState(BLENDSTATE_NONE);   // 合成なし
SetBlendState(BLENDSTATE_ALFA);   // αブレンド
SetBlendState(BLENDSTATE_ADD);    // 加算
SetBlendState(BLENDSTATE_SUB);    // 減算
```

### シェーダータイプ（SHADERTYPE）

`shader/shadermanager.h` より。


| 値                             | 説明                      |
| ----------------------------- | ----------------------- |
| `S_UNLIT`                     | ライティングなし                |
| `S_LAMBERT`                   | 頂点ランバート                 |
| `S_PHONG`                     | ピクセルフォン（汎用の既定寄り）        |
| `S_PBR`                       | PBR + `SetPlayerLights` |
| `S_RIM_LIGHT`                 | リムライト                   |
| `S_OUTLINE`                   | アウトライン                  |
| `S_SHADOW_MAP`                | ShadowMap 深度描画          |
| `S_BILLBOARD_SHADOW_MAP`      | 透過ビルボード用 ShadowMap      |
| `S_SHADOW_RECEIVE`            | 影受け                     |
| `S_NORMAL_MAP_SHADOW_RECEIVE` | 法線マップ + 影受け             |
| `S_PHONG_SHADOW`              | 点光ランバート + 影受け           |
| `S_CHROMAKEY`                 | クロマキー（動画透過）             |
| `S_COOK_TORRANCE`             | Cook-Torrance           |
| `S_DISNEY_PBR`                | Disney PBR              |
| `S_HEMISPHERE`                | 半球ライティング                |
| `S_NORMAL_MAP`                | 法線マップ単体                 |
| `S_POINT_LIGHT`               | ポイントライト                 |
| `S_SPOT_LIGHT`                | スポットライト                 |
| `S_TOON1`                     | 段階トゥーン + 簡易エッジ          |
| `S_TOON2`                     | ランプテクスチャ（`Toon2.png`）   |


検証例: `debugscene/debug_lighting_scene.cpp` / `debug_toon_scene.cpp`。

---

## Camera（カメラ）

ヘッダ: `framework/camera.h`

プレイヤー追従のオービットカメラ。マウス相対移動で yaw/pitch を更新する。

```cpp
Camera_Initialize();
Camera_Finalize();
Camera_Update();

Camera_SetTargetPos(playerPos);
Camera_LookAtPoint(XMFLOAT3(x, y, z));  // 約 0.25 秒で向きを合わせる
GetCamera()->SkipNextInput(2);

Camera_SetSensitivity(1.0f);
Camera_SetDistance(6.0f);

Camera* cam = GetCamera();
XMFLOAT3 pos = cam->GetPos();
float yaw = Camera_GetYaw();
```

ピッチ制限（`define.h`）:

- 上: `PITCH_LIMIT_LOOK_UP = 25.0f`
- 下: `PITCH_LIMIT_LOOK_DOWN = -60.0f`

デバッグ用フリーカメラは `debugscene/debugcamera.h`（WASD + マウス）。

---

## Sprite2D（2Dスプライト）

ヘッダ: `framework/sprite2d.h`

座標は `SCREEN_WIDTH(1280) × SCREEN_HEIGHT(720)` の論理座標。

```cpp
Sprite2D sprite(
    XMFLOAT2(640.0f, 360.0f),
    XMFLOAT2(100.0f, 100.0f),
    0.0f,
    XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f),
    BLENDSTATE_ALFA,
    L"asset\\texture\\image.png"
);
sprite.Draw();
sprite.SetColor(XMFLOAT4(1.0f, 0.5f, 0.5f, 0.8f));
sprite.SetFlipType(FLIPTYPE2D::FLIPTYPE2D_HORIZONTAL);
```

### 分割テクスチャ

```cpp
SplitSprite split(
    XMFLOAT2(100.0f, 100.0f),
    XMFLOAT2(64.0f, 64.0f),
    0.0f,
    XMFLOAT4(1, 1, 1, 1),
    BLENDSTATE_ALFA,
    L"asset\\texture\\spritesheet.png",
    4, 2
);
split.SetTextureNumber(3);
split.Draw();
```

### クリック判定付き

```cpp
ClickSprite2D button(/* Sprite2D と同じ引数 */);
if (button.IsClick()) { /* ... */ }
```


| 値                                   | 説明   |
| ----------------------------------- | ---- |
| `FLIPTYPE2D::FLIPTYPE2D_NONE`       | 反転なし |
| `FLIPTYPE2D::FLIPTYPE2D_HORIZONTAL` | 左右   |
| `FLIPTYPE2D::FLIPTYPE2D_VERTICAL`   | 上下   |
| `FLIPTYPE2D::FLIPTYPE2D_BOTH`       | 両方   |


※ `FLIPTYPE2D` は `enum class`。

---

## Sprite3D（3Dモデル）

ヘッダ: `framework/sprite3d.h`

`.fbx` は Assimp、`.glb` は `GlbModel` に分岐。アニメーションは `AnimSprite3D` を使う。

```cpp
Sprite3D model(
    XMFLOAT3(0.0f, 0.0f, 0.0f),
    XMFLOAT3(1.0f, 1.0f, 1.0f),
    XMFLOAT3(0.0f, 0.0f, 0.0f),
    "asset/model/cube.fbx",
    S_PHONG
);

model.Draw();
model.SetColor(1.0f, 0.5f, 0.5f, 1.0f);
model.SetColorAlpha(0.5f);
model.ResetColor();
model.DrawShadowMap(lightView, lightProj);  // 静的影

XMFLOAT3 originalSize = model.GetModelSize();
XMFLOAT3 displaySize  = model.GetDisplaySize();
```

`Transform3D` 由来: `pos` / `scale` / `rot`（度）。

内部ローダー:

- `.fbx` → `framework/model.h`（`MODEL` / Assimp、スキニング対応）
- `.glb` → `framework/glb_model.h`（`GlbModel`）

通常は `Sprite3D` / `AnimSprite3D` 経由で使い、ローダーを直接触る必要はない。

---

## AnimSprite3D（スケルタルアニメーション）

ヘッダ: `framework/anim_sprite3d.h`

詳細は [anim_sprite3d_usage.md](anim_sprite3d_usage.md) を参照。

```cpp
AnimSprite3D* chara = new AnimSprite3D(
    XMFLOAT3(0, 0, 0), XMFLOAT3(1, 1, 1), XMFLOAT3(0, 0, 0),
    "asset/model/character.fbx", S_PHONG);

chara->SetAnimationBlendDuration(0.2);
chara->PlayAnimationByName("Walk", true);
chara->UpdateAnimation(1.0f / FPS);  // 内部で UpdateBoneMatrices() も実行
chara->Draw();
delete chara;
```

---

## Billboard（3D板ポリゴン）

ヘッダ: `framework/billboard.h`

3D 空間に置く四角形。既定はカメラ追従ビルボード。床など固定板にも切り替え可能。

```cpp
Billboard* bb = new Billboard(
    XMFLOAT3(0, 1, 0), XMFLOAT2(2, 2), XMFLOAT3(0, 0, 0),
    "asset/texture/orb.png", false  // isDoubleSided
);
bb->Update();
bb->Draw();              // または Draw(S_PHONG) でシェーダー指定
bb->SetBillboardMode(false);       // 固定板
bb->SetReceiveShadow(true);        // 影受け（床など）
bb->SetNormalMap("asset/texture/Normal.png");
bb->SetUVAnimation(4, 0.1f);       // 横コマ数, 1コマ秒数
bb->DrawShadowMap(lightView, lightProj);
delete bb;
```

### SplitBilBoard（分割テクスチャ）

ヘッダ: `framework/split_bilboard.h`（`Billboard` 継承）

```cpp
SplitBilBoard* anim = new SplitBilBoard(
    4, 2,  // cols, rows
    XMFLOAT3(0, 1, 0), XMFLOAT2(1, 1), XMFLOAT3(0, 0, 0),
    "asset/texture/sheet.png"
);
anim->SetFPS(12.0f);
anim->SetLoop(true);
anim->Update();
anim->Draw();
anim->SetTextureIndex(3);  // 手動コマ指定も可
delete anim;
```

検証例: `debugscene/debug_lighting_scene.cpp`。

---

## Movie（動画）

ヘッダ: `framework/movie.h`（Media Foundation、`Transform2D` 継承）

MP4 等を 2D テクスチャとして描画。音声は同ファイルから自動再生。

```cpp
Movie* movie = new Movie(
    XMFLOAT2(640.0f, 360.0f),
    640.0f,                 // 幅（高さはアスペクトから算出）
    0.0f,
    XMFLOAT4(1, 1, 1, 1),
    BLENDSTATE_ALFA,
    L"asset/movie/intro.mp4",
    false,  // useChromaKey（緑背景透過 → S_CHROMAKEY）
    true,   // loop
    true    // autoPlay
);
movie->Update();
movie->Draw();   // SetDepthEnable(false) 後
movie->Play();   // 再再生
delete movie;
```

---

## Sound（音声）

ヘッダ: `framework/sound.h`（XAudio2 + Media Foundation、MP3）

```cpp
InitSound();    //main.cppで呼び出し
UninitSound();  //main.cppで呼び出し

SoundData* bgm = LoadMP3(L"asset/sound/bgm.mp3");
SoundData* se  = LoadMP3(L"asset/sound/se/kettei.mp3");

PlaySound(bgm, true);
PlaySound(se, false);
StopSound(bgm);
UnloadSound(bgm);

SetMasterVolume(0.5f);
double sec = GetPlaybackPositionSec(bgm);
```

推奨ボリューム（`define.h`）:

- BGM: `SOUND_BGM_VOLUME = 0.3f`
- SE: `SOUND_SE_VOLUME = 0.4f`

`INPUT_ACTION_DECIDE` のトリガー時、`Input_Initialize` で読み込んだ `kettei.mp3` が自動再生される。

---

## DrawFont（テキスト描画）

ヘッダ: `framework/font.h`

詳細は [font_renderer_usage.md](font_renderer_usage.md) など。

```cpp
Font_InitializeGlobalData();
Font_FinalizeGlobalData();

DrawFont* label = new DrawFont(
    XMFLOAT2(100.0f, 50.0f),
    32.0f,
    0.0f,
    XMFLOAT4(1, 1, 1, 1),
    "Score: 0",
    TA_MIDDLE              // 省略可（既定 TA_MIDDLE）
);
label->Draw();             // SetDepthEnable(false) の後で呼ぶ
label->SetText("Score: 100");
label->PreCacheGlyphs();
delete label;
```

フォント: `asset/font/KaiseiDecol-Medium.ttf`。アトラス 2048×2048、LRU キャッシュ。

### 派生クラス

| クラス | ヘッダ | 用途 | 詳細 |
| ------ | ------ | ---- | ---- |
| `ClickFont` | `ClickFont.h` | ホバー／クリック付き 1 行 | [click_font_usage.md](click_font_usage.md) |
| `MultiLineDrawFont` | `MultiLineDrawFont.h` | 複数行テキスト | [multiline_font_renderer_usage.md](multiline_font_renderer_usage.md) |
| `MultiLineClickFont` | `MultiLineClickFont.h` | 複数行 + クリック | [multiline_click_font_usage.md](multiline_click_font_usage.md) |

---

## Fade（フェード遷移）

ヘッダ: `framework/fade.h`

```cpp
Fade_Initialize();
Fade_Finalize();
Fade_Update();   // 毎フレーム
Fade_Draw();     // 2D の最後（main が Present 直前に呼ぶ）

SetSceneFade(SCENE_GAME);   // フェードアウト → シーン切替 → ウォームアップ → フェードイン
Fade_StartIn();             // SCENE_NONE 暗転後のフェードイン
FADESTAT state = GetFadeState();
```

| 値                | 状態                      |
| ---------------- | ----------------------- |
| `FADE_NONE`      | 非フェード                   |
| `FADE_OUT`       | 暗転中                     |
| `FADE_WAIT_LOAD` | 暗転後、ロード前の 1 フレーム待機      |
| `FADE_WARMUP`    | シーン初期化スパイク逃がし           |
| `FADE_IN`        | 明転中                     |
| `FADE_MAX`       | 完全暗転で待機（`SCENE_NONE` 時） |


速度: α ±0.05f/フレーム（約 20 フレーム ≈ 1/3 秒）。

---

## Texture / Light / Transform

### Texture

ヘッダ: `framework/texture.h`

スプライト等が内部で呼ぶテクスチャキャッシュ。終了時は `main` が `ReleaseAllTextures()` する。

```cpp
ID3D11ShaderResourceView* srv = LoadTexture(L"asset\\texture\\image.png");
ID3D11ShaderResourceView* nrm = LoadTextureLinear(L"asset\\texture\\normal.png");  // 法線用（リニア）
ReleaseTexturesForScene(SCENE_GAME);  // シーン単位解放
UpdateTextureCache();                 // キャッシュ更新（必要時）
```

### Light

ヘッダ: `framework/light.h`

`SetLight` 用のヘルパー。`AmbientLight` + `PointLight::Apply` で一括設定できる。

```cpp
AmbientLight ambient(XMFLOAT4(0.15f, 0.15f, 0.15f, 1.0f));
PointLight point(
    TRUE,
    XMFLOAT4(0, 5, 0, 1),
    XMFLOAT4(1, 1, 1, 1),
    20.0f,   // range
    1.0f     // intensity
);
point.Apply(ambient);  // 内部で SetLight(ToLIGHT(...))
```

PBR の 3 点照明は従来どおり `SetPlayerLights`。検証例: `debugscene/debug_lighting_scene.cpp`。

### Transform（component.h）

ヘッダ: `framework/component.h`

- `Transform3D` … `pos` / `rot`（度）/ `scale`。`Sprite3D`・`AnimSprite3D` の基底。
- `Transform2D` … `pos` / `rot`（度）/ `scale`。`Movie` などの基底。

---

## Input / Keyboard / Mouse / Gamepad

### 入力操作について（原則）

コントローラー・キーボード双方の入力実装をやりやすくするため、**原則** `keyboard.h` / `gamepad.h` **を直接読み取らない**。  
シーンからは `input_manager.h` の `Input_IsActionDown` / `Input_IsActionTrigger` を使う。
（SCENE_DEBUG内ならどうせ除外されるので何やってもいい）

詳細・アクション定数・マッピングは [input.md](input.md) を参照。  
マウス／カメラ設計は [mouse_camera_implementation_flow.md](mouse_camera_implementation_flow.md)。

```cpp
#include "input_manager.h"

// 押し続け / 押した瞬間
if (Input_IsActionDown(INPUT_ACTION_DECIDE)) { /* ... */ }
if (Input_IsActionTrigger(INPUT_ACTION_DECIDE)) { /* ... */ }

Input_Vector2 move = Input_GetMoveVector();  // WASD / DPad / LStick
Input_Vector2 look = Input_GetLookVector();  // 矢印 / RStick
```

| 定数                                     | 主な用途       |
| -------------------------------------- | ---------- |
| `INPUT_ACTION_DECIDE`                  | 決定 / 進行    |
| `INPUT_ACTION_CANCEL`                  | キャンセル / 戻る |
| `INPUT_ACTION_MENU_UP/DOWN/LEFT/RIGHT` | メニュー移動     |
| `INPUT_ACTION_PAUSE`                   | ポーズ        |

### 例外（キーボード直叩き）

デバッグシーンの特殊キー（Tab 切替、Esc でマウスアンロックなど）や、メインループの F2 / F11 のように、抽象アクションに載せない操作のみ `keyboard.h` を直接使ってよい。

```cpp
// 例外: デバッグ等
if (Keyboard_IsKeyDown(KK_W)) { }
if (Keyboard_IsKeyDownTrigger(KK_TAB)) { }
```

主要キー: `KK_A〜KK_Z`, `KK_LEFT/RIGHT/UP/DOWN`, `KK_SPACE`, `KK_ENTER`, `KK_ESCAPE`, `KK_LEFTSHIFT`, `KK_LEFTCONTROL`, `KK_TAB`, `KK_F2`, `KK_F11`

### マウス

```cpp
Mouse_State ms;
Mouse_GetState(&ms);
Mouse_SetMode(MOUSE_POSITION_MODE_RELATIVE);
LockMouse();
UnLockMouse();
```

### Gamepad

ヘッダ: `framework/gamepad.h`

`main` で `Gamepad_Initialize` / `Finalize`。通常プレイは `input_manager` 経由。振動やレイアウト切替など低レベル操作時のみ直接使う。

```cpp
Gamepad_SetLayout(GAMEPAD_LAYOUT_XBOX);  // または GAMEPAD_LAYOUT_SWITCH_ABXY
Gamepad_SetVibration(0, 0.5f, 0.5f);
bool connected = Gamepad_IsConnected(0);
```

---

## デバッグ用ユーティリティ

| ヘッダ | 用途 |
| ------ | ---- |
| `framework/debug_ostream.h` | `hal::dout << "..."` で OutputDebugString（UTF-8） |
| `framework/input_monitor_console.h` | 別コンソールに入力状態を表示（`main` が自動初期化） |
| `framework/main.h` | Win32 / D3D / DirectXTex 共通 include、`SAFE_DELETE`、`SetFPS` |
| `shader/renderer.h` | 描画エンジン API、`SAFE_RELEASE` |

サードパーティ（直接触らない）: `assimp/`・`freetype/`・`imgui/`・`nlohmann/`・`DirectXTex.h`・`stb_truetype.h`。

### SAFE_DELETE

定義: `framework/main.h`

`new` で確保したオブジェクトを解放するマクロ。`nullptr` なら何もしない。解放後はポインタを `nullptr` に戻す。
シーンの `Finalize` などで使う。

```cpp
void Title_Finalize(void)
{
	SAFE_DELETE(g_pTitleText);
	SAFE_DELETE(g_pHintText);
}
```

| 項目 | 内容 |
| ---- | ---- |
| 対象 | `new` / `delete` した単一オブジェクト（`DrawFont*` など） |
| 効果 | `delete` + ポインタを `nullptr` にクリア |
| 再呼び出し | 安全（2 回目は何もしない） |
| 配列 | `new[]` には使わない（`delete[]` が必要） |
| COM | `ID3D11*` 等は `SAFE_RELEASE`（`shader/renderer.h`）を使う |

`main.h` を include していれば利用可能。シーン雛形（`template/template.cpp`）もこの書き方になっている。

### SAFE_RELEASE

定義: `shader/renderer.h`

COM オブジェクト（Direct3D のバッファ・ビュー・シェーダーなど）を解放するマクロ。有効なポインタなら `Release()` を呼び、その後 `nullptr` に戻す。

```cpp
#define SAFE_RELEASE(p) do { if (p) { (p)->Release(); (p) = nullptr; } } while (0)
```

`Finalize` やリソース破棄時に使う。

```cpp
SAFE_RELEASE(m_VertexBuffer);
SAFE_RELEASE(g_RenderTargetView);
SAFE_RELEASE(g_DepthStencilView);
```

| 項目 | 内容 |
| ---- | ---- |
| 対象 | COM（`ID3D11Buffer*`、`ID3D11ShaderResourceView*` など） |
| 効果 | `Release()` + ポインタを `nullptr` にクリア |
| 再呼び出し | 安全（2 回目は何もしない） |
| C++ オブジェクト | `new` したものには使わない（`SAFE_DELETE` を使う） |

`renderer.h` を include していれば利用可能。

---

# tool（開発用スクリプト）

ディレクトリ: `tool/`

ソース編集後のエンコーディング整備や、シーン追加・プロジェクト名変更など、開発用の補助スクリプトを置く。
基本的にrealryo1用なので触れるべきではない（安全のための説明）。

---

## encoding_converter.py

`.editorconfig` のルールに合わせて、対象ディレクトリ直下のファイルを変換する（**サブディレクトリは再帰しない**）。

| 対象                               | 変換後                      |
| -------------------------------- | ------------------------ |
| `.h` / `.hpp` / `.c` / `.cpp` など | UTF-8 **BOM 付き**、改行 CRLF |
| `.hlsl` / `.hlsli`               | UTF-8 **BOM なし**、改行 CRLF |

```powershell
# setupdirectory.txt に列挙したディレクトリを変換
python tool/encoding_converter.py

# 単一ディレクトリのみ（例: /framework）
python tool/encoding_converter.py /framework
```

- `.editorconfig` が無い場合は既定内容で作成する。
- `setupdirectory.txt` が無い場合も既定で作成する。
- 実装・編集の完了後に実行。

`setupdirectory.txt` の書き方:

```
# / = プロジェクトルート直下のみ（サブフォルダは含まない）
/
/framework
/shader
/app
/SCENE_TITLE
```

---

## manage_scene.py

`template/` を雛形に、シーンの追加・削除を対話で行う。

```powershell
python tool/manage_scene.py
```

追加時に更新されるもの:

- `SCENE_XXX/`（`template.h` / `template.cpp` から生成）
- `app/scene.h` の `enum SCENE`
- `app/scene.cpp` の `#include` と `switch` の `case`
- `tool/setupdirectory.txt`
- ルート `*.vcxproj` / `*.vcxproj.filters`

`SCENE_DEBUG` / `SCENE_MAX` / `SCENE_NONE` は対象外。追加・削除後は内部で `encoding_converter` 相当の処理も走る。

---
## rename_project.py

`.sln` / `.vcxproj` / `.rc` などのファイル名と、リポジトリ内テキスト中の旧名称をまとめて置換する。CI（`.github/workflows/*.yml`）も対象。

```powershell
python tool/rename_project.py NewName
python tool/rename_project.py NewName --old MustDice
python tool/rename_project.py NewName --dry-run   # 書き込みなしで確認
python tool/rename_project.py                    # 対話モード
```

実行前に Visual Studio を閉じ、完了後は新しい `.sln` を開き直す。古い名前が残る場合は `.vs/` を削除する。

---
## build.ps1（VSCode系IDE用ビルドツール）

**通常のVisualStudioを使用する場合は関係ない。**
想定される使い方は VS Code 系 IDE（Cursor 含む）の「実行とデバッグ」／タスク経由。`vswhere` または既定パスから MSBuild を探し、`MustDice.sln` を x64 でビルドする。  


| IDE 操作                           | 呼び出し先                                | 実体                                      |
| -------------------------------- | ------------------------------------ | --------------------------------------- |
| 実行とデバッグ → `Debug (Run Only)`     | `preLaunchTask`: Build & Run Debug   | `build.ps1 -Configuration Debug -Run`   |
| 実行とデバッグ → `Release (Run Only)`   | `preLaunchTask`: Build & Run Release | `build.ps1 -Configuration Release -Run` |
| タスク: Build Debug（既定ビルド）          | `.vscode/tasks.json`                 | `build.ps1 -Configuration Debug`        |
| タスク: Build Release               | 同上                                   | `build.ps1 -Configuration Release`      |
| タスク: Clean Debug / Clean Release | 同上                                   | `build.ps1 ... -Clean`                  |


設定ファイル:

- `.vscode/launch.json` … 実行構成（上記 Run Only）
- `.vscode/tasks.json` … `build.ps1` を呼び出すシェルタスク

---

## よくある注意点

- **座標は必ず** `SCREEN_WIDTH/HEIGHT` **基準**。`DRAW_SCREEN_`* を位置計算に使わない。
- **2D / Font は** `Draw()` **単体で完結**。2D 前に `SetDepthEnable(false)`。
- **3D は** `SetDepthEnable(true)`**、2D 前に** `false`。ビューポートもこれに連動。
- `Fade_Draw()` **は他 UI より前面**（`main` が Present 直前に呼ぶ）。
- **ウィンドウリサイズ仕様**は [direct3d_viewport_resize_spec.md](direct3d_viewport_resize_spec.md)（実装は `shader/renderer.cpp`）。
- `.h/.cpp` **は UTF-8 BOM 付き**、`.hlsl` **は UTF-8 BOM なし**。編集後は `python tool/encoding_converter.py` を実行する。
- `new` **したポインタの解放は** `SAFE_DELETE`**（**`delete` **直書きや二重解放を避ける）。**
- **COM（**`ID3D11*` **等）の解放は** `SAFE_RELEASE`**（**`Release()` **直書きや二重解放を避ける）。**

