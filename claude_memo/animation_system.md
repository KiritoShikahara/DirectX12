# アニメーションシステム(Idle/Run切り替え)

## 中核コンポーネント

- `ecs::FbxComponent`: モデルリソース参照(`FbxResource*`)、`CustomColor`等
- `ecs::FbxAnimComponent`: 再生状態(`CurrentClipIndex`, `CurrentTime`等)。
  `Play(resource, clipName, loop)` / `CrossFade(resource, clipName, blendDuration, loop)`を持つ
- `ecs::MoveDirectionComponent::IsMoving`: そのエンティティが今フレーム移動しているか
- `graphics::FbxAnimSystem::Update`: 実際の時間進行・ボーン行列計算(エンジン側常設システム、
  `Engine::UpdateGameplay`から呼ばれる。Player/Enemy用の`LocomotionAnimationSystem`とは別物)

## LocomotionAnimationSystem（旧PlayerAnimationSystem）

場所: `App/src/system/Animation/LocomotionAnimationSystem.h/.cpp`

`FbxComponent + FbxAnimComponent + MoveDirectionComponent`を持つ**全エンティティ**
（Player/Enemy問わず）が対象。`MoveDirectionComponent::IsMoving`を見て
`"Run"`/`"Idle"`クリップ名でCrossFadeするだけの単純なシステム。

**重要な設計判断**: 元々`PlayerAnimationSystem`という名前でPlayerTag限定だったが、
ロジック自体はPlayer固有の要素を一切使っていなかった（`MoveDirectionComponent`は
敵も`EnemyChaseSystem`が更新している共通コンポーネント）ため、汎用化してリネームした。
新しい移動主体（NPC等）を追加する場合、`FbxAnimComponent`+`MoveDirectionComponent`を
付けるだけでこのシステムがそのまま効く。

**登録順序が重要**: `IsMoving`を更新するシステム（`PlayerMovementSystem`、
`EnemyChaseSystem`）の**後**に登録すること。先に置くと1フレーム遅延する。
`GameScene.cpp`内での実際の順序:
```
PlayerMovementSystem → EnemyChaseSystem → LocomotionAnimationSystem
```

## クリップ登録の仕組み

`FbxResource::LoadAnm(anmPath, clipName)`でクリップを名前付きで登録する。
`FbxResource::FindClipIndex(name)`で名前引きできる。

Faulリソースはプレイヤーと敵で**共有**（`FbxResourceManager`がbinPathでキャッシュする）。
そのため、**プレイヤー生成時(`GameSceneFactory::CreatePlayer`)に一度だけ
"Idle"/"Run"クリップを登録**しており、敵生成(`CreateEnemy`)側では登録処理をしていない
（敵は常にプレイヤー生成後に`EnemySpawnSystem`が生成するため、登録済みの前提で安全）。
もし敵が先に生成される設計に変わった場合はこの前提が崩れるので注意。

`CreatePlayer`側の該当コード:
```cpp
if (player_res->FindClipIndex("Idle") < 0)
    FbxResourceManager::Get().LoadAnm(kFaulBin, ".../Idle.fbx.anm", "Idle");
if (player_res->FindClipIndex("Run") < 0)
    FbxResourceManager::Get().LoadAnm(kFaulBin, ".../Jog.fbx.anm", "Run");
```
`FindClipIndex`チェックがあるのはシーン再入場時の重複登録防止。

`CreateEnemy`側は登録なしでそのまま:
```cpp
auto& anim = manager.AddComponent<ecs::FbxAnimComponent>(enemy);
if (res != nullptr) anim.Play(*res, "Idle", true);
```

## 新しいモデル/新しいアニメを追加する場合の手順

1. FbxConverterで変換（`fbx_converter.md`参照。Z-up/Y-upの整合に注意）
2. モデル生成箇所で`LoadAnm`によるクリップ登録（初回のみでよい、`FindClipIndex`で重複防止）
3. `FbxAnimComponent`を付与、`Play(resource, "Idle", true)`で初期状態を設定
4. `MoveDirectionComponent`を持たせれば`LocomotionAnimationSystem`が自動でIdle/Runを切り替える
