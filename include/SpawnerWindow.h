#pragma once

#include "ImGuiWindow.h"

#include <imgui.h>

#include <vector>

#include "Fizziks/Fizzworld.h"
#include "Fizziks/RigidBody.h"
#include "Fizziks/BodyDef.h"
#include "Fizziks/ColliderDef.h"
#include "Fizziks/Fizziks.h"
#include "Fizziks/Vec.h"
#include "Fizziks/Shape.h"
#include "Fizziks/MathUtils.h"

namespace ngin
{
struct SpawnerConfig
{
	enum class ShapeType { Circle, Ellipse, Rect, Polygon, Capsule };
	enum class PlaceMode { None, PlacePoint, PlacePolygonVertex };

	ShapeType shapeType = ShapeType::Circle;
	Fizziks::BodyType bodyType = Fizziks::BodyType::DYNAMIC;
	float position[2] = { 10.0f, 10.0f };
	float degrees = 0;
	float rotation = 0;

	PlaceMode placeMode = PlaceMode::None;

	float circleRadius = 0.2f;
	float ellipseRx = 0.3f, ellipseRy = 0.2f;
	float rectWidth = 0.4f, rectHeight = 0.4f;
	float capsuleCapHeight = 0.15f;
	float capsuleBodyWidth = 0.3f, capsuleBodyHeight = 0.6f;

	std::vector<Fizziks::Vec2> polyVerts;
	float polyVert[2] = { 0.0f, 0.0f };

	float mass = 1;
	float restitution = 0.3f;

	Fizziks::BodyDef def;

	bool dirty = false;
};

class SpawnerWindow : public ImGuiWindow
{
private:
	SpawnerConfig& config;

public:
	SpawnerWindow(SpawnerConfig& config, bool showWindow = true)
		: ImGuiWindow("Spawn", showWindow)
		, config(config) { }

protected:
	virtual void DrawWindow() override
	{
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

		ImGui::InputFloat("Mass", &config.mass, 0.05f);
		ImGui::InputFloat("Restitution", &config.restitution, 0.05f);
		if (ImGui::InputFloat("Rotation (degrees)", &config.degrees))
		{
			config.rotation = Fizziks::deg2rad(config.degrees);
		}
		ImGui::InputFloat2("Position", config.position);
		ImGui::SameLine();
		if (config.placeMode == SpawnerConfig::PlaceMode::PlacePoint)
		{
			ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "Click in viewport...");
		}
		else if (ImGui::Button("Place in Viewport"))
		{
			config.placeMode = SpawnerConfig::PlaceMode::PlacePoint;
		}

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
			if (config.placeMode == SpawnerConfig::PlaceMode::PlacePolygonVertex)
			{
				ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f),
					"Click to add vertices, right-click to finish");
				if (ImGui::Button("Finish"))
				{
					config.placeMode = SpawnerConfig::PlaceMode::None;

					if (config.polyVerts.size() >= 3)
					{
						Fizziks::Vec2 centroid = Fizziks::Vec2::Zero();
						for (const auto& v : config.polyVerts)
						{
							centroid += v;
						}
						centroid /= (float)config.polyVerts.size();

						for (auto& v : config.polyVerts)
						{
							v -= centroid;
						}

						config.position[0] = centroid.x;
						config.position[1] = centroid.y;
					}
				}
			}
			else if (ImGui::Button("Place Vertices in Viewport"))
			{
				config.polyVerts.clear();
				config.placeMode = SpawnerConfig::PlaceMode::PlacePolygonVertex;
			}

			ImGui::Separator();
			ImGui::Text("Or add manually:");
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

			if (ImGui::Button("Horizontal Flip"))
			{
				Fizziks::Vec2 centroid = Fizziks::getCentroid(config.polyVerts);
				for (int i = 0; i < config.polyVerts.size(); ++i)
				{
					config.polyVerts[i].x = 2 * centroid.x - config.polyVerts[i].x;
				}
			}

			if (ImGui::Button("Vertical Flip"))
			{
				Fizziks::Vec2 centroid = Fizziks::getCentroid(config.polyVerts);
				for (int i = 0; i < config.polyVerts.size(); ++i)
				{
					config.polyVerts[i].y = 2 * centroid.y - config.polyVerts[i].y;
				}
			}

			canSpawn = config.polyVerts.size() >= 3;
			if (!canSpawn)
			{
				ImGui::TextColored(ImVec4(1, 0.4f, 0.4f, 1), "Need at least 3 vertices");
			}
			break;
		}

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

			config.def = Fizziks::BodyDefBuilder()
				.setInitPosition({ config.position[0], config.position[1] })
				.setInitRotation(config.rotation)
				.setBodyType(config.bodyType)
				.setColliderDefs({ Fizziks::ColliderDefBuilder().setShape(shape).setMass(config.mass).build() })
				.setRestitution(config.restitution)
				.build();

			config.dirty = true;
		}
		ImGui::EndDisabled();

		ImGui::EndChild();
	}
};
}