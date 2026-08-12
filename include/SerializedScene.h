#pragma once

#include <vector>

class JSONObject;

struct SerializedBody
{
	int t;
};

class SerializedScene
{
private:
	std::vector<SerializedBody> bodies;

public:
	inline static int VERSION = 1; // get the actual version here somehow. Probably want to let version ranges access similar files
	std::vector<SerializedBody> bodies;

	JSONObject ToJson() const;
	void FromJson(JSONObject json);
};
