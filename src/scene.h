#pragma once

#include "renderer.h"

class Scene {
public:
	virtual ~Scene() {}

	virtual void enter() {}
	virtual void update() {}
	virtual void render(Renderer& renderer) {}
	virtual void gui() {}
	virtual void exit() {}
};