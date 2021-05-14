#pragma once

#include "sge_core/sgecore_api.h"
#include "sge_utils/math/Box.h"
#include "sge_utils/math/vec2.h"
#include "sge_utils/sge_utils.h"
#include "sge_utils/utils/optional.h"
#include <string>
#include <vector>

namespace sge {

struct SGE_CORE_API GamepadState {
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

	void Advance(bool sdlStyleDontZeroTheThumbsticks);
	vec2f getInputDir(bool const includeDPad) const;

  public:
	bool hooked = false;           ///< True if the gamepad is still hooked to the system.
	bool hadInputThisPoll = false; ///< True if the gamepad had any input by the player since last polling.

	unsigned char btnState[btn_numButtons] = {0};

	vec2f axisL = vec2f(0.f);
	vec2f axisR = vec2f(0.f);

	float triggerL = 0.f;
	float triggerR = 0.f;
};

/// @brief An enum specifying keyboard and mouse buttons.
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

struct SGE_CORE_API TouchInput {
	/// @brief touchId specifies the index of the occuring touch.
	/// Every touch event has it's own unique index. Meaning when the user
	/// lift his finger and touches the screen again the touch index of the new event
	/// will be different form the previous one.
	/// The value of -1 specifies an invalid touch.
	int64 touchId = -1;
	vec2f touchPositionPixels = vec2f(0.f);
	vec2f touchMotionPixels = vec2f(0.f);
	float touchPressure = 0.f;

	/// @brief True if the touch has just been made (the finger touched the screen).
	bool isJustPressed = false;

	/// @brief True if the touch has just been released. If this is false it is safe to assume that the touch is held.
	bool isJustReleased = false;
};

/// @brief This structure holds the accumulated input state - mouse, keyboard, game controllers, touch pads and so on.
/// Using this structure we can understand what was the user input for a particualar window or a "sub window".
struct SGE_CORE_API InputState {
	InputState();

	/// When the input state is going to get passed to some sub-system. For example the Gameplay Window in the SGEEditor
	/// we need to be able to simulate mouse clicks, touches and so on to be as if the the game was running alone in a fullscreen
	/// application.
	/// This function sets the region of that domain in client space starting form (0,0) to the desireded size.
	void setDomainSpaceFromSize(const vec2f& size) { m_domain = AABox2f(vec2f(0.f), vec2f(size.x - 1.f, size.y - 1.f)); }

	/// When the input state is going to get passed to some sub-system. For example the Gameplay Window in the SGEEditor
	/// we need to be able to simulate mouse clicks, touches and so on to be as if the the game was running alone in a fullscreen
	/// application.
	/// This function sets the region of that domain in client to the one specified.
	void setDomainFromPosAndSize(const vec2f& pos, const vec2f& size) { m_domain = AABox2f(pos, pos + vec2f(size.x - 1.f, size.y - 1.f)); }

	/// @brief Converts the specified client space point to domain space.
	vec2f clientToDomain(const vec2f& ptClient) const { return ptClient - m_domain.min; }

	/// @brief Converts the specified client space point to domain space and normalizes it so
	/// it is in range [0;0] to [1;1], where:
	/// [0;0] is the top left position of the domain space,
	/// [1;1] is the bottom right position of the domain space.
	vec2f clientToDomainUV(const vec2f& ptClient) const {
		const vec2f ptDomain = ptClient - m_domain.min;
		const vec2f ptDomainUV = ptDomain / m_domain.diagonal();
		return ptDomainUV;
	}

	/// @brief Returns an optional containing the touch that was just pressed, or empty otherwise.
	Optional<vec2f> hasTouchJustPresedUV(const vec2f& minUV, const vec2f& maxUV) const;

	/// @brief Returns an optional containing the touch that was just release, or empty otherwise.
	Optional<vec2f> hasTouchJustReleasedUV(const vec2f& minUV, const vec2f& maxUV) const;

	/// @brief Returns an optional containing the touch that is pressed, or empty otherwise.
	Optional<vec2f> hasTouchPressedUV(const vec2f& minUV, const vec2f& maxUV) const;

	void addTouch(const TouchInput& newTouch);
	TouchInput& findOrCreateTouchById(const int64 touchId);
	const TouchInput* getTouchById(const int64 touchId) const;

	// Moves the current input state into previous. Allowing us to detmine changes to the input, like just pressed/released buttons.
	void Advance();

	void setCursorPos(const vec2f& c);
	void addInputText(const char c);
	void addKeyUpOrDown(Key key, bool isDown);
	void addMouseWheel(int v);
	void addMouseMotion(const vec2f& moution) { m_cursorMotion += moution; }

	bool isCursorRelative() const { return m_isCursorRelative; }
	void setCusorIsRelative(bool isRelative) { m_isCursorRelative = isRelative; }

	void setWasActiveDuringPoll(bool v) { m_wasActiveWhilePolling = v; }
	bool wasActiveWhilePolling() const { return m_wasActiveWhilePolling; }

	const char* GetText() const { return m_inputText.c_str(); }

	const vec2f GetCursorPos() const { return clientToDomain(m_cursorClient); }
	const vec2f getCursorPosUV() const { return clientToDomainUV(m_cursorClient); }
	const vec2f GetCursorMotion() const { return m_cursorMotion; }
	int GetWheelCount() const { return m_wheelCount; }

	bool IsKeyDown(Key key) const { return (bool)(m_keyStates[key] & 1); }
	bool IsKeyUp(Key key) const { return !(bool)(m_keyStates[key] & 1); }
	bool IsKeyPressed(Key key) const { return (m_keyStates[key] & 3) == 1; } // Returns true if the target key was just pressed.
	bool IsKeyReleased(Key key) const { return (m_keyStates[key] & 3) == 2; }
	bool isKeyCombo(Key k0, Key k1) const { return (IsKeyDown(k0) && IsKeyReleased(k1)); }

	bool AnyArrowKeyDown(bool includeWASD) const;

	/// Retrieves the vetor pointed by the arrow keys using +X right, +Y up.
	vec2f GetArrowKeysDir(const bool normalize, bool includeWASD = false, int useGamePadAtIndex = -1) const;

	const GamepadState* getHookedGemepad(const int playerIndex) const;
	const GamepadState& getXInputDevice(int index) const { 
		sgeAssert(index >= 0 && index < SGE_ARRSZ(xinputDevicesState));
		return xinputDevicesState[index]; 
	}

  public:
	/// When the input state is going to get passed to some sub-system. For example the Gameplay Window in the SGEEditor
	/// we need to be able to simulate mouse clicks, touches and so on to be as if the the game was running alone in a fullscreen
	/// application.
	/// @m_domain specifies where the "sub window" is and how big it is, so we can convert coordinates to it.
	AABox2f m_domain;

	/// True if the window was active while polling. We still recieve events even if our window is not the focused one.
	/// If our window is not the focused one, we probably do not want to accept any input.
	bool m_wasActiveWhilePolling = false;
	/// True if there were any inputs made form mouse or keyboard this poll.
	bool m_hadkeyboardOrMouseInputThisPoll = false;
	/// True if the cursor was relative (hidden and not stopping at edges to prosduce motion events). Suitable for FPS games.
	bool m_isCursorRelative = false;
	/// The position of the cursor in client space in pixels.
	vec2f m_cursorClient = vec2f(0.f);
	/// The motion of the cursor. Domain space has the same pixel scale as client space so the values could be used for both.
	vec2f m_cursorMotion = vec2f(0.f);
	/// The amount of mouse wheel ticks this poll.
	int m_wheelCount = 0;
	std::string m_inputText;

	/// A bitmask describing the state of each key.
	/// If a bit is 1 then the button was down, 0 is up.
	/// 0 bit - the current state of the button.
	/// 1 bit - the previous state of the button.
	/// 2-7bit - unused.
	unsigned char m_keyStates[KeyboardAndMouse_NumElements];

	GamepadState xinputDevicesState[4];
	GamepadState winapiGamepads[1]; // TODO: add support for more than one gamepad.

	// Touch inputs.
	std::vector<TouchInput> m_touchInputs;
};

} // namespace sge
