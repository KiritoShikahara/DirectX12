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

        /// <summary>
        /// インスタンスの生成済み判定
        /// </summary>
        /// <returns>true:生成済み</returns>
        static bool HasInstance() noexcept {
            return mInitialized;
        }

        /// <summary>
        /// 破棄時にフラグの破棄
        /// </summary>
        static void DestroyInstance() noexcept {
            mInitialized = false;
#ifdef SINGLETON_POLICY_NO_REVIVE
            mDestroyed = true;
#endif
        }

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


// フレンド指定用
#define SINGLETON_FRIEND(T) \
    friend class Singleton<T>

// フレンド指定とPrivateコンストラクタ系の定義
#define SINGLETON_CLASS(T) \
    SINGLETON_FRIEND(T);   \
private:                   \
    T() = default;         \
    ~T() = default


// 引数アリのコンストラクタ定義用
#define SINGLETON_CLASS_CUSTOM_CTOR(T) \
    SINGLETON_FRIEND(T);               \
private:                               \
    ~T() = default

// 省略用：Getで取得できるように
#define SINGLETON_ACCESSOR(T) \
    static T& Get() { return T::GetInstance(); }

// ローカル変数で参照する用
// なくても別に問題ないけどボイラーコード減らすため
#define SINGLETON_REF(T, name) \
    T& name = T::GetInstance()