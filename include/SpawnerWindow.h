#pragma once

#include "ImGuiWindow.h"

#include <imgui.h>

#include <vector>

#include "Fizziks/Fizzworld.h"
#include "Fizziks/RigidBody.h"
#include "Fizziks/BodyDefBuilder.h"

class SpawnerWindow : public ImGuiWindow
{
private:
	Fizziks::FizzWorld* world;
	std::vector<Fizziks::RigidBody> bodies;

public:
	SpawnerWindow(Fizziks::FizzWorld* world, bool showWindow = true) : ImGuiWindow("Spawn", showWindow)
	{
		this->world = world;
	}

	std::vector<Fizziks::RigidBody> GetBodies() const
	{
		return bodies;
	}

protected:
	virtual void DrawWindow() override
	{
		if (ImGui::Button("Clear Scene"))
		{
			for (auto& body : bodies)
			{
				world->destroyBody(body);
			}

			bodies.clear();
		}

		if (ImGui::Button("Spawn Ball"))
		{
			Fizziks::BodyDef def = Fizziks::BodyDefBuilder()
				.setInitPosition({ 10, 10 })
				.setBodyType(Fizziks::BodyType::DYNAMIC)
				.setColliderDefs({ Fizziks::createColliderDef(Fizziks::createCircle(.2f), 0.1f) })
				.setRestitution(0.3f)
				.build();

			bodies.push_back(world->createBody(def));
		}
	}
};
