#pragma once

#include<memory>
#include<string>

namespace data
{
	struct GameSettingsData;
	template<typename T> class ConfigEditor;
}

namespace debug
{
	/// <summary>
	/// GameSettingsData(音量・操作説明の既読フラグなど)をGUI上で編集・保存・リセットするデバッグパネル。
	/// data::ConfigEditorをそのまま使う。生成時にImGuiManagerへ登録し、破棄時に解除する。
	/// </summary>
	class GameSettingsDebugPanel
	{
	public:
		/// <param name="debugKey">ImGuiManager 登録・解除に使うキー(シーンごとに一意にすること)</param>
		explicit GameSettingsDebugPanel(std::string debugKey = "GameSettingsDebug");
		~GameSettingsDebugPanel();

		GameSettingsDebugPanel(const GameSettingsDebugPanel&) = delete;
		GameSettingsDebugPanel& operator=(const GameSettingsDebugPanel&) = delete;

	private:
		void Draw();

		std::unique_ptr<data::ConfigEditor<data::GameSettingsData>> mEditor;
		std::string mDebugKey;
	};
}
