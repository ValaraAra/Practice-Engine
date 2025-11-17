#pragma once

class Scene {
public:
	virtual ~Scene() {}

	virtual void enter() {}
	virtual void update() {}
	virtual void render() {}
	virtual void gui() {}
	virtual void exit() {}
};