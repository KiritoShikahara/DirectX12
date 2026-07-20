#pragma once

#include <Utility/Singleton/Singleton.hpp>
#include <string>
#include <random>

namespace debug
{
    /// <summary>
    /// 開発時にゲームの挙動を一時的に変えるためのフラグ群。
    ///
    /// ImGuiの「Debug Settings」ウィンドウから切り替えられるほか、
    /// 起動時のコマンドライン引数からも設定できる
    /// (性能計測を自動化する際、GUI操作なしで同じ条件を再現するため)。
    ///
    /// 開発ツールが無効なビルド(製品版のRelease)では、
    /// フラグは常に既定値のままでUIも登録されない。
    /// </summary>
    class GameDebugSettings : public utility::Singleton<GameDebugSettings>
    {
        SINGLETON_CLASS(GameDebugSettings);
    public:
        SINGLETON_ACCESSOR(GameDebugSettings);

        /// <summary>ImGuiウィンドウを登録する(開発ツール有効時のみ)</summary>
        void Initialize();

        /// <summary>ImGuiウィンドウの登録を解除する</summary>
        void Finalize();

        /// <summary>
        /// コマンドライン引数を解析してフラグへ反映する。
        /// 対応している引数:
        ///   --godmode           プレイヤーが被ダメージを受けなくなる
        ///   --autoexit=SECONDS  指定秒後に自動終了する(0で無効)
        /// </summary>
        void ParseCommandLine(const char* commandLine);

        /// <summary>プレイヤーが被ダメージを受けないか(PlayerContactDamageSystemが参照する)</summary>
        bool IsPlayerInvincible() const { return mPlayerInvincible; }

        /// <summary>
        /// パーク選択を自動で確定するか(PerkSelectSystemが参照する)。
        /// パーク選択中はTimeScale=0でゲームが止まるため、無人で計測する際は必須。
        /// </summary>
        bool IsAutoSelectPerk() const { return mAutoSelectPerk; }

        /// <summary>自動終了までの秒数(0以下で無効)</summary>
        float GetAutoExitSeconds() const { return mAutoExitSeconds; }

        /// <summary>
        /// 乱数シードを固定するか。
        /// 敵の湧き位置・種類・パークの選択肢はいずれも乱数で決まるため、
        /// 固定しないと実行ごとに負荷が変わり、最適化の前後を比較できない。
        /// </summary>
        bool IsFixedSeed() const { return mFixedSeed; }

        /// <summary>固定時に使う乱数シード</summary>
        unsigned int GetRandomSeed() const { return mRandomSeed; }

        /// <summary>
        /// 乱数エンジンを生成する。シード固定が有効ならその値で、
        /// 無効なら従来通りstd::random_deviceで初期化する。
        /// 乱数を使う各システムはこれを経由すること。
        /// </summary>
        std::mt19937 MakeRandomEngine() const;

    private:
        void ImGuiWindow();

        // 高負荷状態を維持したまま計測したい場合に使う。
        // 通常プレイでは死亡によって敵の数が頭打ちになり、
        // 本来の高負荷時の性能が計測できないため
        bool mPlayerInvincible = false;

        // レベルアップのたびに入力待ちで停止してしまうと無人計測が進まないため、
        // 計測時は自動で確定させる
        bool mAutoSelectPerk = false;

        float mAutoExitSeconds = 0.0f;

        // 最適化の前後比較を成立させるため、計測時は乱数を固定して負荷を再現可能にする
        bool         mFixedSeed = false;
        unsigned int mRandomSeed = 12345;
    };
}
