#pragma once

#include<Utility/Export/Export.h>
#include<graphics/Dx12/Dx12Type.h>
#include<graphics/GraphicsDescriptorHeap/GraphicsDescriptorHeap.h>

#include<DirectXTex.h>
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

		// Decoded CPU-side image data, produced by LoadImageData() and consumed by
		// CreateFromImageData(). Kept as a separate struct so the (potentially slow)
		// file read + decode + mip generation can run on a worker thread, while GPU
		// resource creation (CreateFromImageData) stays on the caller's thread.
		struct ImageData
		{
			bool Success = false;
			DirectX::TexMetadata MetaData = {};
			DirectX::ScratchImage ScratchImage;
		};

		// CPU-only: file read + DirectXTex decode + mip generation. No D3D12 calls,
		// safe to call concurrently for different files from multiple threads
		// (see TextureManager::PreloadBatch).
		static ImageData LoadImageData(const std::filesystem::path& FilePath, bool isSRGB);

		// GPU-side: takes already-decoded ImageData and creates the D3D12 resource,
		// uploads it, and builds the SRV. Not thread-safe with respect to other
		// Texture instances sharing the same device/allocator; call from one thread
		// at a time (matches the existing single-threaded upload path).
		bool CreateFromImageData(const std::filesystem::path& FilePath, bool isSRGB, const ImageData& imageData);

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