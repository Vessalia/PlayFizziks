#pragma once

#include "ImGuiWindow.h"

#include <imgui.h>

#include <vector>

#include "Fizziks/Fizzworld.h"
#include "Fizziks/RigidBody.h"
#include "Fizziks/BodyDefBuilder.h"
#include "Fizziks/Fizziks.h"
#include "Fizziks/Vec.h"

struct SpawnerConfig
{
	enum class ShapeType { Circle, Ellipse, Rect, Polygon, Capsule };

	ShapeType shapeType = ShapeType::Circle;
	Fizziks::BodyType bodyType = Fizziks::BodyType::DYNAMIC;
	float pos[2] = { 10.0f, 10.0f };

	float circleRadius = 0.2f;
	float ellipseRx = 0.3f, ellipseRy = 0.2f;
	float rectWidth = 0.4f, rectHeight = 0.4f;
	float capsuleCapHeight = 0.15f;
	float capsuleBodyWidth = 0.3f, capsuleBodyHeight = 0.6f;

	std::vector<Fizziks::Vec2> polyVerts;
	float polyVert[2] = { 0.0f, 0.0f };

	float mass = 1;
	float restitution = 0.3f;
};

class SpawnerWindow : public ImGuiWindow
{
private:

	Fizziks::FizzWorld* world;
	std::vector<Fizziks::RigidBody> bodies;

	SpawnerConfig& config;

public:
	SpawnerWindow(Fizziks::FizzWorld* world, SpawnerConfig& config, bool showWindow = true)
	: ImGuiWindow("Spawn", showWindow)
	, world(world)
	, config(config) { }

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

		ImGui::BeginChild("Create a RigidBody");

		const char* shapeNames[] = { "Circle", "Ellipse", "Rect", "Polygon", "Capsule" };
		int shapeIndex = static_cast<int>(config.shapeType);
		if (ImGui::Combo("Shape", &shapeIndex, shapeNames, IM_ARRAYSIZE(shapeNames)))
		{
			config.shapeType = static_cast<SpawnerConfig::ShapeType>(shapeIndex);
		}

		const char* bodyTypeNames[] = { "Static", "Dynamic", "Kinematic" };
		int bodyTypeIndex = static_cast<int>(config.bodyType);
		if (ImGui::Combo("Body Type", &bodyTypeIndex, bodyTypeNames, IM_ARRAYSIZE(bodyTypeNames)))
		{
			config.bodyType = static_cast<Fizziks::BodyType>(bodyTypeIndex);
		}

		ImGui::InputFloat2("Position", config.pos);

		bool canSpawn = true;

		switch (config.shapeType)
		{
		case SpawnerConfig::ShapeType::Circle:
			ImGui::InputFloat("Radius", &config.circleRadius, 0.05f);
			break;

		case SpawnerConfig::ShapeType::Ellipse:
			ImGui::InputFloat("Rx", &config.ellipseRx, 0.05f);
			ImGui::InputFloat("Ry", &config.ellipseRy, 0.05f);
			break;

		case SpawnerConfig::ShapeType::Rect:
			ImGui::InputFloat("Width", &config.rectWidth, 0.05f);
			ImGui::InputFloat("Height", &config.rectHeight, 0.05f);
			break;

		case SpawnerConfig::ShapeType::Capsule:
			ImGui::InputFloat("Cap Height", &config.capsuleCapHeight, 0.05f);
			ImGui::InputFloat("Body Width", &config.capsuleBodyWidth, 0.05f);
			ImGui::InputFloat("Body Height", &config.capsuleBodyHeight, 0.05f);
			break;

		case SpawnerConfig::ShapeType::Polygon:
			ImGui::InputFloat2("Vertex Pos", config.polyVert);

			if (ImGui::Button("Add Vertex"))
			{
				config.polyVerts.push_back({ config.polyVert[0], config.polyVert[1] });
			}

			ImGui::SameLine();

			if (ImGui::Button("Remove Last") && !config.polyVerts.empty())
			{
				config.polyVerts.pop_back();
			}

			ImGui::SameLine();

			if (ImGui::Button("Clear Vertices"))
			{
				config.polyVerts.clear();
			}

			ImGui::Text("Vertices (%zu):", config.polyVerts.size());
			for (size_t i = 0; i < config.polyVerts.size(); ++i)
			{
				ImGui::Text("  [%zu] (%.2f, %.2f)", i, config.polyVerts[i].x, config.polyVerts[i].y);
			}

			canSpawn = config.polyVerts.size() >= 3;
			if (!canSpawn)
			{
				ImGui::TextColored(ImVec4(1, 0.4f, 0.4f, 1), "Need at least 3 vertices");
			}
			break;
		}

		ImGui::InputFloat("Mass", &config.mass, 0.05f);
		ImGui::InputFloat("Restitution", &config.restitution, 0.05f);

		ImGui::BeginDisabled(!canSpawn);
		if (ImGui::Button("Spawn"))
		{
			Fizziks::Shape shape;

			switch (config.shapeType)
			{
			case SpawnerConfig::ShapeType::Circle:
				shape = Fizziks::createCircle(config.circleRadius);
				break;
			case SpawnerConfig::ShapeType::Ellipse:
				shape = Fizziks::createEllipse(config.ellipseRx, config.ellipseRy);
				break;
			case SpawnerConfig::ShapeType::Rect:
				shape = Fizziks::createRect(config.rectWidth, config.rectHeight);
				break;
			case SpawnerConfig::ShapeType::Polygon:
				shape = Fizziks::createPolygon(config.polyVerts);
				break;
			case SpawnerConfig::ShapeType::Capsule:
				shape = Fizziks::createCapsule(config.capsuleCapHeight, Fizziks::createRect(config.capsuleBodyWidth, config.capsuleBodyHeight));
				break;
			}

			Fizziks::BodyDef def = Fizziks::BodyDefBuilder()
				.setInitPosition({ config.pos[0], config.pos[1] })
				.setBodyType(config.bodyType)
				.setColliderDefs({ Fizziks::createColliderDef(shape, config.mass) })
				.setRestitution(config.restitution)
				.build();

			bodies.push_back(world->createBody(def));
		}
		ImGui::EndDisabled();

		ImGui::EndChild();
	}
};