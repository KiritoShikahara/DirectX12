#pragma once

#include"Pad.h"
#include <mutex>
#include <vector>

namespace sys
{
    /// <summary>
    /// 複数ゲームパッドの接続管理クラス
    /// WGI の GamepadAdded/GamepadRemoved イベントをスレッドセーフに処理する
    /// </summary>
	class PadManager
	{
    public:
        PadManager();
        ~PadManager() = default;

        PadManager(const PadManager&) = delete;
        PadManager& operator=(const PadManager&) = delete;

        /// <summary>
		/// 更新：接続中のゲームパッドの状態を更新する
        /// </summary>
        void Update();

		// ボタン入力クエリ：指定したゲームパッドのボタン状態を返す
        [[nodiscard]] bool IsPressed(ePadButton button, ePadIndex index = ePadIndex::Pad1) const;
        [[nodiscard]] bool IsHeld(ePadButton button, ePadIndex index = ePadIndex::Pad1) const;
        [[nodiscard]] bool IsReleased(ePadButton button, ePadIndex index = ePadIndex::Pad1) const;

		// スティック入力クエリ：指定したゲームパッドのスティック状態を返す
        [[nodiscard]] DirectX::XMFLOAT2 GetLeftStick3D(ePadIndex index = ePadIndex::Pad1) const;
        [[nodiscard]] DirectX::XMFLOAT2 GetRightStick3D(ePadIndex index = ePadIndex::Pad1) const;
        [[nodiscard]] DirectX::XMFLOAT2 GetLeftStick2D(ePadIndex index = ePadIndex::Pad1) const;
        [[nodiscard]] DirectX::XMFLOAT2 GetRightStick2D(ePadIndex index = ePadIndex::Pad1) const;

        /// <summary>接続中のパッドインデックス一覧を返す (最大 ePadIndex::Count 台)</summary>
        [[nodiscard]] std::vector<ePadIndex> GetAvailableIndices() const;

        /// <summary>接続台数</summary>
        [[nodiscard]] size_t GetConnectedCount() const;

        /// <summary>接続中のいずれかのパッドに入力があったか</summary>
        [[nodiscard]] bool IsAnyInput() const;

        /// <summary>ImGuiのデバック表示</summary>
        void ImGuiUpdate();

    private:
        /// <summary>インデックスが有効範囲内か確認する</summary>
        [[nodiscard]] bool IsValidIndex(uint8_t index) const;

        /// <summary>ボタンコードが有効か確認する</summary>
        [[nodiscard]] static bool IsValidButtonCode(ePadButton button);
            
        mutable std::mutex mMutex;  // GamepadAdded/Removed はバックグラウンドスレッドから来る
        std::vector<Pad>   mPads;
	};
}


