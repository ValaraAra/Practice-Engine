#include "input.h"
#include <stdexcept>
#include <iostream>

// Maybe not the best way to generate handles
CallbackHandle InputManager::generateHandle() {
	return nextHandle++;
}

CallbackHandle InputManager::registerInputCallback(InputCallback callback) {
	CallbackHandle handle = generateHandle();
	inputCallbacks[handle] = callback;

	return handle;
}

void InputManager::deregisterInputCallback(CallbackHandle handle) {
	inputCallbacks.erase(handle);
}

CallbackHandle InputManager::registerMouseCallback(MouseCallback callback) {
	CallbackHandle handle = generateHandle();
	mouseCallbacks[handle] = callback;

	return handle;
}

void InputManager::deregisterMouseCallback(CallbackHandle handle) {
	mouseCallbacks.erase(handle);
}

CallbackHandle InputManager::registerScrollCallback(ScrollCallback callback) {
	CallbackHandle handle = generateHandle();
	scrollCallbacks[handle] = callback;

	return handle;
}

void InputManager::deregisterScrollCallback(CallbackHandle handle) {
	scrollCallbacks.erase(handle);
}

void InputManager::triggerAction(InputAction action, bool pressed) {
	for (auto& [handle, callback] : inputCallbacks) {
		callback(action, pressed);
	}
}

void InputManager::triggerMouse(double deltaX, double deltaY) {
	for (auto& [handle, callback] : mouseCallbacks) {
		callback(deltaX, deltaY);
	}
}

void InputManager::triggerScroll(double offsetX, double offsetY) {
	for (auto& [handle, callback] : scrollCallbacks) {
		callback(offsetX, offsetY);
	}
}
