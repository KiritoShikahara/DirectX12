#pragma once

#include  <utility/Singleton/Singleton.hpp>
#include <Utility/Export/Export.h>
#include<vector>
#include<mutex>

#include "Dx12Type.h"

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dxguid.lib")
namespace graphics
{
	/// <summary>
	/// Dx12�f�o�C�X�Ǘ�
	/// </summary>
	class ENGINE_API DX12Device : public utility::Singleton<DX12Device>
	{
		DX12Device();
		SINGLETON_CLASS_CUSTOM_CTOR(DX12Device);
	public:
		SINGLETON_ACCESSOR(DX12Device);

		/// <summary>
		/// ������
		/// </summary>
		/// <returns>true:�����@</returns>
		bool Initialize();

		/// <summary>
		/// �I������
		/// </summary>
		/// <returns></returns>
		bool Finalize();

		/// <summary>
		/// Dx12�f�o�C�X�̎擾
		/// </summary>
		ID3D12Device* GetDevice();

		/// <summary>
		/// DXGI�t�@�N�g���[�̎擾
		/// </summary>
		IDXGIFactory7* GetFactory();

		/// <summary>
		/// D3D12MA�A���P�[�^�[�̎擾
		/// </summary>
		D3D12MA::Allocator* GetMAAllocator();

		/// <summary>
		/// GPU�Ƀe�N�X�`�����\�[�X��]������B
		/// ��p�̃A�b�v���[�h�L���[�Ŏ��s���邽�ߕ`�惋�[�v�Ɉˑ����Ȃ��B
		/// �X���b�h�Z�[�t (UploadBufferData �Ɠ��� mUploadMutex �ɂ���Ĕr�����䂳���)�B
		/// </summary>
		/// <param name="pResource">�]���惊�\�[�X</param>
		/// <param name="subresources">�]������T�u���\�[�X�̃f�[�^</param>
		/// <returns>true:����</returns>
		bool UploadTextureData(ID3D12Resource* pResource,
			const std::vector<D3D12_SUBRESOURCE_DATA>& subresources);

		/// <summary>
		/// GPU �Ƀo�b�t�@�f�[�^��]������B
		/// UploadTextureData �Ɠ�������p�A�b�v���[�h�L���[�œ����I�Ɋ�������B
		/// �X���b�h�Z�[�t (������ mutex �ɂ���Ĕr�����䂳���)�B
		/// cmdList �͕s�v�B�`�惋�[�v�Ɉˑ����Ȃ��B
		/// </summary>
		/// <param name="pResource">�]���惊�\�[�X (DEFAULT heap, COPY_DEST ��Ԃō쐬�ς�)</param>
		/// <param name="data">�]������f�[�^�|�C���^ (nullptr �֎~)</param>
		/// <param name="size">�]���o�C�g�� (0 �֎~)</param>
		/// <param name="targetState">�]��������̃��\�[�X���</param>
		bool UploadBufferData(
			ID3D12Resource* pResource,
			const void* data,
			size_t                size,
			D3D12_RESOURCE_STATES targetState = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

	private:
		/// <summary>
		/// �f�o�b�O���C���[�̗L�����i�f�o�b�O�r���h�̂݁j
		/// </summary>
		void DebugLayerOn();

		/// <summary>
		/// DXGI�t�@�N�g���[�̏�����
		/// </summary>
		bool InitializeFactory();

		/// <summary>
		/// �f�o�C�X��D3D12MA�A���P�[�^�[�̏�����
		/// </summary>
		bool InitializeDevice();

		/// <summary>
		/// �A�b�v���[�h��p�R���e�L�X�g�̏�����
		/// �i�R�}���h�L���[�E�A���P�[�^�[�E�R�}���h���X�g�E�t�F���X�j
		/// </summary>
		bool InitializeUploadContext();

	private:
		/// <summary>GPU�Ƃ̒ʐM����</summary>
		Device          mDevice;
		/// <summary>�X���b�v�`�F�C����A�_�v�^�̍쐬�Ɏg��</summary>
		Factory         mFactory;
		/// <summary>D3D12MA�̃������A���P�[�^�[</summary>
		MAAllocator     mMAAllocator;
		/// <summary>���\�[�X�R�ꌟ�m�i�f�o�b�O�r���h�̂ݗL���j</summary>
		DebugDevice     mDebugDevice;

		// ---- �A�b�v���[�h��p�R���e�L�X�g ----
		/// <summary>�A�b�v���[�h��p�R�}���h�L���[�i�`��L���[�ƕ����j</summary>
		CmdQueue        mUploadCmdQueue;
		/// <summary>�A�b�v���[�h��p�R�}���h�A���P�[�^�[</summary>
		CmdAlloc        mUploadAllocator;
		/// <summary>�A�b�v���[�h��p�R�}���h���X�g</summary>
		CmdList         mUploadCmdList;
		/// <summary>�A�b�v���[�h���������p�t�F���X</summary>
		Fence           mUploadFence;
		/// <summary>�A�b�v���[�h�p�t�F���X�J�E���^�[</summary>
		UINT64          mUploadFenceValue = 0;
		/// <summary>�A�b�v���[�h�����҂��C�x���g�n���h��</summary>
		HANDLE          mUploadEvent = nullptr;

		/// <summary>
		/// �A�b�v���[�h�R���e�L�X�g�p�̔r������
		///  UploadTextureData / UploadBufferData �𕡐��X���b�h���瓯���ɌĂ񂾏ꍇ��
        /// mUploadAllocator / mUploadCmdList �ւ̓����A�N�Z�X��h��
		/// </summary>
		std::mutex mUploadMutex;

		bool mDebugLayerEnabled = false;
	};
}


