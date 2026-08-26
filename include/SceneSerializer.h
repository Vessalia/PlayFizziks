#pragma once

#include <Fizziks/FizzWorld.h>

#include <string>

class SceneSerializer
{
public:
	static bool Save(const Fizziks::FizzWorld* world, const std::string& path);
	static bool Load(Fizziks::FizzWorld* world, const std::string& path);
};
