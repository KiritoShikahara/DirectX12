# ビルド関連の注意点

## .slnが無い。vcxprojを直接指定する

リポジトリ内に`.sln`ファイルが存在しない。`App.vcxproj`は`$(SolutionDir)`を
前提にしたインクルードパス（`$(SolutionDir)CoreEngine`等）を使っているため、
`SolutionDir`を明示的に渡さないとビルドが通らない。

**Developビルドの正しいコマンド:**
```bash
MSB="/c/Program Files/Microsoft Visual Studio/2022/Community/MSBuild/Current/Bin/MSBuild.exe"
"$MSB" App/App.vcxproj -p:Configuration=Develop -p:Platform=x64 \
  '-p:SolutionDir=C:\Users\kirit\source\repos\DirectX12\' -m -nologo -v:minimal
```
`SolutionDir`は末尾に`\`が必要。これを忘れると
`fatal error C1083: include ファイルを開けません`になる。

成果物: `x64\Develop\App.exe`（CoreEngine.libも同時に再ビルドされる）。

## 構成の意味（DEV_TOOL_ENABLED）

`CoreEngine/src/Utility/config/DebugConfig.h`:
```cpp
#if defined(_DEBUG) || (defined(ECSE_DEV_TOOL) && ECSE_DEV_TOOL)
#define DEV_TOOL_ENABLED  (1)
#else
#define DEV_TOOL_ENABLED  (0)
#endif
```
- **Debug**: `_DEBUG`あり → 開発ツール有効、最適化なし
- **Develop**: `ECSE_DEV_TOOL=1` → 開発ツール有効、**最適化はReleaseと同一**
  （性能計測はDevelopかReleaseで行うこと。Debugは数十倍遅く実性能の参考にならない）
- **Release**: どちらも無し → 製品版

判定は必ず`#if DEV_TOOL_ENABLED`と書くこと。`#if defined(DEV_TOOL_ENABLED)`は
Releaseでも`(0)`として定義されているため常に真になり、意図と逆になる（よくある事故）。

`#if DEV_TOOL_ENABLED`を使うファイルは**必ず`<Utility/config/DebugConfig.h>`を
直接includeすること**（CoreEngine側は`pch.h`経由で見えるが、App側の`apppch.h`には
含まれていないため、includeし忘れるとマクロ未定義で`#if`が常に0扱いになり、
機能がRelease相当（無効）のまま気づかず放置される事故が過去にあった）。

`_DEBUG`のみ専用にすべきもの（DEV_TOOL_ENABLEDへ変換しないこと）:
- `main.cpp`の`_CRTDBG_MAP_ALLOC`（Debug CRTのメモリリーク検出、Developでは使えない）
- `CoreEngine/src/Data/Storage/Manager/DataManager.h`のCSV/DB切り替え
  （ユーザー指示により意図的に`_DEBUG`のまま。`data_csv_db.md`参照）

## 新規ファイルの作法

1. **UTF-8 BOM必須**（日本語コメントがコードページ932として誤解釈されるのを防ぐ）。
   `Write`ツールはBOMを付けないことがあるため、作成後に必ずバイト確認:
   ```bash
   head -c 3 file.cpp | od -An -tx1   # "ef bb bf" でなければBOM無し
   ```
   無ければ`printf '\xEF\xBB\xBF' > tmp && cat file >> tmp && mv tmp file`で付与。

2. **`App.vcxproj` / `App.vcxproj.filters`への登録が必須**（`.sln`が無いため
   Visual Studioでの追加操作を経由せず、直接XMLを編集する運用になっている）。
   `<ClCompile Include="...">`と`<ClInclude Include="...">`を追加し、
   filtersの方は同じパスで`<Filter>ソース ファイル</Filter>` /
   `<Filter>ヘッダー ファイル</Filter>`を追加するだけでよい（ディレクトリ単位の
   フィルタ分けはされていない、全部同じ2フィルタに入る）。

3. ファイル削除時はvcxproj/filters両方からエントリを削除すること
   （残っているとビルド時にファイルが見つからずエラーになる）。

## アセットパス解決の2方式が混在している(exe単体配布で罠になる)

- **CoreEngine内部資産**(シェーダー/フォント/既定テクスチャ/BootstrapConfig)は
  `sys::AssetPathManager`(`CoreEngine/src/system/AssetPath/AssetPathManager.h`)
  経由。`ASSET_PATH("/Engine/...")`マクロで仮想パスを解決する。
  `.game_root`(`App/.game_root`)・`.engine_root`(`CoreEngine/.engine_root`)という
  マーカーファイルを、exeの場所(`GetModuleFileNameW`)またはCWDから最大20階層
  親を遡りつつ、各階層の直下フォルダも探索して見つける方式。**exeの場所にも
  依存できるため、配布後も動く設計。**
- **App側のゲームコンテンツ**(FBX/テクスチャ/CSV/DB/セーブデータ/効果音/エフェクト)
  は`ASSET_PATH`を経由せず、`"Assets/Data/Enemy/EnemyData.csv"`のような**生の
  相対パス文字列**をそのまま`TextureManager`/`FbxResourceManager`/`DataRegistry`/
  `ConfigRegistry`/`AudioManager`/`EffekseerManager`に渡している(App配下のみで
  100箇所近く該当、全て`"Assets/"`始まりで例外なし)。これは`std::filesystem::absolute()`
  で**プロセスのCWD基準**に解決されるため、**exeをどこから起動したかでCWDが変わると
  読み込みが壊れる**(VSからF5実行だとCWD既定値が`$(ProjectDir)`=`App\`になり
  たまたま動くが、`x64\Release\App.exe`を直接ダブルクリックするとCWDがexe自身の
  フォルダになり、直下に`Assets`が無いため全滅する)。

**対策(実装済み)**:
1. `Engine::Initialize()`(`CoreEngine/src/system/Engine/Engine.cpp`)で
   `AssetPathManager::Initialize()`直後に`AssetPathManager::Get().Resolve("/Game")`
   でGameルートを取得し、`std::filesystem::current_path()`でCWDをそこへ固定。
   起動方法に関わらず生の相対パス側の解決先を一致させる。
2. `App.vcxproj`のRelease構成PostBuildEventで、`$(TargetDir)`配下に
   `Game\`(`App/.game_root` + `App/Assets/*`)と`Engine\`
   (`CoreEngine/.engine_root` + `CoreEngine/Assets/*` + `CoreEngine/BootstrapConfig/*`)
   をxcopyで複製する。`AssetPathManager`のマーカー探索は「自分の直下サブフォルダに
   マーカーがあるか」も見るため、exe直下に`Game\`/`Engine\`が揃っていれば
   **exeのフォルダを丸ごとコピーするだけで単体起動できる**(ソースツリー側の
   `App\`/`CoreEngine\`を探しに行かない)。Debug/Developはこの複製をしない
   (VS実行前提でCWDが`App\`になるため元から動く。無用な変更を避けるため据え置き)。

## 起動シーンの切り替え(START_SCENE_NAME)

`App/src/macros.h`の`START_SCENE_NAME`は`DEV_TOOL_ENABLED`で分岐する
(`#if DEV_TOOL_ENABLED`則り、`<Utility/config/DebugConfig.h>`を直接include必須)。
- Debug/Develop: `GAME_SCENE_NAME`(検証効率優先で従来通りGameから開始)
- Release: `TITLE_SCENE_NAME`(製品版は必ずTitleから開始)

## コンソール出力の文字化けに注意

Windowsコンソールの既定コードページ(932)では、UTF-8(BOM付き)ソースの
日本語`std::cout`出力が文字化けする（ソースの文字コードとは別の問題）。
コマンドラインツール（FbxConverter等）でのユーザー向けメッセージは、
確実性を優先して英語にする方が安全（実際にUsageメッセージを日本語→英語に
変更して解決した事例がある）。ゲーム本体側のImGuiデバッグUIは別経路のため
この問題の対象外。
