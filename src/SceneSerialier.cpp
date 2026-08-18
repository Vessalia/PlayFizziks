#include "SceneSerializer.h"
#include "Fizziks/Shape.h"
#include "Fizziks/BodyDefBuilder.h"

#include <nlohmann/json.hpp>
#include <fstream>
#include <variant>

bool SceneSerializer::Save(const Fizziks::FizzWorld* world, const std::string& path)
{
	std::ofstream out(path);
	if (!out) return false;
	
	nlohmann::json json;

	json["version"] = 1;

	json["world"]["gravity"] = { world->Gravity.x, world->Gravity.y };
	json["world"]["timescale"] = world->timescale;
	json["world"]["scale"] = { world->worldScale().x, world->worldScale().y };
	json["world"]["collisionIterations"] = world->collisionIterations();
	json["world"]["timestep"] = world->timestep();
	json["world"]["broadphase"] = world->broadphase();

	auto bodies = world->getActiveBodies();
	for (int i = 0; i < bodies.size(); ++i)
	{
		auto body = bodies[i];
		auto& out = json["bodies"][i];
		out["position"] = { body.position().x, body.position().y };
		out["rotation"] = body.rotation();
		out["velocity"] = { body.velocity().x, body.velocity().y };
		out["angularVelocity"] = body.angularVelocity();
		out["gravityScale"] = body.gravityScale();
		out["bodyType"] = body.bodyType();
		out["layer"] = body.layer();
		out["linearDamping"] = body.linearDamping();
		out["angularDamping"] = body.angularDamping();
		out["restitution"] = body.restitution();

		auto colliders = body.colliders();
		for (int j = 0; j < colliders.size(); ++j)
		{
			auto collider = colliders[j];
			auto& outcol = out["colliders"][j];
			outcol["mass"] = collider.mass;
			outcol["staticFrictionCoeff"] = collider.staticFrictionCoeff;
			outcol["dynamicFrictionCoeff"] = collider.dynamicFrictionCoeff;
			outcol["rotation"] = collider.rotation;
			outcol["position"] = { collider.pos.x, collider.pos.y };
			std::visit([&outcol](const auto& s)
			{
				auto& outshape = outcol["shape"];
				using T = std::decay_t<decltype(s)>;
				if constexpr (std::is_same_v<T, Fizziks::Circle>)
				{
					outshape["type"] = "circle";
					outshape["radius"] = s.radius;
				}
				else if constexpr (std::is_same_v<T, Fizziks::Rect>)
				{
					outshape["type"] = "rect";
					outshape["width"] = s.width;
					outshape["height"] = s.height;
				}
				else if constexpr (std::is_same_v<T, Fizziks::Ellipse>)
				{
					outshape["type"] = "ellipse";
					outshape["rx"] = s.rx;
					outshape["ry"] = s.ry;
				}
				else if constexpr (std::is_same_v<T, Fizziks::Capsule>)
				{
					outshape["type"] = "capsule";
					outshape["capHeight"] = s.capHeight;
					outshape["rect"]["width"] = s.body.width;
					outshape["rect"]["height"] = s.body.height;
				}
				else if constexpr (std::is_same_v<T, Fizziks::Polygon>)
				{
					outshape["type"] = "polygon";
					std::vector<std::array<Fizziks::val_t, 2>> vertices;
					for (const auto& vert : s.vertices)
					{
						vertices.push_back( {vert.x, vert.y} );
					}
					outshape["vertices"] = vertices;
				}
				else constexpr
				{
					static_assert(false, "non-exhaustive visitor!");
				}
			}, collider.shape);
		}
	}

	out << json.dump(4);
	return out.good();
}

bool LoadV1(Fizziks::FizzWorld* world, const nlohmann::json& json)
{
	auto& inworld = json["world"];
	world->Gravity = { inworld["gravity"][0], inworld["gravity"][1] };
	world->timescale = inworld["timescale"];
	world->broadphase(inworld["broadphase"]);
	world->worldScale({ inworld["scale"][0], inworld["scale"][1] });
	world->collisionIterations(inworld["collisionIterations"]);
	world->timestep(inworld["timestep"]);

	if (!json.contains("bodies")) return true;

	for (const auto& inbody : json["bodies"])
	{
		Fizziks::BodyDefBuilder builder = Fizziks::BodyDefBuilder()
			.setLayer(inbody["layer"])
			.setInitPosition({ inbody["position"][0], inbody["position"][1] })
			.setInitVelocity({ inbody["velocity"][0], inbody["velocity"][1] })
			.setInitRotation(inbody["rotation"])
			.setInitAngularVelocity(inbody["angularVelocity"])
			.setGravityScale(inbody["gravityScale"])
			.setRestitution(inbody["restitution"])
			.setLinearDamping(inbody["linearDamping"])
			.setAngularDamping(inbody["angularDamping"])
			.setBodyType(inbody["bodyType"]);

		std::vector<Fizziks::ColliderDef> colliderDefs;
		for (const auto& incol : inbody["colliders"])
		{
			Fizziks::ColliderDef colliderDef;
			colliderDef.mass = incol["mass"];
			colliderDef.staticFrictionCoeff = incol["staticFrictionCoeff"];
			colliderDef.dynamicFrictionCoeff = incol["dynamicFrictionCoeff"];
			colliderDef.rotation = incol["rotation"];
			colliderDef.pos = { incol["position"][0], incol["position"][1] };
		}
		builder.setColliderDefs(colliderDefs);

		world->createBody(builder.build());
	}
}

bool SceneSerializer::Load(Fizziks::FizzWorld* world, const std::string& path)
{
	std::ifstream in(path);
	if (!in) return false;

	try
	{
		nlohmann::json json;
		in >> json;
		if (json["version"] == 1)
		{
			LoadV1(world, json);
		}
		else
		{
			return false;
		}
	}
	catch (const nlohmann::json::exception& e)
	{
		return false;
	}

	return true;
}