# 武器システムのレジストリパターン

武器種別(`ecs::eWeaponType`)ごとに異なるRuntimeComponent型・マスタデータ型を、
巨大なswitch文に分散させず、**`std::unordered_map<eWeaponType, std::function<...>>`の
テーブル参照**に集約するパターンが3箇所ある。新武器を追加する際はこの3ファイルに
1行ずつ追記すればよい（他のswitch文を探し回る必要がない設計）。

## 1. WeaponTypeRegistry（既存、最初からあったパターン）

`App/src/system/Player/Weapon/WeaponTypeRegistry.h/.cpp`

`AddWeaponRuntimeComponent(registry, type, weaponEntity)`:
武器エンティティに、種別に対応する`XxxWeaponRuntimeComponent`を1つ付与する。
`GameSceneFactory::AddWeaponToPlayer`から呼ばれる。

## 2. WeaponCooldownRegistry（今回追加）

`App/src/system/Player/Weapon/WeaponCooldownRegistry.h/.cpp`

`TryGetWeaponCooldown(registry, weaponEntity, weapon, outRemaining, outMax)`:
武器の「残りクールダウン秒数」「最大クールダウン秒数(FireInterval×CooldownRate)」を
種別を意識せず取得する。所持武器バーのクールダウン表示(`WeaponIconBarSystem`)で使用。

各武器の`RuntimeComponent::CooldownTimer`は共通フィールド名（`Nova`だけ
マスタデータのフィールド名が`PulseInterval`で他は`FireInterval`という違いがあるので注意）。
`SelfDefense`(Orbit、常時稼働・クールダウン概念なし)は意図的にテーブル未登録
→ `TryGetWeaponCooldown`は`false`を返す。

`ecs::weaponutil::ComputeWeaponDataId(weapon)`（`WeaponUpdateUtil.h`）で
マスタデータの検索ID(`(WeaponID+1)*1000+Level`)を計算する共通ヘルパーがある。
新しいクールダウン取得コードを書くときは車輪の再発明をせずこれを使うこと。

## 3. WeaponIconRegistry（今回追加）

`App/src/system/Player/Weapon/WeaponIconRegistry.h/.cpp`

`GetWeaponIconPath(eWeaponType)`: 武器種別→表示アイコンのパス。
**パーク選択画面(`PerkSelectSystem`の新武器選択肢)と所持武器バー(`WeaponIconBarSystem`)の
両方がこの関数を共有**している。選択画面とバーでアイコンが食い違わないようにするための
一元化（以前は`PerkSelectSystem`内に重複したマッピングがあったので統合した）。

専用アイコン未作成の武器は`Assets/Icon/loading.png`を代用として返す。
新しいアイコン画像が`Assets/Icon/`に追加されたら、この関数のswitch文だけを
差し替えれば両方の表示に反映される。

## 新武器を追加する際のチェックリスト

1. `eWeaponType`に列挙値を追加（`WeaponInventoryComponent.h`）
2. マスタデータ構造体を追加（`App/src/Data/Weapon/XxxWeaponData.h`、CSV+DB両方）
3. `XxxWeaponRuntimeComponent`を追加
4. `XxxWeaponSystem`を追加（発動ロジック、`CooldownTimer`を`FireInterval*GetCooldownRate`でリセット）
5. `WeaponTypeRegistry.cpp`にRuntimeComponent登録を追記
6. `WeaponCooldownRegistry.cpp`にクールダウン取得を追記
7. `WeaponIconRegistry.cpp`にアイコンパスを追記
8. `PerkDefinition.h`の`GetPerkPool()`に`AcquireWeapon`の選択肢を追記
9. エフェクトは生パスではなく`EffectAssetData`のID経由で持たせる（下記参照）

## エフェクト素材のID管理（EffectAssetData）

以前は各武器マスタデータがエフェクトパスを`std::string`(例:"Assets/Effect/Fire3.efk")で
直接保持しており、Lv1〜MaxLevelの全レベル行に同じ文字列が重複格納されていた
(正規化されていない・パス変更時に全レベル行の書き換えが必要という問題があった)。

**新設計**: `App/src/Data/Effect/EffectAssetData.h`（テーブル`effect_assets`、
`Id: int PK, Path: string`）に素材を1つずつ登録し、各武器マスタデータは
文字列ではなく**数値ID**を持つ。

- **Id体系**: 4桁。上2桁=カテゴリ(01=Fire, 02=Lightning, 03=Light/汎用ヒット,
  04=近接武器, 05=Meteor, 06=Ultimate専用, 07=単体武器演出)、下2桁=カテゴリ内識別番号。
  カテゴリはCSV整理用の目安で、コード上カテゴリ値で分岐する処理は無い。
- **フィールド命名規約**: `XxxEffectPath`(生パス文字列、値そのまま) → `XxxEffectIds`
  (';'区切りの素材ID列、例:"202;203") にリネームした。**サフィックスで意味が変わる**:
  - `EffectPath`で終わる: 値はパス文字列そのまま（`data::UltimateData`はId=0の単一行
    しか持たずレベル重複問題が無いため、あえて変換せずこのまま）
  - `EffectIds`で終わる: 値はエフェクト素材ID列。使う前に必ず
    `ecs::effectutil::ResolveEffectIds(idsCsv)`でパス文字列へ解決すること
- **解決ヘルパー**: `ecs::effectutil::ResolveEffectIds()`（`EffectSpawnUtility.h/.cpp`）が
  ';'区切りのID列を`effect_assets`でルックアップし、';'区切りのパス文字列に組み立てて返す。
  各武器システムは`masterData`取得直後にこれを1回呼び、戻り値を**従来通り**
  `PlayOneShotCombined`/`GetEffect`/`ProjectileComponent::ExplosionEffectPath`等へ渡すだけでよい
  (エフェクト再生ロジック自体は無変更)。ループ内で複数回呼ぶと無駄なので、
  ループの外で1回だけ解決してローカル変数に保持すること(VoidBeamWeaponSystemが実例)。
- **自動プリロード**: `EffectPathPreloadVisitor`(`EffectSpawnUtility.h`)は
  `EffectPath`/`EffectIds`両方のサフィックスに対応済み。新しい`XxxEffectIds`フィールドを
  追加するだけで`GameScene::PreloadWeaponEffects()`が自動的に先読み対象にする
  (呼び出し側の追記は不要)。
- **DB更新の注意**: DBの列名変更は`ALTER TABLE xxx RENAME COLUMN old TO new;`で行った
  (このプロジェクトのSQLiteは3.53.2で`RENAME COLUMN`/`DROP COLUMN`とも対応済み)。
  CSV側はヘッダー行の列名とデータ行の値を両方変更する必要がある(`data_csv_db.md`参照)。
