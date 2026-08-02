#pragma once

#include<Data/Storage/Reflection.h>
#include<Data/Storage/Registry/ConfigRegistry.h>
#include<filesystem>
#include<string>


namespace data
{
	struct WindowConfig
	{
		std::string Title = "SK Engine";
		int Width = 1280;
		int Height = 720;
		bool IsFullScreen = false;
		bool ShowCursor = true;

		std::filesystem::path GetTitle() const { return Title; }
		void SetTitle(const std::filesystem::path& p) { Title = p.string(); }

		REFLECT_BEGIN(WindowConfig, "window")
			REFLECT_FIELD_STR(Title)
			REFLECT_FIELD_INT(Width)
			REFLECT_FIELD_INT(Height)
			REFLECT_FIELD_BOOL(IsFullScreen)
			REFLECT_FIELD_BOOL(ShowCursor)
			REFLECT_END()
	};

	template<typename WindowContext>
	inline void ApplyWindowConfig(const WindowConfig& cfg, WindowContext& ctx)
	{
		ctx.Title = cfg.GetTitle();
		ctx.Width = cfg.Width;
		ctx.Height = cfg.Height;
		ctx.IsFullScreen = cfg.IsFullScreen;
		ctx.ShowCursor = cfg.ShowCursor;
	}

}

// 
REFLECT_REGISTER(data::WindowConfig);

inline data::WindowConfig LoadWindowConfig(const std::filesystem::path& filePath = "Config/window.json")
{
	data::ConfigManager<data::WindowConfig> mgr(filePath.string());
	mgr.Load();
	return mgr.Get();
}

inline void RegisterWindowConfig(const std::filesystem::path& filePath = "Config/window.json")
{
	data::ConfigRegistry::Get().Register<data::WindowConfig>(filePath.string());
}
