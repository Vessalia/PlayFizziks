#pragma once

#include <Fizziks/FizzWorld.h>

#include <string>

namespace ngin
{
class SceneSerializer
{
public:
	static bool Save(const Fizziks::FizzWorld* world, const std::string& path);
	static bool Load(Fizziks::FizzWorld* world, const std::string& path);
};
}
