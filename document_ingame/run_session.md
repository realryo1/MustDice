# RunSession（ラン共有状態）

## 概要

`app/run_session.h` / `app/run_session.cpp` は、**シーンをまたいで残るラン進行データ**を保持する。

- 倍率・出目判定などの**純計算は持たない**（`app/bet_logic` が担当）
- Reset / BeginRound / スコア適用 / クリア報酬 / ゲッター・セッター
- `SCENE_TITLE` / `SCENE_GAME` / `SCENE_SHOP` / `SCENE_RESULT` から参照する

画面遷移は従来どおり `SetSceneFade`。ラウンド内の手順は `SCENE_GAME` の `GamePhase` が担当する。

## フィールド

| 名前 | 意味 |
|------|------|
| `roundIndex` | 現在ラウンド番号（`BeginRound` で +1） |
| `targetScore` | そのラウンドの目標スコア |
| `roundScore` | ラウンド合計スコア |
| `betCount` | 今ラウンドの賭け回数（上限 3） |
| `money` | 所持金（クリア報酬で増加。ショップ購入はα未実装） |

将来のダイス配列・アーティファクトなどは、ここに載せるか別モジュールに分ける。

## API

| 関数 | タイミング |
|------|------------|
| `RunSession_Reset()` | タイトル開始時、リザルトからタイトルへ戻る時 |
| `RunSession_BeginRound()` | 新規ラウンド開始（Title→Game 初回、Shop→Game 前） |
| `RunSession_ApplyBetScore(int score)` | 1回の賭けスコアを加算し `betCount++` |
| `RunSession_GrantClearReward()` | 目標達成時。`money += roundScore / 10` |
| `RunSession_IsTargetMet()` | `roundScore >= targetScore` |
| `RunSession_IsBetLimitReached()` | `betCount >= 3` |
| `RunSession_Get*` / `Set*` | 表示・進行判定 |

## シーンとの関係

```
TITLE  --Reset-->  GAME(BeginRound if needed)
GAME   --クリア + GrantClearReward-->  SHOP  --BeginRound-->  GAME
GAME   --敗北-->  RESULT  --Reset-->  TITLE
```

## 関連

- ルール正本: [about_game.md](about_game.md)
- 賭け計算: `app/bet_logic.h` / `app/bet_logic.cpp`
