# CSV/DBデータ管理の仕組み

## 結論（最初に読むこと）

**Debugビルドは `Assets/Data/**/*.csv` を直接読む。DevelopとReleaseビルドは
`App/Assets/Bin/DB/db.db`（SQLite）を読む。**

CSVを編集しただけでは **Develop/Releaseには反映されない**。バランス調整で
CSVを直接編集した場合、DBも合わせて更新しないと、Developで実機確認しても
古い値のまま動く（これに気づかず「直したのに変わらない」と混乱しがちなので注意）。

該当コード: `CoreEngine/src/Data/Storage/Manager/DataManager.h`
```cpp
void Load()
{
#ifdef _DEBUG
    LoadFromCsv();
#else
    LoadFromDb();
#endif
}
```

この`_DEBUG`分岐は意図的な設計（ユーザー指示: 「DevelopはDBから読むように」）。
`DEV_TOOL_ENABLED`統一の対象**外**なので、他の`_DEBUG`箇所と混同して変換しないこと。

## DBを更新する2つの方法

### 方法A: ゲーム内のDataInspector UI（本来の運用）
Developで開発ツール(`DEV_TOOL_ENABLED`)が有効なため、対応するデバッグパネルを開き
「Save CSV->DB」ボタン（`CoreEngine/src/Data/Storage/Inspector/DataInspector.h`）を押す。
CSVを先に編集してから実行する。AIはGUI操作ができないため、この方法は使えない。

### 方法B: sqlite3を直接叩く自作ツール（AIが使う方法）
プロジェクト内に SQLite のアメルガメーション(`sqlite3.c`/`sqlite3.h`)が既にある:
`CoreEngine/external/SQLite/sqlite3.c` , `sqlite3.h`

これをMSVCでコンパイルするだけで、依存無しの最小SQLite CLIが作れる。
Windows環境には`sqlite3.exe`コマンドが標準で無く、Pythonも入っていないことがあるため、
この方法が最も確実。

**手順（スクラッチパッド等の作業ディレクトリで）:**

1. `sqlite3.c`/`sqlite3.h`をコピー
2. 簡単なCLIラッパー(`dbtool.c`)を書く（SQLを引数で受け取り`sqlite3_exec`するだけ）
```c
#include <stdio.h>
#include "sqlite3.h"
static int print_callback(void* n, int argc, char** argv, char** cols){
    for (int i=0;i<argc;i++) printf("%s=%s  ", cols[i], argv[i]?argv[i]:"NULL");
    printf("\n"); return 0;
}
int main(int argc, char** argv){
    if (argc<3){ printf("Usage: dbtool.exe <dbpath> <sql>\n"); return 1; }
    sqlite3* db=NULL;
    if (sqlite3_open(argv[1], &db)!=SQLITE_OK){ printf("open fail\n"); return 1; }
    char* err=NULL;
    if (sqlite3_exec(db, argv[2], print_callback, NULL, &err)!=SQLITE_OK){
        printf("SQL error: %s\n", err); sqlite3_free(err); return 1;
    }
    printf("OK. Changes: %d\n", sqlite3_changes(db));
    sqlite3_close(db); return 0;
}
```
3. MSVC開発者環境でコンパイル（vcvars64.batを経由してcl.exeを呼ぶ）:
```powershell
$vcvars = "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
cmd.exe /c "call `"$vcvars`" >nul && cd /d <workdir> && cl.exe /O2 /nologo dbtool.c sqlite3.c /Fe:dbtool.exe"
```
（`vswhere.exe`関連の警告が出ても、その後コンパイルは正常に進む。`dbtool.exe`が
生成されていれば成功。）

4. 使い方:
```
dbtool.exe "App/Assets/Bin/DB/db.db" "SELECT * FROM enemies;"
dbtool.exe "App/Assets/Bin/DB/db.db" "UPDATE enemies SET MoveSpeed=60 WHERE Id=0;"
```

**必ず先にSELECTでDBの現在値とCSVの値を突き合わせてから更新すること**
（過去にゲーム内バランス調整でDBだけ先に変わっている可能性があるため、
CSVの値を無条件に信用しない）。

## テーブル名の調べ方

各マスタデータ構造体の`REFLECT_BEGIN(Type, "table_name")`の第2引数がテーブル名。
例: `App/src/Data/Enemy/EnemyData.h` → `REFLECT_BEGIN(EnemyData, "enemies")` → テーブル名`enemies`。
列名は`REFLECT_FIELD_*`マクロの引数（構造体メンバ名と同じ）。

## 運用ルール

CSVとDBの値を編集したら、**両方を同じ値に揃えること**（Debug/Develop/Releaseで
挙動が食い違わないようにするため）。片方だけ更新して忘れるとバグの温床になる。
