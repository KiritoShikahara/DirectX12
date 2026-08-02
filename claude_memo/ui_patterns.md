# UI実装パターン

座標系はすべて仮想解像度(1920×1080、左上原点)。`Sprite`と`TextComponent`は同じ座標系
なので混在させて配置できる（`SpriteRenderer`/`TextRenderer`とも`Window::GetVirtualWidth/Height`
基準）。

## 生成とロジックの分離

このプロジェクトのUI実装は一貫して次の2層構造:
- **生成**: `GameSceneFactory::CreateXxx()` — エンティティとコンポーネントを1回だけ作る
- **更新**: 専用System（`XxxSystem::Update`毎フレーム） — 値を書き換えるだけ

動的な数（所持武器数など）を扱うUIでも、**最大数ぶんのスロットを最初に全部生成し、
空きスロットはIsVisible=falseにする**方式を採る（毎フレームのエンティティ生成/破棄を
避けるため）。`WeaponIconBarSystem`がこのパターンの実例。

## テキストの水平中央揃え

`graphics::TextRenderer::MeasureWidth(text, size)`でピクセル幅を実測し、
`text.X = centerX - width * 0.5f`で中央に揃える（`PerkSelectSystem`で最初に導入、
`WeaponIconBarSystem`でも同じ手法）。

**注意**: `TextComponent::X`は中央揃えのため毎フレーム書き換えられる。
「中心座標」自体を保持する場所が別途必要（`text.X`を書き換えた後の値を基準に
再計算すると、中心がズレ続けるバグになる）。`WeaponIconSlotTag::CenterX`のように
タグ側に不変の基準座標を持たせること。

## Sprite::FillAmount / FillType（ゲージ表現）

`FillType::Horizontal`: 左から右に満ちる（HPバー、必殺ゲージで使用）。
`FillType::Radial`: 12時位置から時計回りに満ちる/消える（円形クールダウンゲージ）。
シェーダー実装: `CoreEngine/Assets/Shader/Sprite/PS_Sprite.hlsl`の`ApplyFillClip`。

**クールダウンオーバーレイの表現**: 黒半透明のSprite(`Assets/Effect/Texture/White.png`を
着色して流用)を`FillType::Radial`にし、`FillAmount = 残り秒数/最大秒数`にする。
発動直後は1.0(アイコン全体を覆う)、時間経過で0へ近づき消えていく。
`GameSceneFactory::CreateWeaponIconBar` + `WeaponIconBarSystem`が実例。

## パーク選択画面 (PerkSelectSystem)

- カード = アイコン画像(大) + その下にテキスト、画面中心を基準に横並び均等配置
- 黒半透明ウィンドウ背景: `Assets/Effect/Texture/White.png`を`Color(0,0,0,alpha)`で着色、
  他の要素より奥のレイヤーに配置
- 背景にタイトル画面のSkybox/背景画像を薄く(`Color`のalphaを下げて)重ねる手法もある

## 所持武器バー (WeaponIconBarSystem)

- 配置: HPバーと必殺ゲージ(共にY=900付近)の間、画面中央下、横5×縦2グリッド
- 各スロット = 4エンティティ: `Icon`(アイコン本体) / `Overlay`(クールダウンRadial) /
  `Text`(残り秒数、中央) / `LevelText`(武器レベル、右下寄せ)。
  `WeaponIconSlotTag{SlotIndex, Element, CenterX}`で同じスロットの4要素を紐付ける
- `WeaponCooldownRegistry`/`WeaponIconRegistry`（`weapon_registries.md`参照）を使い、
  武器種別の分岐をロジック側に持たせない
- 位置・サイズ定数はレイアウト変更のたびにユーザーからの微調整依頼が入りやすい箇所
  （アイコンサイズ、間隔、Y座標、レベル表示の文字サイズ等）。`GameSceneFactory::CreateWeaponIconBar`
  冒頭にまとめてあるので、その定数だけ触れば済む

## HPバー・必殺ゲージ

- `FillAmountLerp`コンポーネント: 目標値へ滑らかに追従させる（撃破の度に必殺ゲージが
  なめらかに増える等）。`Target`を設定するだけで`PlayerHpBarSystem`/`PlayerUltimateGaugeSystem`が
  補間を行う
- 必殺ゲージは体力バーの画像を`Flip.x=-1`+右端基準座標で鏡像配置、`Color=Blue`で色分け
