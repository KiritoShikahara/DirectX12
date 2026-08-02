#include"pch.h"
#include "KeyBoard.h"

#include<Utility/config/DebugConfig.h>

namespace sys
{
    Keyboard::Keyboard()
    {
        mCurrKeys.fill(false);
        mPrevKeys.fill(false);
    }

    // ウィンドウメッセージ
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

    // フレーム更新
    void Keyboard::Update()
    {
        mPrevKeys = mCurrKeys;
    }

    // 状態クエリ
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

    bool Keyboard::IsAnyKeyHeld() const
    {
        for (int i = 0; i < kKeyCount; ++i)
        {
            if (mCurrKeys[i]) return true;
        }
        return false;
    }

    // Privateヘルパー
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

    void Keyboard::ImGuiUpdate() const
    {
#ifdef ENABLE_INPUT_DEBUG_KEYBOARD

        if (ImGui::Begin("KeyBoard"))
        {

            // 押下中のキーをリストアップ
            bool anyHeld = false;
            for (int i = 0; i < kKeyCount; ++i)
            {
                if (!mCurrKeys[i]) continue;

                anyHeld = true;
                const auto code = static_cast<eKeyCode>(i);
                const bool pressed = !mPrevKeys[i];   // このフレームで押した
                const bool released = false;            // held 中なので released にはならない

                if (pressed)
                    ImGui::TextColored({ 0.3f, 1.0f, 0.3f, 1.0f }, "[PRESS]  %s", KeyCodeToString(code));
                else
                    ImGui::TextColored({ 0.9f, 0.9f, 0.9f, 1.0f }, "[HELD]   %s", KeyCodeToString(code));
            }

            // このフレームで離したキー
            for (int i = 0; i < kKeyCount; ++i)
            {
                if (mCurrKeys[i] || !mPrevKeys[i]) continue;
                anyHeld = true;
                const auto code = static_cast<eKeyCode>(i);
                ImGui::TextColored({ 1.0f, 0.5f, 0.3f, 1.0f }, "[RELEASE]%s", KeyCodeToString(code));
            }

            if (!anyHeld)
            {
                ImGui::TextDisabled("(no key input)");
            }
        }

        ImGui::End();
#endif // ENABLE_INPUT_DEBUG_KEYBOARD
    }

    const char* Keyboard::KeyCodeToString(eKeyCode keyCode)
    {
        switch (keyCode)
        {
        case eKeyCode::A: return "A"; case eKeyCode::B: return "B";
        case eKeyCode::C: return "C"; case eKeyCode::D: return "D";
        case eKeyCode::E: return "E"; case eKeyCode::F: return "F";
        case eKeyCode::G: return "G"; case eKeyCode::H: return "H";
        case eKeyCode::I: return "I"; case eKeyCode::J: return "J";
        case eKeyCode::K: return "K"; case eKeyCode::L: return "L";
        case eKeyCode::M: return "M"; case eKeyCode::N: return "N";
        case eKeyCode::O: return "O"; case eKeyCode::P: return "P";
        case eKeyCode::Q: return "Q"; case eKeyCode::R: return "R";
        case eKeyCode::S: return "S"; case eKeyCode::T: return "T";
        case eKeyCode::U: return "U"; case eKeyCode::V: return "V";
        case eKeyCode::W: return "W"; case eKeyCode::X: return "X";
        case eKeyCode::Y: return "Y"; case eKeyCode::Z: return "Z";
        case eKeyCode::Num0: return "0"; case eKeyCode::Num1: return "1";
        case eKeyCode::Num2: return "2"; case eKeyCode::Num3: return "3";
        case eKeyCode::Num4: return "4"; case eKeyCode::Num5: return "5";
        case eKeyCode::Num6: return "6"; case eKeyCode::Num7: return "7";
        case eKeyCode::Num8: return "8"; case eKeyCode::Num9: return "9";
        case eKeyCode::Escape:    return "Escape";
        case eKeyCode::LControl:  return "LCtrl";
        case eKeyCode::LShift:    return "LShift";
        case eKeyCode::LAlt:      return "LAlt";
        case eKeyCode::LSystem:   return "LWin";
        case eKeyCode::RControl:  return "RCtrl";
        case eKeyCode::RShift:    return "RShift";
        case eKeyCode::RAlt:      return "RAlt";
        case eKeyCode::RSystem:   return "RWin";
        case eKeyCode::Menu:      return "Menu";
        case eKeyCode::Space:     return "Space";
        case eKeyCode::Enter:     return "Enter";
        case eKeyCode::Backspace: return "Backspace";
        case eKeyCode::Tab:       return "Tab";
        case eKeyCode::Pause:     return "Pause";
        case eKeyCode::Insert:    return "Insert";
        case eKeyCode::Delete:    return "Delete";
        case eKeyCode::Home:      return "Home";
        case eKeyCode::End:       return "End";
        case eKeyCode::PageUp:    return "PageUp";
        case eKeyCode::PageDown:  return "PageDown";
        case eKeyCode::Left:      return "Left";
        case eKeyCode::Right:     return "Right";
        case eKeyCode::Up:        return "Up";
        case eKeyCode::Down:      return "Down";
        case eKeyCode::F1:  return "F1";  case eKeyCode::F2:  return "F2";
        case eKeyCode::F3:  return "F3";  case eKeyCode::F4:  return "F4";
        case eKeyCode::F5:  return "F5";  case eKeyCode::F6:  return "F6";
        case eKeyCode::F7:  return "F7";  case eKeyCode::F8:  return "F8";
        case eKeyCode::F9:  return "F9";  case eKeyCode::F10: return "F10";
        case eKeyCode::F11: return "F11"; case eKeyCode::F12: return "F12";
        case eKeyCode::F13: return "F13"; case eKeyCode::F14: return "F14";
        case eKeyCode::F15: return "F15";
        case eKeyCode::Numpad0: return "Num0"; case eKeyCode::Numpad1: return "Num1";
        case eKeyCode::Numpad2: return "Num2"; case eKeyCode::Numpad3: return "Num3";
        case eKeyCode::Numpad4: return "Num4"; case eKeyCode::Numpad5: return "Num5";
        case eKeyCode::Numpad6: return "Num6"; case eKeyCode::Numpad7: return "Num7";
        case eKeyCode::Numpad8: return "Num8"; case eKeyCode::Numpad9: return "Num9";
        case eKeyCode::Add:          return "Num+";
        case eKeyCode::Subtract:     return "Num-";
        case eKeyCode::Multiply:     return "Num*";
        case eKeyCode::Divide:       return "Num/";
        case eKeyCode::NumpadPeriod: return "Num.";
        case eKeyCode::LBracket:     return "[";
        case eKeyCode::RBracket:     return "]";
        case eKeyCode::Semicolon:    return ";";
        case eKeyCode::Comma:        return ",";
        case eKeyCode::Period:       return ".";
        case eKeyCode::Apostrophe:   return "'";
        case eKeyCode::Slash:        return "/";
        case eKeyCode::Backslash:    return "\\";
        case eKeyCode::Grave:        return "`";
        case eKeyCode::Equal:        return "=";
        case eKeyCode::Hyphen:       return "-";
        default: return "Unknown";
        }
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
        case VK_DECIMAL:  return eKeyCode::NumpadPeriod;

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