#include"pch.h"
#include "KeyBoard.h"

namespace sys
{
    // -----------------------------------------------------------------------
 //  コンストラクタ
 // -----------------------------------------------------------------------
    Keyboard::Keyboard()
    {
        mCurrKeys.fill(false);
        mPrevKeys.fill(false);
    }

    // -----------------------------------------------------------------------
    //  ウィンドウメッセージ処理
    // -----------------------------------------------------------------------
    bool Keyboard::ProcessEvent(UINT message, WPARAM vkCode)
    {
        if (message == WM_KEYDOWN || message == WM_SYSKEYDOWN)
        {
            SetKeyState(vkCode, true);
            return true;
        }
        if (message == WM_KEYUP || message == WM_SYSKEYUP)
        {
            SetKeyState(vkCode, false);
            return true;
        }
        return false;
    }

    // -----------------------------------------------------------------------
    //  フレーム更新
    // -----------------------------------------------------------------------
    void Keyboard::Update()
    {
        mPrevKeys = mCurrKeys;
    }

    // -----------------------------------------------------------------------
    //  状態クエリ
    // -----------------------------------------------------------------------
    bool Keyboard::IsPressed(eKeyCode keyCode) const
    {
        if (!IsValid(keyCode)) return false;
        const int idx = static_cast<int>(keyCode);
        return mCurrKeys[idx] && !mPrevKeys[idx];
    }

    bool Keyboard::IsHeld(eKeyCode keyCode) const
    {
        if (!IsValid(keyCode)) return false;
        return mCurrKeys[static_cast<int>(keyCode)];
    }

    bool Keyboard::IsReleased(eKeyCode keyCode) const
    {
        if (!IsValid(keyCode)) return false;
        const int idx = static_cast<int>(keyCode);
        return !mCurrKeys[idx] && mPrevKeys[idx];
    }

    // -----------------------------------------------------------------------
    //  private ヘルパー
    // -----------------------------------------------------------------------
    void Keyboard::SetKeyState(WPARAM vkCode, bool isDown)
    {
        const eKeyCode code = ToKeyCode(vkCode);
        if (IsValid(code))
        {
            mCurrKeys[static_cast<int>(code)] = isDown;
        }
    }

    bool Keyboard::IsValid(eKeyCode keyCode)
    {
        return keyCode > eKeyCode::Unknown && keyCode < eKeyCode::Count;
    }

    eKeyCode Keyboard::ToKeyCode(WPARAM vk)
    {
        switch (vk)
        {
            // A ~ Z
        case 'A': return eKeyCode::A;
        case 'B': return eKeyCode::B;
        case 'C': return eKeyCode::C;
        case 'D': return eKeyCode::D;
        case 'E': return eKeyCode::E;
        case 'F': return eKeyCode::F;
        case 'G': return eKeyCode::G;
        case 'H': return eKeyCode::H;
        case 'I': return eKeyCode::I;
        case 'J': return eKeyCode::J;
        case 'K': return eKeyCode::K;
        case 'L': return eKeyCode::L;
        case 'M': return eKeyCode::M;
        case 'N': return eKeyCode::N;
        case 'O': return eKeyCode::O;
        case 'P': return eKeyCode::P;
        case 'Q': return eKeyCode::Q;
        case 'R': return eKeyCode::R;
        case 'S': return eKeyCode::S;
        case 'T': return eKeyCode::T;
        case 'U': return eKeyCode::U;
        case 'V': return eKeyCode::V;
        case 'W': return eKeyCode::W;
        case 'X': return eKeyCode::X;
        case 'Y': return eKeyCode::Y;
        case 'Z': return eKeyCode::Z;

            // 数字キー
        case '0': return eKeyCode::Num0;
        case '1': return eKeyCode::Num1;
        case '2': return eKeyCode::Num2;
        case '3': return eKeyCode::Num3;
        case '4': return eKeyCode::Num4;
        case '5': return eKeyCode::Num5;
        case '6': return eKeyCode::Num6;
        case '7': return eKeyCode::Num7;
        case '8': return eKeyCode::Num8;
        case '9': return eKeyCode::Num9;

            // 制御キー
        case VK_ESCAPE:    return eKeyCode::Escape;
        case VK_LCONTROL:  return eKeyCode::LControl;
        case VK_LSHIFT:    return eKeyCode::LShift;   // 修正: VK_SHIFT → VK_LSHIFT
        case VK_LMENU:     return eKeyCode::LAlt;
        case VK_LWIN:      return eKeyCode::LSystem;
        case VK_RCONTROL:  return eKeyCode::RControl;
        case VK_RSHIFT:    return eKeyCode::RShift;
        case VK_RMENU:     return eKeyCode::RAlt;
        case VK_RWIN:      return eKeyCode::RSystem;
        case VK_APPS:      return eKeyCode::Menu;
        case VK_SPACE:     return eKeyCode::Space;
        case VK_RETURN:    return eKeyCode::Enter;
        case VK_BACK:      return eKeyCode::Backspace;
        case VK_TAB:       return eKeyCode::Tab;
        case VK_PAUSE:     return eKeyCode::Pause;

            // 記号キー
        case VK_OEM_4:     return eKeyCode::LBracket;   // [
        case VK_OEM_6:     return eKeyCode::RBracket;   // ]
        case VK_OEM_1:     return eKeyCode::Semicolon;  // ;
        case VK_OEM_COMMA: return eKeyCode::Comma;      // ,
        case VK_OEM_PERIOD:return eKeyCode::Period;      // .
        case VK_OEM_7:     return eKeyCode::Apostrophe; // '
        case VK_OEM_2:     return eKeyCode::Slash;      // /
        case VK_OEM_5:     return eKeyCode::Backslash;  /* \ */
        case VK_OEM_3:     return eKeyCode::Grave;      // `
        case VK_OEM_PLUS:  return eKeyCode::Equal;      // =
        case VK_OEM_MINUS: return eKeyCode::Hyphen;     // -

            // ナビゲーション
        case VK_PRIOR:  return eKeyCode::PageUp;
        case VK_NEXT:   return eKeyCode::PageDown;
        case VK_END:    return eKeyCode::End;
        case VK_HOME:   return eKeyCode::Home;
        case VK_INSERT: return eKeyCode::Insert;
        case VK_DELETE: return eKeyCode::Delete;
        case VK_LEFT:   return eKeyCode::Left;
        case VK_RIGHT:  return eKeyCode::Right;
        case VK_UP:     return eKeyCode::Up;
        case VK_DOWN:   return eKeyCode::Down;

            // テンキー
        case VK_NUMPAD0:  return eKeyCode::Numpad0;
        case VK_NUMPAD1:  return eKeyCode::Numpad1;
        case VK_NUMPAD2:  return eKeyCode::Numpad2;
        case VK_NUMPAD3:  return eKeyCode::Numpad3;
        case VK_NUMPAD4:  return eKeyCode::Numpad4;
        case VK_NUMPAD5:  return eKeyCode::Numpad5;
        case VK_NUMPAD6:  return eKeyCode::Numpad6;
        case VK_NUMPAD7:  return eKeyCode::Numpad7;
        case VK_NUMPAD8:  return eKeyCode::Numpad8;
        case VK_NUMPAD9:  return eKeyCode::Numpad9;
        case VK_ADD:      return eKeyCode::Add;
        case VK_SUBTRACT: return eKeyCode::Subtract;
        case VK_MULTIPLY: return eKeyCode::Multiply;
        case VK_DIVIDE:   return eKeyCode::Divide;
        case VK_DECIMAL:  return eKeyCode::NumpadPeriod; // 修正: Period との衝突を解消

            // ファンクションキー
        case VK_F1:  return eKeyCode::F1;
        case VK_F2:  return eKeyCode::F2;
        case VK_F3:  return eKeyCode::F3;
        case VK_F4:  return eKeyCode::F4;
        case VK_F5:  return eKeyCode::F5;
        case VK_F6:  return eKeyCode::F6;
        case VK_F7:  return eKeyCode::F7;
        case VK_F8:  return eKeyCode::F8;
        case VK_F9:  return eKeyCode::F9;
        case VK_F10: return eKeyCode::F10;
        case VK_F11: return eKeyCode::F11;
        case VK_F12: return eKeyCode::F12;
        case VK_F13: return eKeyCode::F13;
        case VK_F14: return eKeyCode::F14;
        case VK_F15: return eKeyCode::F15;

        default: return eKeyCode::Unknown;
        }
    }
}