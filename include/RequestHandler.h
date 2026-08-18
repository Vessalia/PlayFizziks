#pragma once

#include "EditorUI.h"
#include "SceneSerializer.h"

#include "Fizziks/FizzWorld.h"

struct RequestHandler
{
	Fizziks::FizzWorld* world;

	bool operator()(const OpenRequest& r)   { return SceneSerializer::Load(world, r.path); }
	bool operator()(const SaveRequest& r)   { return SceneSerializer::Save(world, r.path); }
	bool operator()(const SaveAsRequest& r) { return SceneSerializer::Save(world, r.path); }
};