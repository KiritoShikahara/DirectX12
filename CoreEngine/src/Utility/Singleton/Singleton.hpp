#pragma once

#include <memory>
#include <mutex>
#include <stdexcept>
#include <type_traits>
#include <utility>

#ifndef SINGLETON_ALLOW_RESET
#define SINGLETON_POLICY_NO_REVIVE 1
#endif

namespace utility
{
    template <typename T>
    class Singleton {
    public:

        template <typename... Args>
        static T& GetInstance(Args&&... args) {
#ifdef SINGLETON_POLICY_NO_REVIVE
            if (mDestroyed) {
                throw std::logic_error(
                    "Singleton<T>::GetInstance() called after destroyInstance(). "
                    "Define SINGLETON_ALLOW_RESET to enable re-creation.");
            }
#endif
            static T instance{ std::forward<Args>(args)... };
            return instance;
        }

        // ----------------------------------------------------------
        //  インスタンスが生成済みかを確認する
        //  (Meyers' Singleton では「最初の getInstance 後は常に true」)
        // ----------------------------------------------------------
        static bool HasInstance() noexcept {
            return mInitialized;
        }

        static void DestroyInstance() noexcept {
            mInitialized = false;
#ifdef SINGLETON_POLICY_NO_REVIVE
            mDestroyed = true;
#endif
        }

        // ----------------------------------------------------------
        //  コピー / ムーブ を明示的に禁止
        // ----------------------------------------------------------
        Singleton(const Singleton&) = delete;
        Singleton& operator=(const Singleton&) = delete;
        Singleton(Singleton&&) = delete;
        Singleton& operator=(Singleton&&) = delete;

    protected:
        Singleton() {
            mInitialized = true;  // コンストラクト完了を記録
        }
        ~Singleton() = default;

    private:
        static inline bool mInitialized{ false };
#ifdef SINGLETON_POLICY_NO_REVIVE
        static inline bool mDestroyed{ false };
#endif
    };
}


// ============================================================
//  便利マクロ集
// ============================================================

/**
 * @brief Singleton<T> をフレンド宣言する。
 *        private コンストラクタを持つ派生クラスで必須。
 */
#define SINGLETON_FRIEND(T) \
    friend class Singleton<T>

 /**
  * @brief クラス宣言のボイラープレートを一括定義する。
  *        継承・フレンド宣言・コピー禁止・デフォルト private コンストラクタを自動生成。
  *
  *  使い方:
  *    class AppConfig : public Singleton<AppConfig> {
  *        SINGLETON_CLASS(AppConfig);
  *    public:
  *        void load(const std::string& path);
  *    };
  */
#define SINGLETON_CLASS(T) \
    SINGLETON_FRIEND(T);   \
private:                   \
    T() = default;         \
    ~T() = default


  /**
   * @brief 引数付きコンストラクタを持つクラス向けボイラープレート。
   *        コンストラクタ定義は自分で書く。フレンド宣言とコピー禁止のみ生成。
   *
   *  使い方:
   *    class Connection : public Singleton<Connection> {
   *        SINGLETON_CLASS_CUSTOM_CTOR(Connection);
   *    public:
   *        explicit Connection(const std::string& host, int port);
   *    };
   *
   *    // 初回だけ引数を渡す
   *    auto& conn = Connection::getInstance("localhost", 5432);
   */
#define SINGLETON_CLASS_CUSTOM_CTOR(T) \
    SINGLETON_FRIEND(T);               \
private:                               \
    ~T() = default

   /**
    * @brief T::get() という短縮アクセサを静的メソッドとして追加する。
    */
#define SINGLETON_ACCESSOR(T) \
    static T& Get() { return T::GetInstance(); }

    /**
     * @brief 指定シングルトンのインスタンス参照をローカル変数に束縛する。
     *        関数内で複数回使う際の記述を短縮する。
     *
     *  使い方:
     *    void foo() {
     *        SINGLETON_REF(Logger, log);
     *        log.write("message");
     *    }
     */
#define SINGLETON_REF(T, name) \
    T& name = T::GetInstance()