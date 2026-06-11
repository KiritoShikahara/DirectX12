#include "pch.h"
#include "SceneFactory.h"
#include"../Default/DefaultScene.h"

namespace sys
{
	void SceneFactory::Register(const std::string& name, CreatorFunc creator)
	{
		// 既に存在しているか
		if (mCreators.count(name))
		{
			DEBUG_LOG(sys::eLogLevel::Warning,
				("SceneFactory: \"" + name + "\" is already registered. Overwriting.").c_str());
		}

		// 登録
		mCreators[name] = std::move(creator);
		DEBUG_LOG(sys::eLogLevel::Log,
			("SceneFactory: Registered \"" + name + "\".").c_str());
	}

	std::unique_ptr<IScene> SceneFactory::Create(const std::string& name) const
	{
		auto it = mCreators.find(name);
		if (it != mCreators.end())
		{
			return it->second();
		}

		// 未登録時のフォールバック
		DEBUG_LOG(sys::eLogLevel::Warning,
			("SceneFactory: \"" + name + "\" not found. Falling back to DefaultScene.").c_str());
		return std::make_unique<DefaultScene>();
	}

	std::vector<std::string> SceneFactory::GetRegisteredNames() const
	{
		std::vector<std::string> names;
		names.reserve(mCreators.size());
		for (const auto& [key, _] : mCreators)
		{
			names.push_back(key);
		}
		return names;
	}

	bool SceneFactory::IsRegistered(const std::string& name) const
	{
		return mCreators.count(name) > 0;
	}

	void SceneFactory::SetNextSceneName(const std::string& name)
	{
		mDefaultSceneName = name;
	}

	const std::string& SceneFactory::GetDefaultSceneName()
	{
		return mDefaultSceneName;
	}
}