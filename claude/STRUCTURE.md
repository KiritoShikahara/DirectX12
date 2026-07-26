# STRUCTURE

プロジェクトのフォルダ構成の索引。目的はセッション再開時に `find`/`Glob` で全体探索する
コストを省くこと。中身の詳細は各ファイルを読むこと。ビルド生成物 (`x64/`, `.vs/`, `bin/` 配下の
中間ファイル等) はここに載せない。

## ソリューション構成
- `CoreEngine/src` — エンジンコード
- `App/src` — ゲームコード
- `CoreEngine/Assets` — エンジン側アセット
- `App/Assets` — ゲーム側アセット
- `CoreEngine/external` — 自分でリンクしたライブラリ (JoltPhysics, EnTT, ImGui, Effekseer, DirectX12MA, json, mimalloc, miniaudio, SQLite, FastCSV 等のソース同梱ライブラリ)
- `packages/` — NuGetで取得したライブラリ (DirectXTex, D3D12, CppWinRT 等)
- `doc/` — プロジェクト全体のドキュメント
- `claude/` — AIセッションの引き継ぎ情報 (STATUS.md / TASKS.md / DECISIONS.md / STRUCTURE.md)

## CoreEngine/src
```
audio/          サウンド (BGM/SE/Device/Manager/Resource)
Config/         エンジン設定
Data/Storage/   汎用データストレージ
ecs/            EnTTベースECS (component / entity / system / Serialization)
Editor/         エディタ機能 (Parameter編集など)
graphics/
  Color/        カラー関連ユーティリティ
  ConstantBuffer/ 定数バッファラッパー
  Data/         描画用データ構造
  Dx12/         D3D12デバイス・コンテキスト (Dx12Device, Dx12Context = スワップチェーン/深度バッファ管理)
  Effect/       Effekseer連携 (EffectManager / EffectObject)
  Fbx/
    Animation/  FbxAnimSystem (スキニング/アニメーション再生)
    Data/       FbxData (頂点/マテリアル等の中間データ)
    Pipeline/   FbxPipeline (通常描画PSO), ShadowPipeline (シャドウパスPSO)
    Renderer/   FbxRenderer (描画実行、シャドウマップ生成)
    Resource/   FbxResource / FbxResourceManager (FBX読み込み・GPUリソース化)
  GraphicsDescriptorHeap/ ディスクリプタヒープ管理
  IndexBuffer/  インデックスバッファ
  Line/         デバッグライン描画
  PrimitiveModel/ プリミティブ形状モデル
  Shader/       ShaderManager (HLSLコンパイル/キャッシュ)
  Shape/        図形描画 (Shape)
  Skybox/       Pipeline/Renderer (スカイボックス描画。IBL用ソースとして活用可能)
  Sprite/       2Dスプライト描画
  StructuredBuffer/ StructuredBufferラッパー
  Text/         テキスト描画
  Texture/      Texture / TextureManager (DDS/TGA/HDR/WIC読み込み)
  Transition/   画面遷移エフェクト
  VertexBuffer/ 頂点バッファ
pch/            プリコンパイル済みヘッダ
system/
  AssetPath/    アセットパス解決 (ASSET_PATH マクロ)
  Camera/       カメラ
  Editor/       エディタ用システム
  Engine/       エンジン本体のエントリ/ループ
  ImGui/        ImGui統合
  Input/        入力 (マウス/コントローラー/最終入力デバイス判定)
  Light/        LightSystem (Directional/Point/Spot, シャドウ対象判定)
  Logger/       ログ (DEBUG_LOG マクロ)
  Physics/      JoltPhysics連携
  Scene/        シーン管理
  Time/         時間管理
  Window/       ウィンドウ
Utility/
  config/       設定読み込みユーティリティ
  Export/       エクスポートマクロ
  Singleton/    Singleton基底
  Thread/       マルチスレッド補助
```

## CoreEngine/Assets
```
Shader/
  Fbx/          VS_Fbx / PS_Fbx / VS_Shadow / FbxShader.hlsli (Cook-TorrancePBR, PCFシャドウ)
  Skybox/       SkyboxVS / SkyboxPS
  Shape/ Sprite/ Text/ Transition/ PhysicsDebug/  各用途別シェーダー
Texture/        White/Black/Normal のデフォルトテクスチャ (dds/png)
Fonts/          フォントアセット
```

## App/src
```
Data/
  Enemy/        敵データ定義
  Menu/         メニューデータ定義
Scene/
  Game/         Factory (生成) / State (ゲーム内ステート)
  Menu/ Title/ Test/  各シーン
system/
  CameraFollow/     カメラ追従
  Damage/PlayerContactDamage/ 接触ダメージ処理
  Enemy/Attack/ Enemy/Move/ Enemy/Status/  敵AI関連システム
  GlowAnimation/    発光アニメーション
  MenuController/   メニュー操作
  MoveDirection/    移動方向決定
  Player/
    AimSysten/      照準システム (マウス座標/コントローラー入力で方向決定)
    InputSystem/    プレイヤー入力
    MovementSystem/ 移動処理
    State/          プレイヤーステート
    Status/         プレイヤーステータス
    UI/             プレイヤーUI (HPバー等)
    Weapon/         武器
  RotateToMove/     移動方向への回転
  TitleInputSystem/ タイトル画面入力
Tag/            ECSタグコンポーネント群
pch/            プリコンパイル済みヘッダ
```

## App/Assets
```
Fbx/
  Faul/         プレイヤーキャラクター (Faul.fbx, Animation/*.fbx, Texture/Ganfaul_diffuse.png)
  Enemy/        敵キャラクター (samp_chara.fbx)
  Field/        フィールド (Field.fbx)
Effect/Parts/   Effekseer用パーツテクスチャ群
```

## 主要な設計メモ (コードから自明でない点)
- FBX用マテリアルはAlbedo/Normal/Metallic/Roughness/AO/Emissiveの6テクスチャスロット固定
  (`FbxShader.hlsli` t2〜t7)。未設定時は `Has*` フラグでデフォルト値にフォールバックする。
- インスタンス参照は `SV_InstanceID` に依存せず、Root32BitConstant (`g_InstanceIndex`) で
  明示的に渡す設計 (複数モデル同時描画時の取り違え防止のため)。
- シャドウは Directional Light 1灯・カスケードなし・固定2048解像度のみ対応
  (`FbxRenderer::SHADOW_MAP_SIZE`)。
- Editor機能 (`system/Editor/EditorSystem`, `EditorManager`, `EditorUI`) は
  `DEV_TOOL_ENABLED`(Debug/Develop)ビルドのEditモードでのみ動作
  (`Engine.cpp` の `#if DEV_TOOL_ENABLED` 分岐、Playモードでは通常のゲームループ。Releaseのみ無効)。
  - ImGuiの「Editor」パネルからBox/Sphere/PointLightを配置 → クリック選択(Joltレイキャスト、
    Collider必須) → 選択中に左クリック(Selectアクション)ドラッグでXZ平面移動。
  - 選択中に Delete キー、または Inspector パネルの「Delete Object」ボタンで削除
    (`PlaceableTag`が付いたエンティティのみ削除可能)。
  - 配置物はすべて `PlaceableTag` が付く。Save/Load Layout(JSON) と Play/Stopのスナップショット
    復元もこのタグを対象にする。
  - Editorパネル末尾に `DrawScenePanel()`(シーン切り替えUI)がある。`SceneFactory::GetRegisteredNames()`
    で登録済みシーンを列挙し、ボタンで `SceneManager::ChangeSceneWithTransition()` を呼ぶ。
