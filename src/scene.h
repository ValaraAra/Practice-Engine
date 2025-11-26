#pragma once

#include "renderer.h"

class Scene {
public:
	virtual ~Scene() {}

	virtual void enter() {}
	virtual void update(float) {}
	virtual void render(Renderer&) {}
	virtual void gui() {}
	virtual void exit() {}

	void setTag(const std::string& tag) { this->tag = tag; }
	std::string getTag() const { return tag; }

protected:
	std::string tag;
};