#pragma once

#include "Scene.h"

#include <memory>

class SceneManager
{
private:
	Scene scene;
	bool dirty = false;

public:
	void SetScene(Scene newScene) { scene = std::move(newScene); dirty = false; }
	Scene& GetScene() { dirty = true; return scene; }
	const Scene& GetScene() const { return scene; }
	bool IsDirty() const { return dirty; }
};