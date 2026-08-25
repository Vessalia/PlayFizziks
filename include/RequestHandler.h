#pragma once

#include "EditorUI.h"
#include "SceneSerializer.h"

#include "Fizziks/FizzWorld.h"

enum class RequestResult
{
	LOAD_SUCCESS, LOAD_FAILURE,
	SAVE_SUCCESS, SAVE_FAILURE,
	SAVE_AS_SUCCESS, SAVE_AS_FAILURE
};

struct RequestHandler
{
	Fizziks::FizzWorld** world;

	RequestResult operator()(const OpenRequest& r)
	{
		Fizziks::FizzWorld* loaded = new Fizziks::FizzWorld();

		if (!SceneSerializer::Load(loaded, r.path))
		{
			delete loaded;
			return RequestResult::LOAD_FAILURE;
		}

		std::swap(*world, loaded);
		delete loaded;
		return RequestResult::LOAD_SUCCESS;
	}

	RequestResult operator()(const SaveRequest& r)
	{
		bool result = SceneSerializer::Save(*world, r.path);
		return result ? RequestResult::SAVE_SUCCESS : RequestResult::SAVE_FAILURE;
	}

	RequestResult operator()(const SaveAsRequest& r)
	{
		bool result = SceneSerializer::Save(*world, r.path);
		return result ? RequestResult::SAVE_AS_SUCCESS : RequestResult::SAVE_AS_FAILURE;
	}
};