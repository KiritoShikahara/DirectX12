# DECISIONS

長期的な設計・アーキテクチャに影響する決定のみを追記する。追記専用（削除・上書きしない）。

## 2026-07-15 進捗管理を claude/ フォルダに集約
- 内容: セッション間の引き継ぎ情報を `claude/STATUS.md`（現状スナップショット）、`claude/TASKS.md`（タスク一覧）、`claude/DECISIONS.md`（設計判断ログ）の3ファイルに分離して管理する運用に決定。
- 理由: 再開時にリポジトリ全体やgit履歴を読み込むとコンテキスト消費が大きいため、要点のみをまとめた小さいファイル群を先に読むことで復旧コストを最小化する。
- 運用ルールは `AGENTS.md` に記載。

## 2026-07-15 ゲームのコアループの方向性を確定（GAME_DESIGN.md）
- 内容: 3Dウェーブサバイバル型(Vampire Survivors系)、世界観はPOE2風ダークファンタジー(素材は仮)、
  カメラはPOE2風の急俯瞰アイソメトリック、攻撃は遠距離主体でパークにより武器/魔法を獲得、
  敵は時間経過で連続強化され一定時間でボス出現、制限時間生存でクリア、と方向性を確定。
- 理由: ユーザーへのヒアリングにより決定。既存コードの`WeaponInventoryComponent`(最大6スロット、
  SingleShot/SelfDefense/AreaAttack)や`GameState::PerkSelect`ステートの設計と整合する。
- 開発方針: コアループの完成を最優先し、武器種類の拡充・複数ステージ対応は後回しにする
  (拡張しやすい設計は意識しつつ、まず最小構成で成立させる)。
- 詳細・現状の実装状況は `GAME_DESIGN.md` を参照。

## 2026-07-15 新規C++ファイルはBOM付きUTF-8で保存する
- 内容: `App/src` に日本語コメントを含む新規ヘッダ/cppをBOM無しUTF-8で保存したところ、
  MSVCがファイルをコードページ932(Shift-JIS)として誤解釈し、コメント内の日本語バイト列が
  偶然バックスラッシュ相当のバイトを生成してマクロの行継続と誤認され、
  `REFLECT_FIELD_*`マクロの引数が欠落する等の意味不明なコンパイルエラーが連鎖的に発生した
  （例: `data::SingleShotWeaponData`の一部フィールドだけ「メンバーではない」エラーになった事案）。
  対象ファイルにUTF-8 BOMを付与したところ解消した。
- 理由: 既存ファイルは全てBOM無しでC4819警告(コードページで表示できない文字を含む)が出続けている
  状態だが、警告止まりで実害が出ていなかっただけ。日本語コメントの内容やバイト境界次第では
  今回のように実際のコンパイルエラーに発展しうる。
- 対応方針: 新規に作成するC++ファイル(.h/.cpp)は必ずUTF-8 BOM付きで保存する。
  既存ファイルは動いているため触らない（不要な変更をしないというGitルールに従う）。

## 2026-07-16 既存のBOM無しファイルはUTF-8とShift-JISが混在している
- 内容: BOM無しの既存ファイルは全て同じ文字コードだと思い込みがちだが、実際には
  UTF-8のもの(例: `PlayerStatusComponent.h`、`CoreEngine/.../InputManager.cpp`)と
  Shift-JIS(CP932)のもの(例: `PlayerContactDamageSystem.cpp`)が混在している。
  既存のBOM無しファイルへ日本語コメントを追記する際、そのファイルの実際の文字コードを
  確認せずにUTF-8の日本語文字列をそのまま挿入すると、Shift-JISファイルの場合は
  同一ファイル内でエンコーディングが混在した状態になり、DECISIONS.md記載の過去の
  コンパイルエラー事案と同種の問題を再発しうる。
- 対応方針: 既存のBOM無しファイルへ日本語コメントを追記する前に、そのファイル内の
  既存の日本語コメントの一部をUTF-8前提でgrep検索し、ヒットすれば UTF-8（そのまま追記可）、
  ヒットしなければ Shift-JIS の可能性が高いと判断し、追記する内容は英語(ASCII)のみに留める。

## 2026-07-16 Effekseerの`.efkproj`と`.efk`は別形式（このゲームは`.efk`のみ読める）
- 内容: Effekseerには2種類のファイルが存在する。`.efkproj`はEffekseer **Editor**の
  プロジェクトファイル(実体はXML、`<?xml version="1.0"...?>`から始まる)で、編集用の
  中間形式。`.efk`はコンパイル済みのバイナリ実行時パッケージ(先頭4バイトが"SKFE")で、
  `Effekseer::Effect::Create()`(このプロジェクトの`EffekseerManager::GetEffect()`が
  内部で呼ぶ)はこちらしか読み込めない。`CoreEngine/external/Effekseer`はランタイムSDKのみで
  エディタは含まれないため、`.efkproj`→`.efk`の変換はEffekseer Editor(GUIアプリ)での
  エクスポート操作が必要。CLIツールやこのプロジェクト内蔵の変換手段は無い。
- 経緯: ユーザーが`.efkproj`形式のEffekseerサンプル素材一式を用意してくれたが、
  上記の理由でこのゲームにそのまま組み込めなかった（サンドボックス環境にはGUI操作・
  変換ツールが無いため）。
- 対応方針: `.efk`(コンパイル済み)以外のEffekseer素材ファイルを受け取った場合は、
  そのままでは使えないことをユーザーに伝え、Effekseer Editorでのエクスポートを依頼する。
  エクスポート済み`.efk`(+参照テクスチャ・モデル)であれば通常通り`App/Assets/Effect/`へ
  配置して使用できる。

## 2026-07-16 プロジェクト内蔵のEffekseerランタイムはファイル形式バージョン1.71までしか読めない
- 内容: ユーザーがEffekseer Editorで`.efk`としてエクスポートした素材(`hougu_pre.efk`/
  `hougu_main.efk`等)が、`EffekseerManager::GetEffect()`(内部の`Effekseer::Effect::Create()`)で
  読み込み失敗した(`Failed to load effect`)。バイナリの先頭"SKFE"直後4バイトに埋め込まれている
  ファイル形式バージョンを確認したところ、これらの新規ファイルは`1810`(v1.81相当)だったのに対し、
  既存の動作確認済みファイル(Fire3.efk等)は`1710`(v1.71相当)だった。
  `CoreEngine/external/Effekseer/.../Effekseer.EffectImplemented.h`の
  `SupportBinaryVersion = Version17(=1710)`が上限で、`Effekseer.Effect.cpp`の
  `if (m_version > SupportBinaryVersion) return false;`(コメント「too new version」)により、
  1710を超えるバージョンのファイルは明示的に読み込み拒否される仕様と判明した。
  つまりこのプロジェクトに同梱されているEffekseerランタイムSDKは、ユーザーの手元の
  Effekseer Editorより古いバージョンで、新しいEditorが吐き出すファイル形式を理解できない。
  `EffectObject::Play()`は`effect == nullptr`を安全にスキップする作りのためクラッシュはしないが、
  該当エフェクトは無音無表示のまま何も起きない(かつ`PlayerUltimateSystem`のビーム再生終了待ちは
  `IsPlaying()`が常にfalseになるため即座に終了扱いになり、演出が一瞬で終わったように見える)。
- 対応状況: `App/Assets/Data/Ultimate/UltimateData.csv`は一旦、動作確認済みのプレースホルダー
  (`Tornade.efk`/`Blow2.efk`)へ戻した。`hougu_pre.efk`/`hougu_main.efk`とその依存テクスチャは
  `App/Assets/Effect/`に既に正しく配置済み(依存関係は全て揃っている)ため、バージョン問題さえ
  解決すれば設定を差し替えるだけで使える状態。
- 今後の対応候補（ユーザー判断待ち）:
  1. `CoreEngine/external/Effekseer`のランタイムSDKをv1.81以降が読めるバージョンへ更新する
     (根本解決だが、同梱ライブラリの更新のためAPI差分の調査・既存ラッパーコード
     (`EffectManager.cpp`/`EffectObject.cpp`等)の追従・全体の再ビルド確認が必要な大きめの変更)
  2. ユーザー側でEffekseer Editorの古いバージョン(1.71以前を書き出せるもの)を使って
     再エクスポートしてもらう(手元にあれば最も手軽)
- **追記(2026-07-16)**: ユーザーが対応2を選択し、両ファイルを再エクスポート。1回目の結果は
  `hougu_pre.efk`→バージョン1710(対応済み)、`hougu_main.efk`→バージョン1810のまま
  (未対応)と、ファイルによって結果が分かれた。`hougu_pre.efk`のみ先に`UltimateData.csv`の
  `BeamEffectPath`へ導入。その後ユーザーがEffekseer Editor側の設定を見直して`hougu_main.efk`を
  再々エクスポートし、こちらもバージョン1710に収まった。`ActivationEffectPath`へ導入し、
  必殺技の専用エフェクトが両方とも揃った。バイナリのバージョンバイトは
  `[System.BitConverter]::ToInt32($bytes, 4)`(先頭4バイトが"SKFE"、続く4バイトがバージョン)
  で機械的に確認できる。今後もユーザーがEffekseer素材を追加する際は、エクスポート直後に
  この方法でバージョンを確認してから導入するのが安全。

## 2026-07-16 ゴールド強化システムの永続化はConfigManager<T>(JSON)、マスタデータはDataManager<T>(CSV/DB)と使い分ける
- 内容: プレイヤーの所持ゴールド・ステータス強化レベル(`data::PlayerSaveData`)は、
  プレイの結果によって書き換わる「セーブデータ」そのものであり、CSVを正として
  Debug/Releaseで挙動を分けるマスタデータ(`data::DataManager<T>`、CSVはDebug、
  DBはRelease)の仕組みとは性質が異なる。既存の`WindowConfig`が使っていた
  `data::ConfigManager<T>`(JSONファイルへの自動保存/読込、`data::ConfigRegistry`)を
  そのまま流用し、`Assets/Bin/Save/player_save.json`へDebug/Release共通で永続化する
  設計にした。強化のコスト・効果量・レベル上限(`data::StatUpgradeData`)は逆に
  「開発者が調整するバランスデータ」であるため、既存のCSV/DBパターンをそのまま使う。
- 対応: `data::EnsurePlayerSaveDataLoaded()`(inline関数)で二重登録を防ぎつつ、
  GameSceneFactory/EnemyDeathSystem/StatusUpgradeInputSystem/PlayerSaveDebugPanel等
  複数箇所から安全に参照できるようにした。

## 2026-07-16 Write toolによる既存ファイルの全文上書きはBOMを再度失わせる
- 内容: 新規作成時にBOMが付かない問題(既存メモリ参照)は把握済みだったが、
  「既にBOM付きで存在するファイル」をWrite toolで全文書き換えた場合も、BOMが
  再度失われることが判明した。`StatusUpgradeScene.cpp`/`StatusUpgradeInputSystem.cpp`
  をプレースホルダーから本実装へ置き換えた際に発生。日本語wstringリテラル
  (`L"ステータス強化"`等、コメントではなく実行コード中の文字列)がコードページ932として
  誤解釈され、`error C2001: 定数が2行目に続いています`や`'dataRegistry': 定義されていない
  識別子です`といった、実際の原因(エンコーディング)とは無関係に見える連鎖的な構文エラーが
  ファイルの途中(最初の日本語文字列付近)から大量に発生した。
- 対応: PowerShellで該当ファイルのBOMを再確認し、無ければ
  `New-Object System.Text.UTF8Encoding $true`で再書き込みして解決した。
- 今後の運用: Write toolで.h/.cppを書く場合（新規作成・既存ファイルの全文上書き問わず）は、
  必ず直後にBOMバイト(`ef bb bf`)を確認する。Edit toolによる部分編集は既存のバイト列を
  保持するため対象外（新規ファイルはWriteでしか作れないため必ず要確認）。

## 2026-07-17 排他的な演出中フラグはIsPlayerActionLocked()に一本化する
- 内容: 必殺技(Ultimate)発動中は他の武器の発動・プレイヤー操作・パーク選択への遷移を
  一時停止するため`ecs::IsPlayerUltimateActive(registry)`というガードを15箇所
  (各武器System、GameStateSystem等)で個別に呼んでいた。Flicker Strike(2つ目の同種の
  排他スキル、ワープ攻撃シーケンス中も同様に他を止める必要がある)を追加するにあたり、
  同じガードをもう1系統分15箇所へ重複して書き並べるのではなく、
  `ecs::IsPlayerActionLocked(registry)`(`App/src/system/Player/PlayerActionLock.h`、
  内部で`IsPlayerUltimateActive() || IsPlayerFlickerStrikeActive()`を判定)へ統合し、
  該当15箇所全ての呼び出しをこちらへ差し替えた。
- 理由: 排他スキルが2つ以上になった時点で個別ガードの並記は重複コードであり、
  3つ目が増えるたびに15箇所以上への追記が発生し続けるのは保守性を損なう。
  各武器/状態System側は「今、排他的な演出中かどうか」だけを知っていればよく、
  具体的にどのスキルが原因かを知る必要はない(疎結合)。
- 対応方針: 今後3つ目以降の同種の排他スキル(プレイヤー操作をロックし他の攻撃を止める演出)を
  追加する場合、各Systemへ個別のガードを追加するのではなく、
  `IsPlayerActionLocked()`の内部条件に1行追加する形で対応すること。

## 2026-07-17 「近くの未処理の敵を1体探す」ロジックはEnemyTargetUtilに共通化する
- 内容: Chain Lightningが跳躍先(JumpRadius内の未命中の敵)を探すために持っていた
  private staticメソッド`FindNearestExcluding`を、`App/src/system/Enemy/EnemyTargetUtil.h/.cpp`
  (`ecs::targetutil::FindNearestExcluding`)へ切り出した。Flicker Strikeのワープ先探索
  (直前の対象を除いた近くの敵)でも全く同じロジックが必要になったため。
- 対応方針: 「候補のうち、特定の除外リストに含まれない最も近い敵を1体選ぶ」系の
  ロジックが新たに必要になった場合はこのユーティリティを使うこと。なお
  `HomingMissileSteeringSystem::FindNearestEnemy`(除外リスト無しの単純な最近接探索、
  Homing/VoidBeam/BoneSpearが利用)はまだ統合していない(役割が微妙に異なり、
  統合の効果に対して影響範囲が広がるため保留。4つ目の類似ロジックが必要になった
  タイミングで統合を検討する)。

## 2026-07-17 JsonSerializer::SaveToFileは保存前に親ディレクトリを作成する
- 内容: `ConfigManager<T>::Save()`(`PlayerSaveData`が使用)が、保存先の`Assets/Bin/Save/`
  フォルダが存在しない状態で呼ばれ、`std::ofstream`が開けず`std::runtime_error`が
  投げられてデバッガでブレークする事案が発生した。`std::ofstream`は中間ディレクトリを
  自動作成しないため、フォルダが誤って削除された場合や新規環境への配置直後は
  必ず再発する。`ConfigManager<T>`を使う箇所全て(将来の追加設定含む)に共通する問題のため、
  個別のセーブデータ側ではなく`JsonSerializer::SaveToFile()`
  (`CoreEngine/src/Data/Storage/Loader/JsonSerializer.h`)側で
  `std::filesystem::create_directories()`により親ディレクトリを保存前に作成するよう修正した。
- 対応: 修正後、実際に`Assets/Bin/Save/`フォルダを削除した状態から起動→Save()を実行させ、
  フォルダ・ファイルが自動生成されエラーが出ないことを確認済み。

## 2026-07-16 EnTTのview.each()は空のタグ型にはコールバック引数を渡さない
- 内容: `ecs::StatusUpgradeGoldUiTag`のようにフィールドを持たない空のタグ構造体を
  `registry.view<EmptyTag, TextComponent>().each([&](const EmptyTag&, TextComponent&){...})`
  のように2引数のラムダで受けようとすると、`std::invoke`のオーバーロード解決に失敗して
  `error C2672: 'invoke': 一致するオーバーロードされた関数が見つかりませんでした`という
  分かりにくいテンプレートエラーになる。EnTTは「空の型(データを持たないコンポーネント)」を
  ストレージ上で特別扱いし、`.each()`のコールバックへその型の引数を渡さない
  (empty型最適化)ため。`ecs::PerkOptionUiTag`/`ecs::HubMenuOptionUiTag`のように
  `int OptionIndex`等のフィールドを持つタグは通常通り渡される。
- 対応: 空のタグ型と一緒にviewする場合、コールバックの引数はそのタグ型を含めず
  (例: `[&](TextComponent& text){...}`)、他の非空コンポーネントの型だけを受け取るようにする。
