#pragma once

#include <string>
#include <vector>
#include <optional>

class Scene;
class SerializedScene;

class SceneSerializer
{
public:
	static bool Save(const Scene& scene, const std::string& path);
	static std::optional<Scene> Load(const std::string& path);

private:
	static SerializedScene ToSerialized(const Scene& scene);
	static Scene FromSerialized(const SerializedScene& data);
};
