#pragma once

#include "EditorUI.h"
#include "SceneManager.h"
#include "SceneSerializer.h"

struct RequestHandler
{
	SceneManager& sceneManager;

	void operator()(const OpenRequest& r)
	{
		if (auto scene = SceneSerializer::Load(r.path))
		{
			sceneManager.SetScene(*scene);
		}
	}

	void operator()(const SaveRequest& r)
	{
		SceneSerializer::Save(sceneManager.GetScene(), r.path);
	}

	void operator()(const SaveAsRequest& r)
	{
		SceneSerializer::Save(sceneManager.GetScene(), r.path);
	}
};