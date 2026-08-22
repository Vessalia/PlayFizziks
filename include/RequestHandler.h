#pragma once

#include "EditorUI.h"
#include "SceneSerializer.h"

#include "Fizziks/FizzWorld.h"

struct RequestHandler
{
	Fizziks::FizzWorld** world;

	bool operator()(const OpenRequest& r)
	{
		Fizziks::FizzWorld* loaded = new Fizziks::FizzWorld();

		if (!SceneSerializer::Load(loaded, r.path))
		{
			delete loaded;
			return false;
		}

		std::swap(*world, loaded);
		delete loaded;
		return true;
	}

	bool operator()(const SaveRequest& r)
	{
		return SceneSerializer::Save(*world, r.path);
	}

	bool operator()(const SaveAsRequest& r)
	{
		return SceneSerializer::Save(*world, r.path);
	}
};