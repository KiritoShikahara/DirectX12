#pragma once

#include <Utility/Export/Export.h>
#include <entt/entt.hpp>
#include <string>

namespace ecs
{
    /// <summary>
    /// PlaceableTag を持つエンティティの Save と Load をまとめて行うクラス
    /// </summary>
    class ENGINE_API EditorSerialization
    {
    public:
        EditorSerialization() = delete;

        /// <summary>PlaceableTag エンティティを JSON 文字列へシリアライズする</summary>
        static std::string SerializeToString(entt::registry& registry);

        /// <summary>JSON 文字列から PlaceableTag エンティティを復元する</summary>
        static void DeserializeFromString(entt::registry& registry, const std::string& json);

        /// <summary>ファイルへ保存する</summary>
        static bool SaveToFile(entt::registry& registry, const std::string& filePath);

        /// <summary>ファイルから読み込む</summary>
        static bool LoadFromFile(entt::registry& registry, const std::string& filePath);

    private:
        /// <summary>PlaceableTag エンティティを全て破棄する</summary>
        static void ClearPlaceableEntities(entt::registry& registry);
    };
}