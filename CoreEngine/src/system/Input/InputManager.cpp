#include"pch.h"
#include"InputManager.h"

#include<Utility/config/DebugConfig.h>

namespace sys
{
    bool InputManager::Initialize()
    {
        mPadManager = std::make_unique<PadManager>();
        mMouse = std::make_unique<Mouse>();
        mKeyboard = std::make_unique<Keyboard>();

        // ----------------------------------------------------------------
        //  デフォルトのアクションマッピング
        //  将来的には外部ファイルから読み込む想定
        //
        //  各デバイスは vector なので、波カッコの中に複数コードを並べれば良い。
        //  例: Keys = {Up, W} で上矢印とWどちらでも判定が通る。
        // ----------------------------------------------------------------
        AddAction("Sprint", { { eKeyCode::LShift }, { ePadButton::L1 }, {} });
        AddAction("Select", { { eKeyCode::Space }, { ePadButton::A }, { eMouseButton::Left } });
        AddAction("Cancel", { { eKeyCode::Escape }, { ePadButton::B }, {} });
        AddAction("Attack", { {}, { ePadButton::R2 }, { eMouseButton::Left } });
        AddAction("Interact", { { eKeyCode::F }, { ePadButton::X }, {} });
        AddAction("MoveRight", { { eKeyCode::D }, { ePadButton::DPadRight }, {} });
        AddAction("MoveLeft", { { eKeyCode::A }, { ePadButton::DPadLeft }, {} });
        AddAction("Skill1", { { eKeyCode::Q }, { ePadButton::L1 }, {} });
        AddAction("Skill2", { { eKeyCode::E }, { ePadButton::R1 }, {} });
        AddAction("Option", { { eKeyCode::Escape }, { ePadButton::Menu }, {} });

        /*
        * ImGuiに登録
        */
#ifdef ENABLE_INPUT_DEBUG_SHOW
        sys::ImGuiManager::Get().AddDebugUI([this]()
            {
                mPadManager->ImGuiUpdate();
            });

        sys::ImGuiManager::Get().AddDebugUI([this]()
            {
                mKeyboard->ImGuiUpdate();
            });


#endif // ENABLE_INPUT_DEBUG_SHOW



        mIsInitialized = true;

        return true;
    }

    void InputManager::Update()
    {
        mPadManager->Update();
        mMouse->Update();
        mKeyboard->Update();
    }

    // -----------------------------------------------------------------------
    //  ウィンドウメッセージ
    // -----------------------------------------------------------------------
    bool InputManager::ProcessEvent(UINT message, WPARAM wParam, LPARAM lParam)
    {
        if (!mIsInitialized) return false;

        if (mKeyboard->ProcessEvent(message, wParam))  return true;
        if (mMouse->ProcessEvent(message, wParam, lParam)) return true;
        return false;
    }

    // -----------------------------------------------------------------------
    //  軸入力
    // -----------------------------------------------------------------------
    DirectX::XMFLOAT2 InputManager::GetMoveAxis() const
    {
        using namespace DirectX;

        // キーボード入力
        XMFLOAT2 keyInput{ 0.0f, 0.0f };
        if (mKeyboard->IsHeld(eKeyCode::D)) keyInput.x += 1.0f;
        if (mKeyboard->IsHeld(eKeyCode::A)) keyInput.x -= 1.0f;
        if (mKeyboard->IsHeld(eKeyCode::W)) keyInput.y += 1.0f;
        if (mKeyboard->IsHeld(eKeyCode::S)) keyInput.y -= 1.0f;

        // パッド左スティック (3D空間向け)
        const XMFLOAT2 padInput = mPadManager->GetLeftStick3D();

        // 合成してから正規化 (斜め入力で速度が増さないよう長さを 1 に制限)
        XMVECTOR vec = XMVectorAdd(
            XMLoadFloat2(&keyInput),
            XMLoadFloat2(&padInput));

        const float lenSq = XMVectorGetX(XMVector2LengthSq(vec));
        if (lenSq > 1.0f)
        {
            vec = XMVector2Normalize(vec);
        }

        XMFLOAT2 result;
        XMStoreFloat2(&result, vec);
        return result;
    }

    DirectX::XMFLOAT2 InputManager::GetLookAxis() const
    {
        using namespace DirectX;

        // マウス (Y 軸を反転して 3D カメラの慣習に合わせる)
        const XMFLOAT2 mouseDelta = mMouse->GetDeltaPosition();
        const XMFLOAT2 mouseInput{ mouseDelta.x, -mouseDelta.y };

        // パッド右スティック
        const XMFLOAT2 padInput = mPadManager->GetRightStick3D();

        XMVECTOR vec = XMVectorAdd(
            XMLoadFloat2(&mouseInput),
            XMLoadFloat2(&padInput));

        // わずかな揺れは無視する
        constexpr float kEpsilon = 0.0001f;
        if (XMVectorGetX(XMVector2LengthSq(vec)) > kEpsilon)
        {
            vec = XMVector2Normalize(vec);
        }

        XMFLOAT2 result;
        XMStoreFloat2(&result, vec);
        return result;
    }

    // -----------------------------------------------------------------------
    //  アクションマッピング
    // -----------------------------------------------------------------------
    void InputManager::AddAction(const std::string& actionName, const ActionBinding& bind)
    {
        if (actionName.empty()) return;
        mActionMaps[actionName] = bind;
    }

    bool InputManager::IsActionPressed(const std::string& actionName) const
    {
        const auto it = mActionMaps.find(actionName);
        if (it == mActionMaps.end()) return false;

        const auto& bind = it->second;

        for (const auto key : bind.Keys)
        {
            if (mKeyboard->IsPressed(key)) return true;
        }
        for (const auto pad : bind.Pads)
        {
            if (mPadManager->IsPressed(pad)) return true;
        }
        for (const auto mouse : bind.Mouses)
        {
            if (mMouse->IsPressed(mouse)) return true;
        }
        return false;
    }

    bool InputManager::IsActionHeld(const std::string& actionName) const
    {
        const auto it = mActionMaps.find(actionName);
        if (it == mActionMaps.end()) return false;

        const auto& bind = it->second;

        for (const auto key : bind.Keys)
        {
            if (mKeyboard->IsHeld(key)) return true;
        }
        for (const auto pad : bind.Pads)
        {
            if (mPadManager->IsHeld(pad)) return true;
        }
        for (const auto mouse : bind.Mouses)
        {
            if (mMouse->IsHeld(mouse)) return true;
        }
        return false;
    }

    bool InputManager::IsActionReleased(const std::string& actionName) const
    {
        const auto it = mActionMaps.find(actionName);
        if (it == mActionMaps.end()) return false;

        const auto& bind = it->second;

        for (const auto key : bind.Keys)
        {
            if (mKeyboard->IsReleased(key)) return true;
        }
        for (const auto pad : bind.Pads)
        {
            if (mPadManager->IsReleased(pad)) return true;
        }
        for (const auto mouse : bind.Mouses)
        {
            if (mMouse->IsReleased(mouse)) return true;
        }
        return false;
    }
}