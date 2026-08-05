# MustDice フローチャート

元図: [`kaihatsu_resorce/mustdiceフローチャート.png`](../kaihatsu_resorce/mustdiceフローチャート.png)

配置の目安（元図どおり）:
- 左列: ピンポイント（YES）
- 右列: 奇数/偶数（NO）
- 中央下で両枝が合流 → ラウンド判定 → ショップ / ゲームオーバー
- 上へ戻る矢印はレイアウトを崩すため、戻り先をノード文言で示す

```mermaid
flowchart TB
    Title[タイトル]
    Title --> ToGame[ゲームに遷移]
    ToGame --> GameView[ゲーム画面表示]
    GameView --> RoundStart[ラウンド開始<br>目標スコア提示]
    RoundStart --> BetSelect[賭け方選択]
    BetSelect --> IsPinpoint{賭け方<br>ピンポイント？}

    %% 左: ピンポイント
    IsPinpoint -->|YES| PickA[賭ける数字選択<br>賭ける数字「A」]
    PickA --> RollB[サイコロを振る<br>サイコロの数値「B」]
    RollB --> DiffCheck{AとBは異なる？}
    DiffCheck -->|YES| Adjust[A = Bになるように<br>Aの数字を +1 または -1<br>倍率 -0.1倍<br>※異なる間は繰り返す]
    DiffCheck -->|NO| PinScore[スコアの素点に倍率をかける<br>素点は100点<br>確率の中央値を1.0倍<br>そこから1つ離れるほど +0.1倍]
    Adjust --> PinScore
    PinScore -.-> MultTable[倍率表は about_game.md 参照]

    %% 右: 奇数/偶数
    IsPinpoint -->|NO| OddEven[賭け方「奇数/偶数」<br>合計の奇偶を当てる賭け方]
    OddEven --> PickOE[奇数か偶数か選択]
    PickOE --> RollOE[サイコロを振る]
    RollOE --> HitCheck{的中した？}
    HitCheck -->|YES| HitScore[スコアの素点に 1.2倍<br>素点は100点]
    HitCheck -->|NO| MissScore[スコアの素点に 0.6倍<br>素点は100点]

    %% 合流〜ラウンド内ループ判定
    PinScore --> AddScore[スコア算出し<br>ラウンドの合計スコアに加算<br>賭けた数 +1]
    HitScore --> AddScore
    MissScore --> AddScore
    AddScore --> BetCount{賭けた数が3を超えた？}
    BetCount -->|NO| BackBet[賭け方選択へ戻る]
    BetCount -->|YES| GoalCheck{ラウンドの合計スコアが<br>目標スコア以上？}

    %% 勝敗・ショップ（最下部）
    GoalCheck -->|NO| GameOver[ゲームオーバー<br>リザルト画面に遷移<br>※タイトルへ戻る]
    GoalCheck -->|YES| Shop[お金を獲得<br>ショップに入る]
    Shop --> Spend[お金を消費し<br>サイコロの数字変化・アーティファクト購入<br>ダイス面のメッキなどを行う<br>次ラウンドの目標スコアは少し高くなる<br>※ラウンド開始へ戻る]
```
