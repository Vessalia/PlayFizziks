#include "SceneSerializer.h"
#include "SceneManager.h"
#include "SerializedScene.h"
#include "Scene.h"
#include "JSONObject.h"

#include <fstream>

SerializedScene SceneSerializer::ToSerialized(const Scene& scene)
{
	SerializedScene data;

	for (const Body& body : scene.GetBodies())
	{
		SerializedBody sb;
		// RigidBody to serialized body

		std::visit([&sb](const auto& shape)
		{
			// RigidBody shape to serialized body shape
		}, body.GetShape());

		data.bodies.push_back(std::move(sb));
	}

	return data;
}

Scene SceneSerializer::FromSerialized(const SerializedScene& data)
{
	Scene scene;

	for (const SerializedBody& sb : data.bodies)
	{
		scene.AddBody(/*serialized body to RigidBody*/);
	}

	return scene;
}

bool SceneSerializer::Save(const Scene& scene, const std::filesystem::path& path)
{
	JSONObject json = ToSerialized(scene);
	std::ofstream out(path);
	if (!out) return false;
	
	out << json.dump();
	return out.good();
}

std::optional<Scene> SceneSerializer::Load(const std::filesystem::path& path)
{
	std::ifstream in(path);
	if (!in) return std::nullopt;

	JSONObject json;
	try
	{
		in >> json;
	}
	catch (/* catch error here */)
	{
		return std::nullopt;
	}

	if (json.value("version") != SerializedScene::VERSION) return std::nullopt;

	return FromSerialized(json.get<SerializedScene>());
}