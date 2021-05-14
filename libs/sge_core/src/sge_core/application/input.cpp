#include "input.h"

namespace sge {

InputState::InputState() {
	for (auto& v : m_keyStates) {
		v = 0;
	}
}

Optional<vec2f>  InputState::hasTouchJustPresedUV(const vec2f& minUV, const vec2f& maxUV) const {
	for (const TouchInput& touch : m_touchInputs) {
		const vec2f touchPosUV = clientToDomainUV(touch.touchPositionPixels);
		bool isIn = touchPosUV.x >= minUV.x && touchPosUV.y >= minUV.y && touchPosUV.x <= maxUV.x && touchPosUV.y <= maxUV.y;
		if (isIn && touch.isJustPressed) {
			return touchPosUV;
		}
	}
	return NullOptional();
}

Optional<vec2f>  InputState::hasTouchJustReleasedUV(const vec2f& minUV, const vec2f& maxUV) const {
	for (const TouchInput& touch : m_touchInputs) {
		const vec2f touchPosUV = clientToDomainUV(touch.touchPositionPixels);
		bool isIn = touchPosUV.x >= minUV.x && touchPosUV.y >= minUV.y && touchPosUV.x <= maxUV.x && touchPosUV.y <= maxUV.y;
		if (isIn && touch.isJustReleased) {
			return touchPosUV;
		}
	}
	return NullOptional();
}

Optional<vec2f>  InputState::hasTouchPressedUV(const vec2f& minUV, const vec2f& maxUV) const {
	for (const TouchInput& touch : m_touchInputs) {
		const vec2f touchPosUV = clientToDomainUV(touch.touchPositionPixels);
		bool isIn = touchPosUV.x >= minUV.x && touchPosUV.y >= minUV.y && touchPosUV.x <= maxUV.x && touchPosUV.y <= maxUV.y;
		if (isIn && !touch.isJustReleased) {
			return touchPosUV;
		}
	}
	return NullOptional();
}

void InputState::addTouch(const TouchInput& newTouch) {
	for (TouchInput& touch : m_touchInputs) {
		if (touch.touchId == newTouch.touchId) {
			touch = newTouch;
			return;
		}
	}
}

TouchInput& InputState::findOrCreateTouchById(const int64 touchId) {
	for (TouchInput& t : m_touchInputs) {
		if (t.touchId == touchId) {
			return t;
		}
	}

	TouchInput newTouch;
	newTouch.touchId = touchId;
	m_touchInputs.push_back(newTouch);

	return m_touchInputs.back();
}

const TouchInput* InputState::getTouchById(const int64 touchId) const {
	for (const TouchInput& t : m_touchInputs) {
		if (t.touchId == touchId) {
			return &t;
		}
	}

	return nullptr;
}

void InputState::Advance() {
	m_hadkeyboardOrMouseInputThisPoll = false;
	m_wasActiveWhilePolling = false;
	m_inputText.clear();
	m_wheelCount = 0;
	m_cursorMotion = vec2f(0.f);

	// Advance the polled input one step ahead, while keeping the current.
	for (auto& state : m_keyStates) {
		state = (state << 1) | (state & 1);
	}

	for (int t = 0; t < SGE_ARRSZ(xinputDevicesState); ++t) {
		xinputDevicesState[t].Advance(false);
	}

	for (int t = 0; t < SGE_ARRSZ(winapiGamepads); ++t) {
		winapiGamepads[t].Advance(false);
	}

	// Keep the cursor position unmodified.
	m_isCursorRelative = false;

	// We cannot clear the touch input here.
	// For example, if the player is holding theirs finger on the screen without moving it,
	// SDL will not generate an event for that.
	// We clear events when on the previous frame were just released. Meaning that they
	// no longer exist and the user had the chance to process them.
	for (int t = 0; t < int(m_touchInputs.size()); ++t) {
		// This would be the second frame the touch exists it cannot be just pressed anymore.
		m_touchInputs[t].isJustPressed = false;

		if (m_touchInputs[t].isJustReleased) {
			m_touchInputs.erase(m_touchInputs.begin() + t);
			--t;
		}
	}
}

void InputState::setCursorPos(const vec2f& c) {
	if (m_cursorClient != c) {
		m_hadkeyboardOrMouseInputThisPoll = true;
		m_cursorClient = c;
	}
}

void InputState::addInputText(const char c) {
	m_hadkeyboardOrMouseInputThisPoll = true;
	m_inputText.push_back(c);
}

void InputState::addKeyUpOrDown(Key key, bool isDown) {
	m_hadkeyboardOrMouseInputThisPoll = true;
	if (key >= 0 && key < Key_NumElements) {
		if (isDown) {
			m_keyStates[key] |= 1;
		} else {
			m_keyStates[key] &= ~1;
		}
	}
}

void InputState::addMouseWheel(int v) {
	m_hadkeyboardOrMouseInputThisPoll = true;
	m_wheelCount = v;
}

const GamepadState* InputState::getHookedGemepad(const int playerIndex) const {
	int numFoundHooked = 0;
	for (int t = 0; t < SGE_ARRSZ(xinputDevicesState); ++t) {
		if (xinputDevicesState[t].hooked) {
			if (numFoundHooked == playerIndex) {
				return &xinputDevicesState[t];
			}
			numFoundHooked++;
		}
	}

	for (int t = 0; t < SGE_ARRSZ(winapiGamepads); ++t) {
		if (winapiGamepads[t].hooked) {
			if (numFoundHooked == playerIndex) {
				return &winapiGamepads[t];
			}
			numFoundHooked++;
		}
	}

	return nullptr;
}

void GamepadState::Advance(bool sdlStyleDontZeroTheThumbsticks) {
	hadInputThisPoll = false;
	hooked = false;

	for (int t = 0; t < SGE_ARRSZ(btnState); ++t) {
		btnState[t] = (btnState[t] << 1) | (btnState[t] & 1);
	}

	if (sdlStyleDontZeroTheThumbsticks) {
		axisL = vec2f(0.f);
		axisR = vec2f(0.f);
		triggerL = 0.f;
		triggerR = 0.f;
	}
}

bool GamepadState::isBtnDown(Button btn) const {
	if (!hooked)
		return false;
	return (btnState[btn] & 1) != 0;
}

bool GamepadState::isBtnUp(Button btn) const {
	if (!hooked)
		return false;
	return (btnState[btn] & 1) == 0;
}

bool GamepadState::isBtnPressed(Button btn) const {
	if (!hooked)
		return false;
	return (btnState[btn] & 0x3) == 1;
}

bool GamepadState::isBtnReleased(Button btn) const {
	if (!hooked)
		return false;
	return (btnState[btn] & 0x3) == 2;
}

vec2f GamepadState::getInputDir(bool const includeDPad) const {
	vec2f r(0.f);

	if (!hooked)
		return r;

	if (includeDPad) {
		if (isBtnDown(btn_left))
			r.x = -1.f;
		if (isBtnDown(btn_right))
			r.x = 1.f;

		if (isBtnDown(btn_up))
			r.y = 1.f;
		if (isBtnDown(btn_down))
			r.y = -1.f;
	}

	if (r.x == 0.f && r.y == 0.f) {
		r = axisL;
	}

	return r;
}

bool InputState::AnyArrowKeyDown(bool includeWASD) const {
	bool res = IsKeyDown(Key_Left) || IsKeyDown(Key_Right) || IsKeyDown(Key_Up) || IsKeyDown(Key_Down);
	if (includeWASD) {
		res |= IsKeyDown(Key_A) || IsKeyDown(Key_D) || IsKeyDown(Key_W) || IsKeyDown(Key_S);
	}

	return res;
}

vec2f InputState::GetArrowKeysDir(const bool normalize, bool includeWASD, int useGamePadAtIndex) const {
	vec2f result(0.f);

	bool const left = IsKeyDown(Key_Left) || (includeWASD && IsKeyDown(Key_A));
	bool const right = IsKeyDown(Key_Right) || (includeWASD && IsKeyDown(Key_D));
	bool const up = IsKeyDown(Key_Up) || (includeWASD && IsKeyDown(Key_W));
	bool const down = IsKeyDown(Key_Down) || (includeWASD && IsKeyDown(Key_S));

	result.x = (left ? -1.f : 0.f) + (right ? 1.f : 0.f);
	result.y = (down ? -1.f : 0.f) + (up ? 1.f : 0.f);

	if (result == vec2f(0.f) && useGamePadAtIndex >= 0 && useGamePadAtIndex < SGE_ARRSZ(xinputDevicesState)) {
		result = getXInputDevice(useGamePadAtIndex).getInputDir(true);
	}

	return normalize ? normalized0(result) : result;
}

} // namespace sge
