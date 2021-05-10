#pragma once

#include <string>

#include "sge_core/sgecore_api.h"
#include "sge_utils/math/vec2.h"
#include "sge_utils/sge_utils.h"

namespace sge {

struct SGE_CORE_API GamepadState {
	bool hooked = false; // True if the gamepad is still hooked to the system.
	bool hadInputThisPoll = false;

	enum Button : int {
		btn_a,
		btn_b,
		btn_x,
		btn_y,
		btn_shoulderL,
		btn_shoulderR,

		btn_up,
		btn_down,
		btn_left,
		btn_right,

		btn_thumbL,
		btn_thumbR,

		btn_back,
		btn_start,

		btn_numButtons
	};

	bool isBtnDown(Button btn) const;
	bool isBtnUp(Button btn) const;
	bool isBtnPressed(Button btn) const;
	bool isBtnReleased(Button btn) const;

	unsigned char btnState[btn_numButtons] = {0};

	vec2f axisL = vec2f(0.f);
	vec2f axisR = vec2f(0.f);

	float triggerL = 0.f;
	float triggerR = 0.f;

	void Advance(bool sdlStyleDontZeroTheThumbsticks);

	vec2f getInputDir(bool const includeDPad) const;
};

enum Key {
	Key_MouseLeft,
	Key_MouseRight,
	Key_MouseMiddle,

	Key_LShift,
	Key_RShift,

	Key_Up,
	Key_Down,
	Key_Left,
	Key_Right,

	Key_Escape,

	Key_0,
	Key_1,
	Key_2,
	Key_3,
	Key_4,
	Key_5,
	Key_6,
	Key_7,
	Key_8,
	Key_9,
	Key_Backspace,
	Key_Enter,
	Key_Tab,

	Key_PageUp,
	Key_PageDown,
	Key_Insert,
	Key_Home,
	Key_End,
	Key_Delete,

	Key_LCtrl,
	Key_RCtrl,
	Key_LAlt,
	Key_RAlt,

	Key_F1,
	Key_F2,
	Key_F3,
	Key_F4,
	Key_F5,
	Key_F6,
	Key_F7,
	Key_F8,
	Key_F9,
	Key_F10,
	Key_F11,
	Key_F12,

	Key_A,
	Key_B,
	Key_C,
	Key_D,
	Key_E,
	Key_F,
	Key_G,
	Key_H,
	Key_I,
	Key_J,
	Key_K,
	Key_L,
	Key_M,
	Key_N,
	Key_O,
	Key_P,
	Key_Q,
	Key_R,
	Key_S,
	Key_T,
	Key_U,
	Key_V,
	Key_W,
	Key_X,
	Key_Y,
	Key_Z,

	Key_Space,

	KeyboardAndMouse_NumElements,

	Key_NumElements,
};

struct SGE_CORE_API InputState {
  public:
	InputState();

	bool m_wasActiveWhilePolling; // True if the window was active while polling.
	bool m_hadkeyboardOrMouseInputThisPoll;
	bool m_isCursorRelative = false;
	vec2f m_cursorClient; // The position of the cursor in client space in pixels.
	vec2f m_cursorDomain; // The position of the cursor in some specific domain. Used when we pass the input structure to some other
	                      // structure that doesn't where it's origin of the curosr should be.
	vec2f m_cursorDomainSize = vec2f(0.f);
	vec2f m_cursorMotion; // The moution of the cursor. Not the actual moution but the difference in the coords.
	int m_wheelCount;     // The amount of mouse wheel ticks this poll.
	std::string m_inputText;

	// The state of each character.
	// 0 bit - the current state of the button.
	// 1 bit - the previous state of the button.
	// 2-7bit - undefined.
	unsigned char m_keyStates[KeyboardAndMouse_NumElements];

	// Gamepads
	GamepadState xinputDevicesState[4];
	GamepadState winapiGamepads[1]; // TODO: add support for more than one gamepad.

  public:
	// Moves the crrent input state into previous
	void Advance();

	void setCursorPos(const vec2f& c) {
		if (m_cursorClient != c) {
			m_hadkeyboardOrMouseInputThisPoll = true;
			m_cursorClient = c;
		}

		m_cursorDomain = c;
	}

	void addInputText(const char c) {
		m_hadkeyboardOrMouseInputThisPoll = true;
		m_inputText.push_back(c);
	}
	void addKeyUpOrDown(Key key, bool isDown) {
		m_hadkeyboardOrMouseInputThisPoll = true;
		if (key >= 0 && key < Key_NumElements) {
			if (isDown) {
				m_keyStates[key] |= 1;
			} else {
				m_keyStates[key] &= ~1;
			}
		}
	}
	void addMouseWheel(int v) {
		m_hadkeyboardOrMouseInputThisPoll = true;
		m_wheelCount = v;
	}

	void addMouseMotion(const vec2f& moution) { m_cursorMotion += moution; }
	bool isCursorRelative() const { return m_isCursorRelative; }
	void setCusorIsRelative(bool isRelative) { m_isCursorRelative = isRelative; }

	void setWasActiveDuringPoll(bool v) { m_wasActiveWhilePolling = v; }
	bool wasActiveWhilePolling() const { return m_wasActiveWhilePolling; }

	const char* GetText() const { return m_inputText.c_str(); }

	const vec2f& GetCursorPos() const { return m_cursorDomain; }
	const vec2f getCursorPosUV() const { return m_cursorDomain / m_cursorDomainSize; }
	const vec2f& GetCursorMotion() const { return m_cursorMotion; }
	int GetWheelCount() const { return m_wheelCount; }

	bool IsKeyDown(Key key) const { return (bool)(m_keyStates[key] & 1); }
	bool IsKeyUp(Key key) const { return !(bool)(m_keyStates[key] & 1); }
	bool IsKeyPressed(Key key) const { return (m_keyStates[key] & 3) == 1; } // Returns true if the target key was just pressed.
	bool IsKeyReleased(Key key) const { return (m_keyStates[key] & 3) == 2; }

	bool isKeyCombo(Key k0, Key k1) const { return (IsKeyDown(k0) && IsKeyReleased(k1)); }

	bool AnyArrowKeyDown(bool includeWASD) const {
		bool res = IsKeyDown(Key_Left) || IsKeyDown(Key_Right) || IsKeyDown(Key_Up) || IsKeyDown(Key_Down);
		if (includeWASD) {
			res |= IsKeyDown(Key_A) || IsKeyDown(Key_D) || IsKeyDown(Key_W) || IsKeyDown(Key_S);
		}

		return res;
	}

	bool AnyWASDDown() const { return IsKeyDown(Key_W) || IsKeyDown(Key_A) || IsKeyDown(Key_S) || IsKeyDown(Key_D); }

	/// Retrieves the vetor pointed by the arrow keys using +X right, +Y up.
	vec2f GetArrowKeysDir(const bool normalize, bool includeWASD = false, int useGamePadAtIndex = -1) const;

	const GamepadState* getHookedGemepad(const int playerIndex) const;

	const GamepadState& getXInputDevice(int index) const { return xinputDevicesState[index]; }
};

} // namespace sge
