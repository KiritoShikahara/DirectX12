# FbxConverterツール

## 場所

- ソース: `C:\Users\kirit\source\repos\FbxConverter`（DirectX12本体とは**別リポジトリ**）
- ビルド成果物の配置先: `CoreEngine\Tool\FbxConverter.exe`（+ `.pdb`）
- 本体プロジェクトはこの配置先のexeを`App/Assets/Fbx/**/*.fbx`の変換に使う

## 使い方

```
FbxConverter.exe <FbxFilePath> [--uemodel] [--rootnomove]
```
（ファイルパスが先、オプションは後。逆にすると"Invalid Option"になる）

出力: `<file>.bin`（メッシュ・ボーン・マテリアル。メッシュが無いFBXは
"Not 3DModel File. Skip Convert"でスキップされる）と `<file>.anm`（アニメーションキーフレーム）。
`.txt`版も同時出力される（人間が読める形式、NumFrame/NumBone等）。

## フラグの意味

### `--uemodel`
UE(Unreal Engine)由来のZ-up座標のFBXを、本エンジンのY-upへ補正する。
具体的には**ルートボーン(親を持たないボーン)にだけ**-90°X回転を後乗算する
（`FbxConverter.cpp`の`LoadKeyFrame`関数）。子ボーンは親相対のローカル変換のため、
ルートだけ回せば階層全体が正しく起き上がる。

**モデル(.bin)とアニメ(.anm)は同じ座標系の前提を共有させること。**
片方だけ`--uemodel`を付けて片方は付けない、という運用をすると、
エンジン側でモデルとアニメの基準がズレて破綻する(過去に実際に「モデル崩壊」を起こした)。

判断基準: 変換後に立って表示されない（寝そべる）→ `--uemodel`が必要な可能性が高い。
逆に、フラグ無しで既に正しく立っているアニメ(例: Attack系)に`--uemodel`を追加で
付けると壊れる。**「同じキャラクターの別アニメだから同じフラグのはず」と決め打ちせず、
アニメごとに実際どちらが正しいか検証すること**（同じFaulモデルでもAttack_A.fbxは
フラグ無しで正常、Idle.fbx/Jog.fbxは`--uemodel`が必要、という事例があった）。

### `--rootnomove`
ルートボーンの**水平移動(X/Z)だけ**を0にする（その場ループ用、ルートモーション除去）。
歩行・走行のようにキャラクターが前進するアニメを、キャラクターの移動を物理側
(RigidBody)で制御しつつ、その場で足踏みさせたい場合に使う。

**Y(高さ)には絶対に触らないこと。** かつてY成分も0にするバグがあり、
ルートボーン(Hips)の基準の高さ自体が地面レベルまで落ち、スキニングされた
メッシュ全体が「地面に半分埋まる」不具合になった（`FbxConverter.cpp`の
`LoadAnimation`関数、`isRootBoneNoMove`分岐）。修正済みだが、同種の改修をする際は
X/Z/Yのどれを触っているか必ず確認すること。

## 過去に見つかった重大バグ（修正済み、再発防止用の記録）

`main.cpp`で`bool OptionUEModel = true;`とデフォルト**true**で初期化されていた。
コマンドライン引数の有無に関わらず常にUEModelオプションが有効になってしまう
バグで、実証済み（フラグ無しで実行しても"Enable UEModel Option."とログが出た）。
`false`に修正済み。**もしFbxConverterのソースに再度手を入れる場合、
このデフォルト値が壊れていないか必ず確認すること。**

## 座標変換の確認方法（ボーン行列を直接読む）

`.anm`のバイナリフォーマット:
```
[NumFrame:i32][numBone:i32]
  ボーンごとに [frameCount:i32][frameCount × Matrix(16 float, 64bytes)]
```
ボーン0は常にルート(親なし)。ルートのframe0行列の平行移動成分(`_41,_42,_43`
＝ファイルの12バイト目から64バイト、その中の最後の4要素)を読めば、
「どの軸が高さ(Y)として扱われているか」を実データで確認できる（Y-upなら
_42が大きい値、Z-upなら_43が大きい値になる、等）。目視で「寝ている/立っている」を
判断するより、この数値比較の方が確実。

## 関連するエンジン側の仕組み

- `FbxResource::LoadAnm(anmPath, clipName, convertZUpToYUp=false)`:
  エンジン側にも「Z-up→Y-up変換」の保険機能がある（`CoreEngine/src/graphics/Fbx/Resource/FbxResource.cpp`）。
  ただし現在は**コンバータ側で正しく変換したファイルを使う運用に統一**しており、
  この引数は`false`のまま使っていない（二重に回転をかけると崩れるため、
  コンバータ側で解決したファイルにはtrueを渡さないこと）。汎用ユーティリティとして
  残してあるだけ。
- `WeaponIconRegistry`等とは無関係。純粋にFBXインポート層の話。

## モデル/アニメのアセット構成（Faul）

- `App/Assets/Fbx/Faul/Faul.fbx` → `Faul.fbx.bin`（モデル本体、フラグ無し変換）
- `App/Assets/Fbx/Faul/Animation/Attack_A.fbx` 等 → フラグ無し変換（元々Y-up）
- `App/Assets/Fbx/Faul/Animation/Idle.fbx` → `--uemodel`
- `App/Assets/Fbx/Faul/Animation/Jog.fbx`(="Run"クリップ名) → `--uemodel --rootnomove`
- `App/Assets/Fbx/Ganfaul/`は同一素材のバックアップ（MD5一致確認済み）。
  Faul側を更新したらこちらも同期するとよい（必須ではない）。
