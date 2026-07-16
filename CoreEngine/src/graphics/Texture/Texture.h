#pragma once

#include<Utility/Export/Export.h>
#include<graphics/Dx12/Dx12Type.h>
#include<graphics/GraphicsDescriptorHeap/GraphicsDescriptorHeap.h>

#include<filesystem>

namespace graphics
{
	class ENGINE_API Texture
	{
	public:
		Texture();
		virtual ~Texture();

		/// <summary>
		/// ���\�[�X�̍쐬
		/// </summary>
		/// <param name="FilePath">�e�N�X�`���t�@�C���̃p�X</param>
		/// <returns>�쐬�ɐ��������ꍇ��true�A���s�����ꍇ��false</returns>
		bool Create(const std::filesystem::path& FilePath, bool isSRGB = false);

		/// <summary>
		/// ���\�[�X�̉��
		/// </summary>
		void Release();

		/// <summary>
		/// ���蓖�Ă�ꂽ�C���f�b�N�X
		/// </summary>
		/// <returns></returns>
		uint32_t GetDescriptorIndex() const;

		/// <summary>
		/// ���蓖�Ă�ꂽGpu�n���h���̎擾
		/// </summary>
		/// <returns></returns>
		D3D12_GPU_DESCRIPTOR_HANDLE GetGpuHandle() const;

		/// <summary>
		/// �e�N�X�`���̕�
		/// </summary>
		/// <returns></returns>
		float GetWidth()const;
		/// <summary>
		/// �e�N�X�`���̍���
		/// </summary>
		/// <returns></returns>
		float GetHeight()const;

		/// <summary>
		/// ���\�[�X�̎擾
		/// </summary>
		/// <returns></returns>
		ID3D12Resource* GetResource() const;

		bool IsValid()   const { return mSrvHeap.IsValid(); }
	private:
		/// <summary>
		/// ���\�[�X
		/// </summary>
		Resource mResource;
		/// <summary>
		/// ���������蓖�ď��
		/// </summary>
		MAAllocation mAllocation;

		/// <summary>
		/// SRV�p�̃f�B�X�N���v�^�X���b�g
		/// </summary>
		graphics::GDescriptorHeap mSrvHeap;

		/// <summary>
		/// �e�N�X�`������
		/// </summary>
		float mWidth;
		/// <summary>
		/// �e�N�X�`���c��
		/// </summary>
		float mHeight;
	};
}