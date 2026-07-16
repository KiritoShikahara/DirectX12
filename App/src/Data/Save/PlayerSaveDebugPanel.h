#pragma once

#include<memory>
#include<string>

namespace data
{
	struct PlayerSaveData;
	template<typename T> class ConfigEditor;
}

namespace debug
{
	/// <summary>
	/// プレイヤーの永続的な進行状況(PlayerSaveData：所持ゴールド・ステータス強化レベル)の
	/// デバッグパネル。data::ConfigEditor(Load/Save/Resetボタン+ドラッグ編集可能なフィールド一覧)
	/// をそのまま使い、ImGui上でゴールドを直接いじれるようにする。
	/// IUserSystemは継承せず、生成時にImGuiManagerへ登録・破棄時に解除する。
	/// </summary>
	class PlayerSaveDebugPanel
	{
	public:
		/// <param name="debugKey">ImGuiManager 登録・解除に使うキー（シーンごとに一意にすること）</param>
		explicit PlayerSaveDebugPanel(std::string debugKey = "PlayerSaveDebug");
		~PlayerSaveDebugPanel();

		PlayerSaveDebugPanel(const PlayerSaveDebugPanel&) = delete;
		PlayerSaveDebugPanel& operator=(const PlayerSaveDebugPanel&) = delete;

	private:
		void Draw();

		std::unique_ptr<data::ConfigEditor<data::PlayerSaveData>> mEditor;
		std::string mDebugKey;
	};
}
