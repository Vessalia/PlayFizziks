#pragma once

#include "Fizziks/RigidBody.h"

#include <vector>

class Scene
{
public:
	void AddBody(Fizziks::RigidBody body);
	const std::vector<Fizziks::RigidBody>& GetBodies() const;

private:
	std::vector<Fizziks::RigidBody> bodies;
};
