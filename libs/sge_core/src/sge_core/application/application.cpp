#define NOMINMAX

#include "imgui/imgui.h"
#include "sge_core/ICore.h"
#include "sge_utils/sge_utils.h"
#include "sge_utils/utils/StaticArray.h"

#ifdef SGE_RENDERER_GL
#include "sge_renderer/gl/opengl_include.h"
#include <algorithm>
#endif

#ifdef __EMSCRIPTEN__
#include <SDL2/SDL.h>
#include <emscripten.h>
#include <emscripten/html5_webgl.h>
//#include <GLES3/gl3.h> // WebGL2 + GLES 3 emulation.
#else
#include <SDL.h>
#include <SDL_syswm.h>
#endif

#include "application.h"

namespace sge {

struct WindowImplData {
	SDL_Window* window = nullptr;
	bool isActive = 0;
};

Key SDL_key_to_SGEKey(const int sdlKey) {
	switch (sdlKey) {
		// Letters
		case SDLK_a:
			return Key_A;
		case SDLK_b:
			return Key_B;
		case SDLK_c:
			return Key_C;
		case SDLK_d:
			return Key_D;
		case SDLK_e:
			return Key_E;
		case SDLK_f:
			return Key_F;
		case SDLK_g:
			return Key_G;
		case SDLK_h:
			return Key_H;
		case SDLK_i:
			return Key_I;
		case SDLK_j:
			return Key_J;
		case SDLK_k:
			return Key_K;
		case SDLK_l:
			return Key_L;
		case SDLK_m:
			return Key_M;
		case SDLK_n:
			return Key_N;
		case SDLK_o:
			return Key_O;
		case SDLK_p:
			return Key_P;
		case SDLK_q:
			return Key_Q;
		case SDLK_r:
			return Key_R;
		case SDLK_s:
			return Key_S;
		case SDLK_t:
			return Key_T;
		case SDLK_u:
			return Key_U;
		case SDLK_v:
			return Key_V;
		case SDLK_w:
			return Key_W;
		case SDLK_x:
			return Key_X;
		case SDLK_y:
			return Key_Y;
		case SDLK_z:
			return Key_Z;

		// Numbers
		case SDLK_0:
			return Key_0;
		case SDLK_1:
			return Key_1;
		case SDLK_2:
			return Key_2;
		case SDLK_3:
			return Key_3;
		case SDLK_4:
			return Key_4;
		case SDLK_5:
			return Key_5;
		case SDLK_6:
			return Key_6;
		case SDLK_7:
			return Key_7;
		case SDLK_8:
			return Key_8;
		case SDLK_9:
			return Key_9;

		// F1-12
		case SDLK_F1:
			return Key_F1;
		case SDLK_F2:
			return Key_F2;
		case SDLK_F3:
			return Key_F3;
		case SDLK_F4:
			return Key_F4;
		case SDLK_F5:
			return Key_F5;
		case SDLK_F6:
			return Key_F6;
		case SDLK_F7:
			return Key_F7;
		case SDLK_F8:
			return Key_F8;
		case SDLK_F9:
			return Key_F9;
		case SDLK_F10:
			return Key_F10;
		case SDLK_F11:
			return Key_F11;
		case SDLK_F12:
			return Key_F12;

		// Other:
		case SDLK_ESCAPE:
			return Key_Escape;
		case SDLK_TAB:
			return Key_Tab;
		case SDLK_RETURN:
			return Key_Enter;
		case SDLK_BACKSPACE:
			return Key_Backspace;
		case SDLK_LCTRL:
			return Key_LCtrl;
		case SDLK_RCTRL:
			return Key_RCtrl;
		case SDLK_LALT:
			return Key_LAlt;
		case SDLK_RALT:
			return Key_RAlt;
		case SDLK_LSHIFT:
			return Key_LShift;
		case SDLK_RSHIFT:
			return Key_RShift;
		case SDLK_SPACE:
			return Key_Space;
		case SDLK_PAGEUP:
			return Key_PageUp;
		case SDLK_PAGEDOWN:
			return Key_PageDown;
		case Key_Insert:
			return Key_Insert;
		case SDLK_HOME:
			return Key_Home;
		case SDLK_END:
			return Key_End;
		case SDLK_DELETE:
			return Key_Delete;

		// Arrow keys.
		case SDLK_LEFT:
			return Key_Left;
		case SDLK_UP:
			return Key_Up;
		case SDLK_RIGHT:
			return Key_Right;
		case SDLK_DOWN:
			return Key_Down;
	};

	// Unknown key.
	return Key_NumElements;
}

WindowBase* ApplicationHandler::findWindowBySDLId(const uint32 id) {
	for (WindowBase* wnd : this->m_wnds) {
		if (SDL_GetWindowID(wnd->m_implData->window) == id) {
			return wnd;
		}
	}

	return nullptr;
}

void ApplicationHandler::removeWindow(WindowBase* const wndToRemove) {
	for (int t = 0; t < m_wnds.size(); ++t) {
		if (wndToRemove == m_wnds[t]) {
			delete wndToRemove;
			m_wnds.erase(m_wnds.begin() + t);
			return;
		}
	}
}

void ApplicationHandler::NewWindowInternal(
    WindowBase* window, const char* windowName, int width, int height, bool isMaximized, bool noResize) {
	Uint32 wndFlags = SDL_WINDOW_SHOWN;
	if (noResize == false)
		wndFlags |= SDL_WINDOW_RESIZABLE;

	if (isMaximized)
		wndFlags |= SDL_WINDOW_MAXIMIZED;

#ifdef SGE_RENDERER_GL
	wndFlags |= SDL_WINDOW_OPENGL;
#endif

	window->m_implData->window = SDL_CreateWindow(windowName, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height, wndFlags);

	SDL_EventState(SDL_DROPFILE, SDL_ENABLE);

#if SGE_RENDERER_GL
	[[maybe_unused]] const SDL_GLContext context = SDL_GL_CreateContext(window->m_implData->window);
	sgeAssert(context != nullptr);
	DumpAllGLErrors();

	// emscripten_webgl_get_current_context emscripten_webgl_enable_extension
#if !defined(__EMSCRIPTEN__)
	SDL_GL_SetSwapInterval(1);
#endif
#endif

	// SDL_ShowWindow(window->m_implData->window);


	m_wnds.push_back(window);
}

void ApplicationHandler::DeregisterWindowInternal(WindowBase* wnd) {
	m_wnds.erase(std::find(m_wnds.begin(), m_wnds.end(), wnd));
}

void ApplicationHandler::PollEvents() {
	m_inputState.Advance();

	// Mark the connected gamepads as active, the input state advance makes this false.
	for (auto pair : sdlJoystickInstanceIdIdToIndex) {
		m_inputState.xinputDevicesState[pair.second].hooked = true;
	}

	m_inputState.m_wasActiveWhilePolling = true;

	// int sdlMouseX, sdlMouseY;
	// SDL_GetMouseState(&sdlMouseX, &sdlMouseY);

	// m_inputState.setCursorPos(vec2f(float(sdlMouseX), float(sdlMouseY)));

	// According to the SDL API, touch events aren't generated for any particular window,
	// they are just generated based on the device that captures the touches (remember touches are not generated by only touch screen, they
	// could be touch pads, pen tablets or other).
	// However, if the SDL hint SDL_HINT_TOUCH_MOUSE_EVENTS is set to "1", SDL somehow manages
	// to convert those touch events to mouse clicks at the correct location.
	// The code basically searches for the focused window and uses its size to rescale the
	// touch inputs from normalized space (called UV space in SGEEngine) to screen pixels.
	// The code below does just that.
	vec2f touchEventsToClientSpaceScaling = vec2f(1.f);
	{
		// Mouse focus may have been lost by e.g. the window resizing
		// due to device orientation change while the mouse state is
		// pressed (because its position is now out of the window).
		// SendMouse* will update mouse focus again after that, but
		// if those are never called then SDL might think the
		// 'mouse' has no focus at all.
		SDL_Window* window = SDL_GetMouseFocus();
		if (window == nullptr) {
			window = SDL_GetKeyboardFocus();
		}

		if (window != nullptr) {
			int w, h;
			SDL_GetWindowSize(window, &w, &h);
			touchEventsToClientSpaceScaling.x = float(w);
			touchEventsToClientSpaceScaling.y = float(h);
		}
	}

	SDL_Event event;
	// Bacause SDL keyboard text event doesn't report Enter key presses as text (unlike WINAPI),
	// we need to fake this somehow, this is our way to append enter presses at the end of the text;
	StaticArray<char, 4> additionalTextInput;
	while (SDL_PollEvent(&event)) {
		switch (event.type) {
			case SDL_QUIT: {
				m_isAppQuitRequested = true;
				const std::vector<WindowBase*> availableWindows = m_wnds;
				for (WindowBase* wndToClose : availableWindows) {
					SDL_DestroyWindow(wndToClose->m_implData->window);
					removeWindow(wndToClose);
				}
			} break;
			case SDL_WINDOWEVENT: {
				WindowBase* const wnd = findWindowBySDLId(event.window.windowID);
				if (wnd) {
					if (event.window.event == SDL_WINDOWEVENT_SHOWN) {
						wnd->HandleEvent(WE_Create, nullptr);
					} else if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
						WE_Resize_Data data(event.window.data1, event.window.data2);
						wnd->HandleEvent(WE_Resize, &data);
					} else if (event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
						// From https://wiki.libsdl.org/SDL_WindowEventID
						// window size has changed, either as a result of an API call or through the system or user changing the window
						// size; this event is followed by SDL_WINDOWEVENT_RESIZED if the size was changed by an external event, i.e. the
						// user or the window manager
					} else if (event.window.event == SDL_WINDOWEVENT_CLOSE) {
						wnd->HandleEvent(WE_Destroying, nullptr);
						SDL_DestroyWindow(wnd->m_implData->window);
						removeWindow(wnd); // There might be some more messages here but there is no window destroyed event in SDL.
					}
				}
			} break;
			case SDL_FINGERDOWN: {
				TouchInput& touch = m_inputState.findOrCreateTouchById(event.tfinger.fingerId);

				touch.touchPositionPixels = vec2f(event.tfinger.x, event.tfinger.y) * touchEventsToClientSpaceScaling;
				touch.touchMotionPixels = vec2f(event.tfinger.dx, event.tfinger.dy) * touchEventsToClientSpaceScaling,
				touch.touchPressure =  event.tfinger.pressure;
				touch.isJustPressed = true;
			} break;
			case SDL_FINGERUP: {
				TouchInput& touch = m_inputState.findOrCreateTouchById(event.tfinger.fingerId);

				touch.touchPositionPixels = vec2f(event.tfinger.x, event.tfinger.y) * touchEventsToClientSpaceScaling;
				touch.touchMotionPixels = vec2f(event.tfinger.dx, event.tfinger.dy) * touchEventsToClientSpaceScaling,
				touch.touchPressure =  event.tfinger.pressure;
				touch.isJustReleased = true;

			} break;
			case SDL_FINGERMOTION: {
				TouchInput& touch = m_inputState.findOrCreateTouchById(event.tfinger.fingerId);

				touch.touchPositionPixels = vec2f(event.tfinger.x, event.tfinger.y) * touchEventsToClientSpaceScaling;
				touch.touchMotionPixels = vec2f(event.tfinger.dx, event.tfinger.dy) * touchEventsToClientSpaceScaling,
				touch.touchPressure =  event.tfinger.pressure;
			} break;
			case SDL_TEXTINPUT: {
				for (char ch : event.text.text) {
					if (ch != '\0') {
						m_inputState.addInputText(ch);
					} else {
						break;
					}
				}
			} break;
			case SDL_KEYDOWN: {
				const Key sge_key = SDL_key_to_SGEKey(event.key.keysym.sym);
				m_inputState.addKeyUpOrDown(sge_key, true);

				if (sge_key == Key_Enter) {
					additionalTextInput.push_back('\n');
				} else if (sge_key == Key_Tab) {
					additionalTextInput.push_back('\t');
				}

			} break;
			case SDL_KEYUP: {
				printf("SDL_KEYUP");
				const Key sge_key = SDL_key_to_SGEKey(event.key.keysym.sym);
				m_inputState.addKeyUpOrDown(sge_key, false);
			} break;
			case SDL_MOUSEBUTTONDOWN: {
				Key sgeKey = Key_NumElements;
				if (event.button.button == SDL_BUTTON_LEFT)
					sgeKey = Key_MouseLeft;
				if (event.button.button == SDL_BUTTON_RIGHT)
					sgeKey = Key_MouseRight;
				if (event.button.button == SDL_BUTTON_MIDDLE)
					sgeKey = Key_MouseMiddle;

				m_inputState.addKeyUpOrDown(sgeKey, true);

			} break;
			case SDL_MOUSEBUTTONUP: {
				Key sgeKey = Key_NumElements;
				if (event.button.button == SDL_BUTTON_LEFT)
					sgeKey = Key_MouseLeft;
				if (event.button.button == SDL_BUTTON_RIGHT)
					sgeKey = Key_MouseRight;
				if (event.button.button == SDL_BUTTON_MIDDLE)
					sgeKey = Key_MouseMiddle;

				m_inputState.addKeyUpOrDown(sgeKey, false);
			} break;
			case SDL_MOUSEWHEEL: {
				m_inputState.addMouseWheel(-event.wheel.y);
			} break;
			case SDL_MOUSEMOTION: {
				m_inputState.addMouseMotion(vec2f(float(event.motion.xrel), float(event.motion.yrel)));
				m_inputState.setCursorPos(vec2f(float(event.motion.x), float(event.motion.y)));
			} break;
			case SDL_JOYDEVICEADDED: {
				const int sdlJoystickIndex = event.cdevice.which;
				if (SDL_IsGameController(sdlJoystickIndex)) {
					SDL_GameController* const gameController = SDL_GameControllerOpen(sdlJoystickIndex);
					SDL_Joystick* const gameContollerJoystick = SDL_GameControllerGetJoystick(gameController);
					const int joystickInstanceId = SDL_JoystickInstanceID(gameContollerJoystick);

					if_checked(gameController && sdlJoystickInstanceIdIdToIndex.count(joystickInstanceId) == 0) {
						const int deviceIndex = int(sdlJoystickInstanceIdIdToIndex.size());
						sdlJoystickInstanceIdIdToIndex[joystickInstanceId] = deviceIndex;
					}
				}

			} break;
			case SDL_JOYDEVICEREMOVED: {
				const int sdlGamepadInstanceId = event.cdevice.which;
				if_checked(sdlJoystickInstanceIdIdToIndex.count(sdlGamepadInstanceId) > 0) {
					sdlJoystickInstanceIdIdToIndex.erase(sdlGamepadInstanceId);
				}
			} break;
			case SDL_JOYBUTTONDOWN: {
				const int joystickInstanceId = event.caxis.which;
				if (sdlJoystickInstanceIdIdToIndex.count(joystickInstanceId)) {
					const int gamepadIndex = sdlJoystickInstanceIdIdToIndex[joystickInstanceId];
					m_inputState.xinputDevicesState[gamepadIndex].hadInputThisPoll = true;

					if (event.cbutton.button == 0) {
						m_inputState.xinputDevicesState[gamepadIndex].btnState[GamepadState::btn_a] |= 1;
					} else if (event.cbutton.button == 1) {
						m_inputState.xinputDevicesState[gamepadIndex].btnState[GamepadState::btn_b] |= 1;
					} else if (event.cbutton.button == 2) {
						m_inputState.xinputDevicesState[gamepadIndex].btnState[GamepadState::btn_x] |= 1;
					} else if (event.cbutton.button == 3) {
						m_inputState.xinputDevicesState[gamepadIndex].btnState[GamepadState::btn_y] |= 1;
					} else if (event.cbutton.button == 4) {
						m_inputState.xinputDevicesState[gamepadIndex].btnState[GamepadState::btn_shoulderL] |= 1;
					} else if (event.cbutton.button == 5) {
						m_inputState.xinputDevicesState[gamepadIndex].btnState[GamepadState::btn_shoulderR] |= 1;
					} else if (event.cbutton.button == 6) {
						m_inputState.xinputDevicesState[gamepadIndex].btnState[GamepadState::btn_back] |= 1;
					} else if (event.cbutton.button == 7) {
						m_inputState.xinputDevicesState[gamepadIndex].btnState[GamepadState::btn_start] |= 1;
					} else if (event.cbutton.button == 8) {
						m_inputState.xinputDevicesState[gamepadIndex].btnState[GamepadState::btn_thumbL] |= 1;
					} else if (event.cbutton.button == 9) {
						m_inputState.xinputDevicesState[gamepadIndex].btnState[GamepadState::btn_thumbR] |= 1;
					} else if (event.cbutton.button == 11) { // PS4 hat-up
						m_inputState.xinputDevicesState[gamepadIndex].btnState[GamepadState::btn_up] |= 1;
					} else if (event.cbutton.button == 12) { // PS4 hat-down
						m_inputState.xinputDevicesState[gamepadIndex].btnState[GamepadState::btn_down] |= 1;
					} else if (event.cbutton.button == 13) { // PS4 hat-left
						m_inputState.xinputDevicesState[gamepadIndex].btnState[GamepadState::btn_left] |= 1;
					} else if (event.cbutton.button == 14) { // PS4 hat-right
						m_inputState.xinputDevicesState[gamepadIndex].btnState[GamepadState::btn_right] |= 1;
					}
				}
			} break;
			case SDL_JOYBUTTONUP: {
				const int joystickInstanceId = event.caxis.which;
				if (sdlJoystickInstanceIdIdToIndex.count(joystickInstanceId)) {
					const int gamepadIndex = sdlJoystickInstanceIdIdToIndex[joystickInstanceId];
					m_inputState.xinputDevicesState[gamepadIndex].hadInputThisPoll = true;

					if (event.cbutton.button == 0) {
						m_inputState.xinputDevicesState[gamepadIndex].btnState[GamepadState::btn_a] &= ~1;
					} else if (event.cbutton.button == 1) {
						m_inputState.xinputDevicesState[gamepadIndex].btnState[GamepadState::btn_b] &= ~1;
					} else if (event.cbutton.button == 2) {
						m_inputState.xinputDevicesState[gamepadIndex].btnState[GamepadState::btn_x] &= ~1;
					} else if (event.cbutton.button == 3) {
						m_inputState.xinputDevicesState[gamepadIndex].btnState[GamepadState::btn_y] &= ~1;
					} else if (event.cbutton.button == 4) {
						m_inputState.xinputDevicesState[gamepadIndex].btnState[GamepadState::btn_shoulderL] &= ~1;
					} else if (event.cbutton.button == 5) {
						m_inputState.xinputDevicesState[gamepadIndex].btnState[GamepadState::btn_shoulderR] &= ~1;
					} else if (event.cbutton.button == 6) {
						m_inputState.xinputDevicesState[gamepadIndex].btnState[GamepadState::btn_back] &= ~1;
					} else if (event.cbutton.button == 7) {
						m_inputState.xinputDevicesState[gamepadIndex].btnState[GamepadState::btn_start] &= ~1;
					} else if (event.cbutton.button == 8) {
						m_inputState.xinputDevicesState[gamepadIndex].btnState[GamepadState::btn_thumbL] &= ~1;
					} else if (event.cbutton.button == 9) {
						m_inputState.xinputDevicesState[gamepadIndex].btnState[GamepadState::btn_thumbR] &= ~1;
					} else if (event.cbutton.button == 11) { // PS4 hat-up
						m_inputState.xinputDevicesState[gamepadIndex].btnState[GamepadState::btn_up] &= ~1;
					} else if (event.cbutton.button == 12) { // PS4 hat-down
						m_inputState.xinputDevicesState[gamepadIndex].btnState[GamepadState::btn_down] &= ~1;
					} else if (event.cbutton.button == 13) { // PS4 hat-left
						m_inputState.xinputDevicesState[gamepadIndex].btnState[GamepadState::btn_left] &= ~1;
					} else if (event.cbutton.button == 14) { // PS4 hat-right
						m_inputState.xinputDevicesState[gamepadIndex].btnState[GamepadState::btn_right] &= ~1;
					}
				}
			} break;
			case SDL_JOYAXISMOTION: {
				const int joystickInstanceId = event.caxis.which;
				if (sdlJoystickInstanceIdIdToIndex.count(joystickInstanceId)) {
					auto clampDeadZone = [](float v) -> float {
						if (fabsf(v) < 0.2f) {
							return 0.f;
						}

						return v;
					};

					const int gamepadIndex = sdlJoystickInstanceIdIdToIndex[joystickInstanceId];

					if (gamepadIndex < SGE_ARRSZ(m_inputState.xinputDevicesState)) {
						if (event.caxis.axis == 0) {
							float newValue = clampDeadZone(float(event.caxis.value) / 32768.f);
							m_inputState.xinputDevicesState[gamepadIndex].axisL.x = newValue;
							m_inputState.xinputDevicesState[gamepadIndex].hadInputThisPoll |= newValue > 0.f;
						} else if (event.caxis.axis == 1) {
							float newValue = clampDeadZone(float(-event.caxis.value) / 32768.f);
							m_inputState.xinputDevicesState[gamepadIndex].axisL.y = newValue;
							m_inputState.xinputDevicesState[gamepadIndex].hadInputThisPoll |= newValue > 0.f;
						} else if (event.caxis.axis == 3) {
							float newValue = clampDeadZone(float(event.caxis.value) / 32768.f);
							m_inputState.xinputDevicesState[gamepadIndex].axisR.x = newValue;
							m_inputState.xinputDevicesState[gamepadIndex].hadInputThisPoll |= newValue > 0.f;
						} else if (event.caxis.axis == 4) {
							float newValue = clampDeadZone(float(-event.caxis.value) / 32768.f);
							m_inputState.xinputDevicesState[gamepadIndex].axisR.y = newValue;
							m_inputState.xinputDevicesState[gamepadIndex].hadInputThisPoll |= newValue > 0.f;
						} else if (event.caxis.axis == 2) {
							float newValue = clampDeadZone(float(event.caxis.value) / 32768.f);
							m_inputState.xinputDevicesState[gamepadIndex].triggerL = (newValue + 1.f) * 0.5f;
							m_inputState.xinputDevicesState[gamepadIndex].hadInputThisPoll |= newValue > 0.f;
						} else if (event.caxis.axis == 5) {
							float newValue = clampDeadZone(float(event.caxis.value) / 32768.f);
							m_inputState.xinputDevicesState[gamepadIndex].triggerR = (newValue + 1.f) * 0.5f;
							m_inputState.xinputDevicesState[gamepadIndex].hadInputThisPoll |= newValue > 0.f;
						}
					}
				}
			} break;
			case SDL_JOYBALLMOTION: {
			} break;
			case SDL_JOYHATMOTION: {
				const int joystickInstanceId = event.caxis.which;
				if (sdlJoystickInstanceIdIdToIndex.count(joystickInstanceId)) {
					const int gamepadIndex = sdlJoystickInstanceIdIdToIndex[joystickInstanceId];
					m_inputState.xinputDevicesState[gamepadIndex].hadInputThisPoll = true;

					switch (event.jhat.value) {
						case SDL_HAT_CENTERED: {
							m_inputState.xinputDevicesState[gamepadIndex].btnState[GamepadState::btn_left] &= ~1;
							m_inputState.xinputDevicesState[gamepadIndex].btnState[GamepadState::btn_right] &= ~1;
							m_inputState.xinputDevicesState[gamepadIndex].btnState[GamepadState::btn_up] &= ~1;
							m_inputState.xinputDevicesState[gamepadIndex].btnState[GamepadState::btn_down] &= ~1;
						} break;
						case SDL_HAT_UP: {
							m_inputState.xinputDevicesState[gamepadIndex].btnState[GamepadState::btn_left] &= ~1;
							m_inputState.xinputDevicesState[gamepadIndex].btnState[GamepadState::btn_right] &= ~1;
							m_inputState.xinputDevicesState[gamepadIndex].btnState[GamepadState::btn_up] |= 1;
							m_inputState.xinputDevicesState[gamepadIndex].btnState[GamepadState::btn_down] &= ~1;
						} break;
						case SDL_HAT_RIGHT: {
							m_inputState.xinputDevicesState[gamepadIndex].btnState[GamepadState::btn_left] &= ~1;
							m_inputState.xinputDevicesState[gamepadIndex].btnState[GamepadState::btn_right] |= 1;
							m_inputState.xinputDevicesState[gamepadIndex].btnState[GamepadState::btn_up] &= ~1;
							m_inputState.xinputDevicesState[gamepadIndex].btnState[GamepadState::btn_down] &= ~1;
						} break;
						case SDL_HAT_DOWN: {
							m_inputState.xinputDevicesState[gamepadIndex].btnState[GamepadState::btn_left] &= ~1;
							m_inputState.xinputDevicesState[gamepadIndex].btnState[GamepadState::btn_right] &= ~1;
							m_inputState.xinputDevicesState[gamepadIndex].btnState[GamepadState::btn_up] &= ~1;
							m_inputState.xinputDevicesState[gamepadIndex].btnState[GamepadState::btn_down] |= 1;
						} break;
						case SDL_HAT_LEFT: {
							m_inputState.xinputDevicesState[gamepadIndex].btnState[GamepadState::btn_left] |= 1;
							m_inputState.xinputDevicesState[gamepadIndex].btnState[GamepadState::btn_right] &= ~1;
							m_inputState.xinputDevicesState[gamepadIndex].btnState[GamepadState::btn_up] &= ~1;
							m_inputState.xinputDevicesState[gamepadIndex].btnState[GamepadState::btn_down] &= ~1;
						} break;
						case SDL_HAT_RIGHTUP: {
							m_inputState.xinputDevicesState[gamepadIndex].btnState[GamepadState::btn_left] &= ~1;
							m_inputState.xinputDevicesState[gamepadIndex].btnState[GamepadState::btn_right] |= 1;
							m_inputState.xinputDevicesState[gamepadIndex].btnState[GamepadState::btn_up] |= 1;
							m_inputState.xinputDevicesState[gamepadIndex].btnState[GamepadState::btn_down] &= ~1;
						} break;
						case SDL_HAT_RIGHTDOWN: {
							m_inputState.xinputDevicesState[gamepadIndex].btnState[GamepadState::btn_left] &= ~1;
							m_inputState.xinputDevicesState[gamepadIndex].btnState[GamepadState::btn_right] |= 1;
							m_inputState.xinputDevicesState[gamepadIndex].btnState[GamepadState::btn_up] &= ~1;
							m_inputState.xinputDevicesState[gamepadIndex].btnState[GamepadState::btn_down] |= 1;
						} break;
						case SDL_HAT_LEFTUP: {
							m_inputState.xinputDevicesState[gamepadIndex].btnState[GamepadState::btn_left] |= 1;
							m_inputState.xinputDevicesState[gamepadIndex].btnState[GamepadState::btn_right] &= ~1;
							m_inputState.xinputDevicesState[gamepadIndex].btnState[GamepadState::btn_up] |= 1;
							m_inputState.xinputDevicesState[gamepadIndex].btnState[GamepadState::btn_down] &= ~1;
						} break;
						case SDL_HAT_LEFTDOWN: {
							m_inputState.xinputDevicesState[gamepadIndex].btnState[GamepadState::btn_left] |= 1;
							m_inputState.xinputDevicesState[gamepadIndex].btnState[GamepadState::btn_right] &= ~1;
							m_inputState.xinputDevicesState[gamepadIndex].btnState[GamepadState::btn_up] &= ~1;
							m_inputState.xinputDevicesState[gamepadIndex].btnState[GamepadState::btn_down] |= 1;
						} break;
					}
				}
			} break;
			case SDL_DROPFILE: {
				WindowBase* const wnd = findWindowBySDLId(event.drop.windowID);
				if (wnd) {
					char* const droppedFilenameCStr = event.drop.file;
					const std::string droppedFilename = droppedFilenameCStr;
					SDL_free(droppedFilenameCStr); // SDL SDK says that we need to free this.

					WE_FileDrop_Data data;
					data.filename = droppedFilename;

					wnd->HandleEvent(WE_FileDrop, &data);
				}
			} break;
			default:
				break;
		};
	}

	m_inputState.setCusorIsRelative(SDL_GetRelativeMouseMode() == SDL_TRUE);

	for (char ch : additionalTextInput) {
		m_inputState.addInputText(ch);
	}

	for (WindowBase* const wnd : m_wnds) {
		const bool wasActiveWhilePolling = true; // SDL_GetWindowFlags(wnd->m_implData->window) & SDL_WINDOW_INPUT_GRABBED;
		wnd->m_implData->isActive = wasActiveWhilePolling;
		wnd->m_inputState = m_inputState;

		wnd->m_inputState.setDomainSpaceFromSize(vec2f(float(wnd->GetClientWidth()), float(wnd->GetClientHeight())));
		wnd->m_inputState.setWasActiveDuringPoll(wasActiveWhilePolling);
	}

#ifdef SGE_RENDERER_GL
	for (auto wnd : m_wnds) {
		SDL_GL_SwapWindow(wnd->m_implData->window);
	}
#endif
} // namespace sge

WindowBase::WindowBase() {
	m_implData = new WindowImplData;
}

WindowBase::~WindowBase() {
	delete m_implData;
	m_implData = nullptr;
}

void* WindowBase::GetNativeHandle() const {
#ifdef _WIN32
	SDL_SysWMinfo wmInfo;
	SDL_VERSION(&wmInfo.version);
	if (SDL_GetWindowWMInfo(m_implData->window, &wmInfo) == false) {
		[[maybe_unused]] const char* sdlErrorMsg = SDL_GetError();
		sgeAssert(false);
		return nullptr;
	}
	HWND hwnd = wmInfo.info.win.window;

	return hwnd;
#else
	sgeAssert(false);
	return nullptr;
#endif
}

bool WindowBase::IsActive() const {
	return bool(SDL_WINDOW_INPUT_FOCUS & SDL_GetWindowFlags(m_implData->window));
}


void WindowBase::resizeWindow(int width, int height) {
	SDL_SetWindowSize(m_implData->window, width, height);
}

int WindowBase::GetClientWidth() const {
	int w = 0;
	int h = 0;
	SDL_GetWindowSize(m_implData->window, &w, &h);
	return w;
}

int WindowBase::GetClientHeight() const {
	int w = 0;
	int h = 0;
	SDL_GetWindowSize(m_implData->window, &w, &h);
	return h;
}

bool WindowBase::isMaximized() const {
	bool isMaximizied = SDL_GetWindowFlags(m_implData->window) & SDL_WINDOW_MAXIMIZED;
	return isMaximizied;
}

void WindowBase::setWindowTitle(const char* title) {
	if (title) {
		SDL_SetWindowTitle(m_implData->window, title);
	}
}

void setMouseCursorRelative(bool isRelative) {
	SDL_SetRelativeMouseMode(isRelative ? SDL_TRUE : SDL_FALSE);
}

bool getMouseCursorRelative() {
	return SDL_GetRelativeMouseMode() == SDL_TRUE;
}

} // namespace sge
