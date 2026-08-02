#pragma once

#include "../Loader/JsonSerializer.h"
#include "../Reflection.h"
#include <string>
#include <functional>

namespace data
{
    /// <summary>
    /// ConfigManager
    /// JSON ファイルへの自動 Save / Load + Dirty 検知。
    /// </summary>
    template<typename T>
    class ConfigManager
    {
    public:
        explicit ConfigManager(std::string filePath)
            : mFilePath(std::move(filePath))
        {
        }

        /// <summary>設定をファイルからロードする</summary>
        bool Load()
        {
            const bool loaded = JsonSerializer::LoadFromFile(mFilePath, mData);
            mLastMessage = loaded
                ? "[Config] Loaded: " + mFilePath
                : "[Config] Using defaults: " + mFilePath;
            mDirty = false;
            if (mOnChanged) mOnChanged(mData);
            return loaded;
        }

        /// <summary>設定をファイルにセーブする</summary>
        void Save()
        {
            JsonSerializer::SaveToFile(mFilePath, mData);
            mLastMessage = "[Config] Saved: " + mFilePath;
            mDirty = false;
        }

        /// <summary>設定をデフォルト値にリセットする</summary>
        void Reset()
        {
            mData = T{};
            mDirty = true;
            if (mOnChanged) mOnChanged(mData);
        }

        /// <summary>設定データを取得する（非同期編集用）</summary>
        T& Get() { return mData; }

        /// <summary>設定データを取得する（読み取り専用）</summary>
        const T& Get() const { return mData; }

        /// <summary>未保存の変更があるかどうか</summary>
        bool IsDirty() const { return mDirty; }

        /// <summary>ファイルパスを取得する</summary>
        const std::string& GetFilePath() const { return mFilePath; }

        /// <summary>直近のメッセージを取得する</summary>
        const std::string& GetLastMessage() const { return mLastMessage; }

        /// <summary>変更通知コールバックを設定する</summary>
        void SetOnChanged(std::function<void(const T&)> cb) { mOnChanged = std::move(cb); }

        /// <summary>外部エディタ（ConfigEditor）が値を変更したことを通知する</summary>
        void NotifyChanged()
        {
            mDirty = true;
            if (mOnChanged) mOnChanged(mData);
        }

    private:
        T                             mData{};
        std::string                   mFilePath;
        std::function<void(const T&)> mOnChanged;
        bool                          mDirty = false;
        std::string                   mLastMessage;
    };
}