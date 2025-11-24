#pragma once

#include <functional>
#include <unordered_map>
#include <vector>

enum class InputAction {
	ToggleLighting,
	ToggleFlashlight,
	MoveForward,
	MoveBackward,
	MoveLeft,
	MoveRight,
	MoveUp,
	MoveDown,
	MouseLeft,
	MouseRight,
	Shift,
	Escape,
	Exit,
};

using CallbackHandle = uint64_t;

class InputManager {
public:
	using InputCallback = std::function<void(InputAction, bool pressed)>;
	using MouseCallback = std::function<void(double deltaX, double deltaY)>;
	using ScrollCallback = std::function<void(double xOffset, double yOffset)>;

	CallbackHandle registerInputCallback(InputCallback callback);
	void deregisterInputCallback(CallbackHandle handle);

	CallbackHandle registerMouseCallback(MouseCallback callback);
	void deregisterMouseCallback(CallbackHandle handle);

	CallbackHandle registerScrollCallback(ScrollCallback callback);
	void deregisterScrollCallback(CallbackHandle handle);

	void triggerAction(InputAction action, bool pressed);
	void triggerMouse(double deltaX, double deltaY);
	void triggerScroll(double offsetX, double offsetY);

private:
	CallbackHandle nextHandle = 1;

	std::unordered_map<CallbackHandle, InputCallback> inputCallbacks;
	std::unordered_map<CallbackHandle, MouseCallback> mouseCallbacks;
	std::unordered_map<CallbackHandle, ScrollCallback> scrollCallbacks;

	CallbackHandle generateHandle();
};