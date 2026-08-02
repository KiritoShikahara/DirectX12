# フィールドサイズ・境界壁・シャドウ追従

## FieldConstants.h

`App/src/Scene/Game/Factory/FieldConstants.h`

フィールド関連の寸法を1箇所に集約した定数（地面生成・境界壁生成・敵スポーンの
境界クランプで値がズレないようにするため）。

- `kRawHalfExtent`(50): `Field.fbx.bin`の生データ半径。**実測値**
  （バイナリを直接パースして頂点座標の min/max を取って確認した。X/Zとも[-50,50]、
  Y=0固定の完全な平面）
- `kScale`(40): `CreateGround`で適用するスケール。ワールド半径 = 50×40 = 2000
- `kPlayableHalfExtent`(= kWorldHalfExtent - 100): 実際にプレイヤーが移動できる範囲。
  見た目のフィールド端より内側に余裕を持たせてある
- `kWallHalfThickness` / `kWallHalfHeight`: 見えない境界壁の厚み・高さ

## バイナリを直接パースしてワールドサイズを実測する方法

`.bin`のフォーマット（`FbxConverter`が出力）:
```
[MeshCount:i32][PolygonCount:i32][VertexCount:i32]
頂点ごとに: Position(vec3,12B) UV(vec2,8B) Normal(vec3,12B) Tangent(vec3,12B)
            Bone[4](16B) Weight[4](16B)  ※1頂点=76bytes
```
PowerShellの`BinaryReader`で読み、Position(x,y,z)のmin/maxを取れば実サイズが分かる
（`data_csv_db.md`のsqlite3ツールと同様、「目視で確認できないなら実データを読む」の一例）。

## 境界壁（GameSceneFactory::CreateFieldBoundary）

見えない静的コライダー(`RigidBodyComponent::MakeStatic` + `ColliderComponent::MakeBox`)を
四辺に配置。描画は無し（当たり判定のみ）。プレイヤーがフィールド外に出ないようにする。

**この壁を追加した副作用として、`EnemySpawnSystem::ComputeSpawnPosition`が
プレイヤー相対の座標だけで計算していたため、プレイヤーが境界付近にいると
壁の外側に敵が湧いて詰む不具合が起きた。** 境界を追加した場合、それに依存する
座標計算（スポーン位置、ノックバック着地点など）が範囲外に出ないか必ず確認すること。
`ComputeSpawnPosition`は現在`FieldConstants::kPlayableHalfExtent`でclampして解決済み。

## マップ外を隠す（Skybox）

`GameSceneFactory::CreateSkybox()`で`ecs::SkyboxComponent`を追加、
`Assets/Skybox/skybox.dds`を使用。以前はGameSceneで一切使われておらず、
フィールド外が「クリアカラーの虚無」に見えていた。

## カメラFar Clip

フィールド拡張時は`CameraComponent::Far`もフィールドサイズに合わせて拡張すること
（`GameSceneFactory::CreateCamera`）。Far不足だと地平線方向でフィールド端や
Skyboxとの継ぎ目が正しく描画されない。

## DirLightFollowSystem（シャドウのプレイヤー追従）

`App/src/system/Light/DirLightFollowSystem.h/.cpp`

指向性ライトの`DirectionalLightComponent::ShadowTarget`を、毎フレーム
プレイヤー位置(XZのみ、Yは固定)へ追従させる。`PostUpdate`フェーズに登録、
`Engine.cpp`内で直接呼ばれる`LightSystem::Update`（`LightViewProj`計算）より前に
確定させる必要がある。

**理由**: フィールドは2000×2000と広いが、シャドウマップの解像度(2048×2048固定)を
維持するため`ShadowRange`は150程度に絞ってある。`ShadowTarget`をワールド原点に
固定していると、プレイヤーが少し移動しただけでシャドウ計算の範囲外に出てしまい、
フィールドの大部分で陰影が付かない（「光が届かない」ように見える）不具合になっていた。
`CameraPlayerFollowSystem`と同じ「追従」パターンで解決している。
