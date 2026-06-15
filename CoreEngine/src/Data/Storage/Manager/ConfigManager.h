#pragma once

#include"../Loader/JsonSerializer.h"
#include"../Reflection.h"
#include<string>
#include<functional>

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
            : mFilePath(std::move(filePath)) {
        }

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

        void Save()
        {
            JsonSerializer::SaveToFile(mFilePath, mData);
            mLastMessage = "[Config] Saved: " + mFilePath;
            mDirty = false;
        }

        void Reset()
        {
            mData = T{};
            mDirty = true;
            if (mOnChanged) mOnChanged(mData);
        }

        // アクセサ
        T& Get() { return mData; }
        const T& Get()          const { return mData; }
        bool     IsDirty()      const { return mDirty; }
        const std::string& GetFilePath()    const { return mFilePath; }
        const std::string& GetLastMessage() const { return mLastMessage; }

        void SetOnChanged(std::function<void(const T&)> cb) { mOnChanged = std::move(cb); }

        // 外部エディタ（ConfigEditor）が値を変更したことを通知する
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