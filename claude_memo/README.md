# claude_memo

Claude(AI)が過去のセッションで調査・実装した「仕組み」の知見を、次回以降のセッションで
再調査せずに済むようまとめたメモ。プロジェクト本体のドキュメントではなく、AIの作業効率化用。

CLAUDE.md（設計方針・命名規則等）とは役割が異なる。こちらは「どう動いているか」
「何が罠だったか」を記録する場所。コードが変わったら都度更新すること。

## 目次

| ファイル | 内容 |
|---|---|
| [data_csv_db.md](data_csv_db.md) | CSV/DBデータ管理の仕組み。Developで値を変えるにはDB直接更新が必要な理由と手順 |
| [fbx_converter.md](fbx_converter.md) | FbxConverterツールの使い方・フラグの意味・過去のバグ |
| [animation_system.md](animation_system.md) | Idle/Runアニメーション切り替えの仕組み(LocomotionAnimationSystem) |
| [weapon_registries.md](weapon_registries.md) | 武器システムの3つのレジストリパターン、新武器追加時に触る箇所 |
| [ui_patterns.md](ui_patterns.md) | UI実装パターン(パーク選択・所持武器バー・HPバー等) |
| [field_and_light.md](field_and_light.md) | フィールドサイズ・境界壁・シャドウ追従の仕組み |
| [build_notes.md](build_notes.md) | ビルドコマンド・BOM・DEV_TOOL_ENABLED統一の注意点 |

## 関連リポジトリ

- 本体: `C:\Users\kirit\source\repos\DirectX12`
- FBX変換ツール: `C:\Users\kirit\source\repos\FbxConverter`（別リポジトリ。ビルド後 `CoreEngine\Tool\FbxConverter.exe` へ配置する運用）
