#pragma once

#include<Utility/Singleton/Singleton.hpp>
#include<Utility/Export/Export.h>

#include<filesystem>
#include<unordered_map>
#include<mutex>

namespace graphics
{

	class Texture;

	class ENGINE_API TextureManager : public utility::Singleton<TextureManager>
	{
		SINGLETON_CLASS(TextureManager);
	public:
		SINGLETON_ACCESSOR(TextureManager);

		/// <summary>
		/// �e�N�X�`���̎擾�A�~���[�h�Ȃ烍�[�h����B
		/// </summary>
		/// <param name="FilePath">�t�@�C���p�X</param>
		/// <param name="isSRGB">true�Ȃ�SRV��sRGB�Ƃ��ĉ��߂���(Albedo/Emissive�Ȃǐ F�e�N�X�`���p)</param>
		/// <returns>�Q�Ɨp�̃|�C���^</returns>
		Texture* GetOrLoad(const std::filesystem::path& FilePath, bool isSRGB = false);

		/// <summary>
		/// ���ׂẴe�N�X�`�����������B
		/// </summary>
		void Clear();
	private:
		/// <summary>
		/// ���\�[�X�S��
		/// </summary>
		std::unordered_map<std::string, std::unique_ptr<Texture>> mResources;

		/// <summary>
		/// �r������
		/// </summary>
		std::mutex mMutex;

	};

}

