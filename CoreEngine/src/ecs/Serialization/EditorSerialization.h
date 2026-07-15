#pragma once

#include<Utility/Export/Export.h>
#include<entt/entt.hpp>
#include<string>

namespace ecs
{
	/// <summary>
	/// PlaceableTag を持つエンティティ(エディタで配置した主要な配置系のみ)の
	/// Save/Load をまとめて行う。
	///
	/// EditorManager の Play/Stop 用インメモリスナップショットと、
	/// ファイルへの Save Layout はどちらもこのクラスの
	/// SerializeToString/DeserializeFromString を共用する(二重実装しない)。
	/// </summary>
	class ENGINE_API EditorSerialization
	{
	public:
		EditorSerialization() = delete;

		/// <summary>PlaceableTag エンティティを JSON 文字列へシリアライズする</summary>
		static std::string SerializeToString(entt::registry& registry);

		/// <summary>
		/// JSON 文字列から PlaceableTag エンティティを復元する。
		/// 復元前に既存の PlaceableTag エンティティは全て破棄する(重複防止)。
		/// </summary>
		static void DeserializeFromString(entt::registry& registry, const std::string& json);

		/// <summary>ファイルへ保存する。失敗時 false。</summary>
		static bool SaveToFile(entt::registry& registry, const std::string& filePath);

		/// <summary>
		/// ファイルから読み込む。ファイルが存在しない/読み込み失敗時は false を返し、
		/// registry には一切触れない。
		/// </summary>
		static bool LoadFromFile(entt::registry& registry, const std::string& filePath);

	private:
		static void ClearPlaceableEntities(entt::registry& registry);
	};
}
