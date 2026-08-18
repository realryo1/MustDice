# MustDice マルチ対戦仕様（最小実装版）

## 1. 目的

既存の MustDice のダイス賭け・スコア計算を流用し、最大4人でラウンドごとのスコア順位を競う対戦モードを実装する。

この初期版では、HP、攻撃、ターゲット選択、特殊ダイス、ショップ、相手妨害は扱わない。各プレイヤーが同じ条件でダイス賭けを行い、ラウンドごとの順位に応じた対戦ポイント（`matchPoint`）を獲得する。

- 最大人数：4人（最小2人）
- 試合ラウンド数：5
- 1ラウンドの賭け回数：3回
- 勝者：5ラウンド終了時点の累積対戦ポイント1位
- 通信：固定IP VPS上の専用サーバーへクライアントが接続
- クライアント側ポート開放：不要
- 想定ネットワーク実装：standalone Asio / Boost.Asio

## 2. 既存ルールの流用

ソロ版の以下のダイス・得点ルールをそのまま対戦で使用する。

- 2個の6面ダイス（2d6）を振り、合計値は2〜12
- 1回の賭けにつき、プレイヤーはA役またはB役を選択する
- A役は合計値をピンポイント予想する
- B役は出目合計の奇数／偶数を予想する
- 各プレイヤーの1ラウンド内の3回分の得点を `roundScore` として合計する

### A役：ピンポイント

プレイヤーは予想合計値 `A`（2〜12）を選択し、実際の合計値 `B` をサーバーが確定する。

- 素点：100
- 倍率：出やすい中央の7を1.0倍とし、両端ほど高倍率
- 外れた場合：予想 `A` と実出目 `B` の差1ごとに倍率を0.1減らす

```text
score = round(100 × (mult(A) − 0.1 × abs(A − B)))
```

倍率表：

| 予想合計 | 倍率 |
|---:|---:|
| 2 / 12 | 1.5x |
| 3 / 11 | 1.4x |
| 4 / 10 | 1.3x |
| 5 / 9 | 1.2x |
| 6 / 8 | 1.1x |
| 7 | 1.0x |

### B役：奇数／偶数

プレイヤーは奇数または偶数を選択する。

| 結果 | 得点 |
|---|---:|
| 的中 | 100 × 1.2 = 120 |
| 外れ | 100 × 0.6 = 60 |

## 3. 試合ルール

### ラウンドの進行

1. 全員がラウンド開始状態になる
2. 各プレイヤーは1回目の賭け内容を選択・送信する
3. 全員の入力が揃う、または制限時間になる
4. サーバーが各プレイヤーのダイス結果と得点を確定する
5. 全クライアントでダイス演出・得点演出を再生する
6. 2回目、3回目も同様に処理する
7. 3回の賭け終了後、各プレイヤーの `roundScore` を比較して順位を確定する
8. 順位に応じて `matchPoint` を加算する
9. 5ラウンド未満なら次ラウンドを開始する
10. 5ラウンド終了後、累積 `matchPoint` の順位で試合結果を確定する

### 順位ポイント

| ラウンド順位 | 獲得対戦ポイント |
|---|---:|
| 1位 | 4点 |
| 2位 | 3点 |
| 3位 | 2点 |
| 4位 | 1点 |

2人対戦では1位4点、2位3点のみを使用する。3人対戦では1位4点、2位3点、3位2点を使用する。

### 同点処理

ラウンド順位が同点の場合、該当順位に割り当てられたポイントを等分する。

例：1位が2人同点の場合、1位と2位のポイント合計 `4 + 3 = 7` 点を等分し、それぞれ3.5点とする。

小数を使わないため、内部的には対戦ポイントを100倍した整数 `matchPointX100` として保持する。

- 4点：400
- 3点：300
- 2点：200
- 1点：100
- 3.5点：350

最終結果で `matchPointX100` が同点の場合は、次の順に比較する。

1. ラウンド1位の獲得回数が多い
2. 全ラウンドの `roundScore` 合計が高い
3. それでも同値なら同率順位

### 入力時間と切断

- 1回の賭けの入力制限時間：15〜20秒を想定
- 全員が送信済みなら、制限時間を待たずに即時解決する
- 時間切れのプレイヤーは自動選択とする
- 自動選択の初期仕様：B役（奇数／偶数）をランダム選択する
- 切断中のプレイヤーも当面は自動選択として試合を継続する
- 再接続時はサーバーから最新の試合状態スナップショットを送る

## 4. フローチャート

```mermaid
flowchart TB
    Title[タイトル]
    Title --> Lobby[ロビー作成 / 入室]
    Lobby --> WaitReady{2〜4人が参加し<br>全員Ready？}
    WaitReady -->|No| Lobby
    WaitReady -->|Yes| MatchStart[試合開始<br>matchPointを初期化]

    MatchStart --> RoundStart[ラウンド開始<br>roundScore・betCountを初期化]
    RoundStart --> BetInput[全員が賭け内容を選択<br>A役: 合計2〜12 / B役: 奇数・偶数]
    BetInput --> WaitInput{全員送信済み<br>または時間切れ？}
    WaitInput -->|No| BetInput
    WaitInput -->|Yes| AutoSelect[未送信者を自動選択]

    AutoSelect --> ServerRoll[サーバーがダイス結果を確定]
    ServerRoll --> ResolveA{選択した役はA役？}

    ResolveA -->|Yes| CalcA[A役スコア計算<br>100 × (mult(A) - 0.1 × abs(A-B))]
    ResolveA -->|No| CalcB[B役スコア計算<br>的中120 / 外れ60]

    CalcA --> ApplyScore[各プレイヤーのroundScoreへ加算<br>betCountを+1]
    CalcB --> ApplyScore
    ApplyScore --> Broadcast[確定結果を全員へ送信<br>ダイス・得点演出]

    Broadcast --> BetEnd{betCountが3回に到達？}
    BetEnd -->|No| BetInput
    BetEnd -->|Yes| RankRound[roundScoreでラウンド順位を決定]

    RankRound --> AddPoints[順位ポイントを加算<br>1位:4 / 2位:3 / 3位:2 / 4位:1<br>同点はポイント等分]
    AddPoints --> RoundResult[ラウンド結果画面]
    RoundResult --> MatchEnd{5ラウンド終了？}
    MatchEnd -->|No| RoundStart
    MatchEnd -->|Yes| FinalRank[累積matchPointで最終順位を決定]
    FinalRank --> Result[リザルト画面]
    Result --> Title
```

## 5. サーバー権威設計

対戦結果をクライアントで確定しない。VPS上の専用サーバーを唯一の正とし、クライアントは入力送信と結果表示を担当する。

### サーバーが持つ責務

- ルーム作成・参加・Ready状態
- プレイヤーIDと接続状態の管理
- ラウンド数、賭け回数、タイムアウトの管理
- 全プレイヤーの賭け内容の受付と入力締切
- ダイス乱数、出目、得点、順位、対戦ポイントの確定
- 試合状態スナップショットの配信
- 切断・再接続処理

### クライアントが持つ責務

- 賭け内容のUI入力
- サーバーへ入力送信
- サーバー確定済みのダイス結果・得点・順位の表示
- ダイスロール、得点加算、順位表の演出

### 乱数の扱い

- ダイスの出目はサーバーだけが生成・確定する
- クライアントが送るのは「選択した役」と「予想値」のみ
- クライアントから送られたダイス結果やスコアは信用しない
- 必要になれば、ターン開始時にサーバーシードのハッシュを公開し、解決時にシードを開示する方式を追加する

## 6. 通信構成

```text
DirectX クライアント（最大4人）
        │
        │ TCP 接続
        ▼
固定IP VPS 上の専用サーバー
  - standalone Asio / Boost.Asio
  - ルームと試合状態
  - 入力受付・タイムアウト
  - 乱数・得点・順位の確定
```

プレイヤーのクライアントはVPSへ外向きにTCP接続するだけでよいため、プレイヤー側のポート開放は不要。VPS側でゲームサーバー用のTCPポートだけを開放する。

ターン制・最大4人・低頻度メッセージのため、初期版はTCPのみで十分とする。UDP、ENet、P2P、NAT越え、リレーサーバーは必要になってから検討する。

## 7. 最小データ設計

ソロ用の `RunSession` とは分離し、マルチ対戦専用の状態をサーバー側に定義する。

```cpp
constexpr int kMaxPlayers = 4;
constexpr int kBetsPerRound = 3;
constexpr int kTotalRounds = 5;

struct PlayerMatchState {
    int playerId = -1;
    int roundScore = 0;
    int totalRoundScore = 0;
    int betCount = 0;
    int matchPointX100 = 0;
    int firstPlaceCount = 0;
    bool submitted = false;
    bool connected = false;
};

struct MatchState {
    int currentRound = 1;
    int currentBet = 1;
    int playerCount = 0;
    std::array<PlayerMatchState, kMaxPlayers> players;
};
```

基本処理の責務を分ける。

```cpp
void BeginMatch(MatchState& match);
void BeginRound(MatchState& match);
void SubmitBet(MatchState& match, int playerId, const BetRequest& request);
void ResolveCurrentBet(MatchState& match);
void ResolveRoundRanking(MatchState& match);
void AddRoundRankPoints(MatchState& match);
bool IsRoundFinished(const MatchState& match);
bool IsMatchFinished(const MatchState& match);
```

既存の賭け計算はネットワーク状態を持たない純粋な計算として維持する。

```cpp
int CalculatePinpointScore(int predictedSum, int rolledSum);
int CalculateOddEvenScore(bool predictedOdd, int rolledSum);
```

## 8. 初期実装の範囲

### 実装する

- 2〜4人ロビー
- Readyと試合開始
- 5ラウンド
- 1ラウンド3ベット
- A役・B役の既存得点計算
- 同時提出・一斉解決
- タイムアウト時の自動選択
- ラウンド順位と4 / 3 / 2 / 1ポイント
- 同点時のポイント等分
- 累積ポイントによる最終結果
- VPS上の専用サーバー
- standalone Asio または Boost.Asio によるTCP通信

### 後回しにする

- HP、攻撃、ターゲット選択
- 特殊ダイス、ダイス構築、アーティファクト
- ショップ、所持金、ラン進行
- 相手への妨害、バフ、デバフ
- 観戦、リプレイ、ランキング
- P2P接続、UDP、NATパンチスルー
- シード公開による乱数検証
