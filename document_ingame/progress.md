# MustDice 進捗リスト

最終更新の目安: α（テキスト賭けループが遊べる状態／安定化一通り済み）

凡例: `[x]` 完了 / `[ ]` 未着手・α対象外

## シーン・骨格

- [x] TITLE / GAME / SHOP / RESULT のシーン構成
- [x] フェード遷移（`SetSceneFade`）
- [x] ラン共有状態（`run_session`）
- [x] GAME 内フェーズ状態機械
- [x] フェーズ戻り（Esc / X = `INPUT_ACTION_BACK`）
- [x] `BeginRound` を Game 入場時のみに一本化
- [x] フェード中の連打ガード（クリア報酬の二重加算防止など）
- [ ] ポーズ画面
- [ ] スレスパ風マップ移動

## コア賭けループ（α）

- [x] ピンポイント（予想合計 2〜12）
- [x] 奇数 / 偶数
- [x] 2d6 乱数ロール
- [x] 倍率・スコア計算（`bet_logic`）
- [x] ルール定数の明示（マジックナンバー整理）
- [x] 1ラウンド最大 3 回賭け
- [x] 出目確定時（RESOLVE 入場）にスコア加算
- [x] 目標スコア判定 → SHOP / RESULT
- [x] クリア報酬（`roundScore / 10`）
- [x] 次ラウンドで目標 +50
- [x] テキスト UI での進行表示
- [x] サイコロの3D表現（`dice.fbx`を物理挙動で転がし、投射時に組み立てた回転軌道と誤差時の小バウンドで2d6の個別出目へ着地、結果時は見下ろしカメラへ遷移）

## ショップ・強化

- [x] ショップ入場・所持金表示・次ラウンドへ
- [ ] ダイス面のメッキ
- [ ] ダイスの数字変化
- [ ] アーティファクト購入（例: ノーカンTシャツ）
- [ ] ダイスを増やす
- [ ] ショップでのお金消費 UI

## 特殊ダイス

- [ ] 4面ダイス
- [ ] 時計ダイス
- [ ] ステーキ
- [ ] 404ダイス
- [ ] その他案（バトル鉛筆など）

## 戦闘・表現（草案の将来要素）

- [ ] ターン制バトルとしての演出（攻撃力としての役）
- [ ] A役の「近いほど強い」戦闘表現（現状はスコア化済み）
- [ ] B役の強ヒット / カスダメージ演出

## マルチ対戦

- [x] シーン枠 `SCENE_MULTILOBBY` / `SCENE_MULTIGAME` / `SCENE_MULTIRESULT`（ソロ `game.cpp` は未改変）
- [x] Python 専用サーバー `server/mustdice_server.py` と `downloadstart.py`
- [x] タイトルでローカル / マルチ分岐、接続先は `option.yml`
- [x] ベットごとの出目は全員共通 2d6
- [x] 試合は3ラウンド
- [x] 4人未満を Bot で埋める（`FILLBOTS`）
- [x] 管理サイト `mustdice_admin.py`（GitHub 取得・アップロード・再起動・ログ、TCP 8777、Basic 認証）
- [ ] 実機での4人通しプレイ確認

## ドキュメント

- [x] [multiplayer.md](multiplayer.md)
- [x] [server.md](server.md)（VPS 構成・管理画面）
- [x] [about_game.md](about_game.md)（ルール正本・αルール追記済み）
- [x] [flowchart.md](flowchart.md)（about_game に合わせて最小整合）
- [x] [run_session.md](run_session.md)
- [x] 本進捗リスト
