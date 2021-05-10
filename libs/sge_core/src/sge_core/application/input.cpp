#include "input.h"

namespace sge {

InputState::InputState() {
	m_cursorClient = vec2f(0.f);
	m_cursorDomain = vec2f(0.f);
	m_cursorMotion = vec2f(0.f);
	m_wheelCount = 0;
	for (auto& v : m_keyStates)
		v = 0;

	for (int t = 0; t < SGE_ARRSZ(winapiGamepads); ++t) {
		// winapiGamepads[t].isUsingStatePolling = false;
	}
}

// Moves the crrent input state into previous

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
