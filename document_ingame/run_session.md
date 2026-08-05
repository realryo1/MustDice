# RunSession（ラン共有状態）

## 概要

`app/run_session.h` / `app/run_session.cpp` は、**シーンをまたいで残るラン進行データ**を保持する。

- 計算ロジック（賭け判定・倍率・乱数など）は持たない
- Reset / BeginRound / スタブ加算 / ゲッター・セッターのみ
- `SCENE_TITLE` / `SCENE_GAME` / `SCENE_SHOP` / `SCENE_RESULT` から参照する

画面遷移は従来どおり `SetSceneFade`。ラウンド内の手順は `SCENE_GAME` の `GamePhase` が担当する。

## フィールド

| 名前 | 意味（α） |
|------|-----------|
| `roundIndex` | 現在ラウンド番号（`BeginRound` で +1） |
| `targetScore` | そのラウンドの目標スコア（プレースホルダ） |
| `roundScore` | ラウンド合計スコア（スタブ加算用） |
| `betCount` | 今ラウンドの賭け回数（上限 3） |
| `money` | 所持金（ショップ連携用枠。αでは未使用に近い） |

将来のダイス配列・アーティファクトなどは、ここに載せるか別モジュールに分ける。現時点では未定義。

## API

| 関数 | タイミング |
|------|------------|
| `RunSession_Reset()` | タイトル開始時、リザルトからタイトルへ戻る時 |
| `RunSession_BeginRound()` | 新規ラウンド開始（Title→Game 初回、Shop→Game 前） |
| `RunSession_AddBetStub()` | 賭け1回分の固定点加算スタブ（本ロジック置き換え予定） |
| `RunSession_Get*` / `Set*` | 表示・進行判定 |
| `RunSession_IsBetLimitReached()` | `betCount >= 3` |

## シーンとの関係

```
TITLE  --Reset-->  GAME(BeginRound if needed)
GAME   --クリア stub-->  SHOP  --BeginRound-->  GAME
GAME   --敗北 stub-->  RESULT  --Reset-->  TITLE
```

## 拡張ポイント

- `AddBetStub` を本スコア計算に置き換える
- `money` をショップ購入・ラウンド報酬と接続する
- ダイス／アーティファクト状態を追加する場合は、このモジュールか隣接の `ingame` 系に分離する
