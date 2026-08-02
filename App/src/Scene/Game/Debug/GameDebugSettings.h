#pragma once

#include <Utility/Singleton/Singleton.hpp>
#include <string>
#include <random>

namespace debug
{
    ///<summary>
    ///開発時にゲームの挙動を一時的に変えるためのフラグ群。ImGuiまたはコマンドライン引数から設定できる
    ///</summary>
    class GameDebugSettings : public utility::Singleton<GameDebugSettings>
    {
        SINGLETON_CLASS(GameDebugSettings);
    public:
        SINGLETON_ACCESSOR(GameDebugSettings);

        ///<summary>
        ///ImGuiウィンドウを登録する、開発ツール有効時のみ
        ///</summary>
        void Initialize();

        ///<summary>
        ///ImGuiウィンドウの登録を解除する
        ///</summary>
        void Finalize();

        ///<summary>
        ///コマンドライン引数を解析してフラグへ反映する。対応: --godmode、--autoexit=秒数、--seed=乱数シード
        ///</summary>
        void ParseCommandLine(const char* commandLine);

        ///<summary>
        ///プレイヤーが被ダメージを受けないか。PlayerContactDamageSystemが参照する
        ///</summary>
        bool IsPlayerInvincible() const { return mPlayerInvincible; }

        ///<summary>
        ///パーク選択を自動で確定するか。パーク選択中はTimeScale=0で止まるため、無人計測では必須
        ///</summary>
        bool IsAutoSelectPerk() const { return mAutoSelectPerk; }

        ///<summary>
        ///自動終了までの秒数。0以下で無効
        ///</summary>
        float GetAutoExitSeconds() const { return mAutoExitSeconds; }

        ///<summary>
        ///乱数シードを固定するか。固定しないと実行ごとに負荷が変わり最適化の前後を比較できない
        ///</summary>
        bool IsFixedSeed() const { return mFixedSeed; }

        ///<summary>
        ///固定時に使う乱数シード
        ///</summary>
        unsigned int GetRandomSeed() const { return mRandomSeed; }

        ///<summary>
        ///乱数エンジンを生成する。シード固定時はその値、そうでなければstd::random_deviceで初期化する
        ///</summary>
        std::mt19937 MakeRandomEngine() const;

    private:
        void ImGuiWindow();

        ///<summary>
        ///高負荷状態を維持したまま計測するためのフラグ。通常プレイでは死亡で敵数が頭打ちになり本来の負荷を計測できない
        ///</summary>
        bool mPlayerInvincible = false;

        ///<summary>
        ///レベルアップ時の入力待ちで無人計測が止まらないよう、計測時は自動で確定させるためのフラグ
        ///</summary>
        bool mAutoSelectPerk = false;

        float mAutoExitSeconds = 0.0f;

        ///<summary>
        ///最適化の前後比較のため、計測時は乱数を固定して負荷を再現可能にするためのフラグ
        ///</summary>
        bool         mFixedSeed = false;
        unsigned int mRandomSeed = 12345;
    };
}
