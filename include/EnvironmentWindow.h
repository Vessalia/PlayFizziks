#pragma once

#include "ImGuiWindow.h"

#include <imgui.h>

#include "Fizziks/FizzWorld.h"

using namespace Fizziks;

struct EnvironmentConfig
{
	float timescale = 1;
	float gravity[2] = { 0, -9.81 };
	bool paused = false;
	FizzWorld::AccelStruct accel = FizzWorld::AccelStruct::BVH;
	bool drawDebug = false;
};

class EnvironmentWindow : public ImGuiWindow
{
private:
	Fizziks::FizzWorld* world;
	EnvironmentConfig& config;

public:
	EnvironmentWindow(Fizziks::FizzWorld* world, EnvironmentConfig& config, bool showWindow = true)
	: ImGuiWindow("Environment", showWindow)
	, world(world)
	, config(config) { }

protected:
	virtual void DrawWindow() override
	{
		if (!config.paused)
		{
			if (ImGui::Button("Pause"))
			{
				config.paused = true;
				world->timescale = 0;
			}
		}
		else
		{
			if (ImGui::Button("Resume"))
			{
				config.paused = false;
				world->timescale = config.timescale;
			}
		}

		ImGui::Checkbox("Draw Broadphase Visualization", &config.drawDebug);

		if (ImGui::SliderFloat("Timescale", &config.timescale, 0, 2) && !config.paused)
		{
			world->timescale = config.timescale;
		}

		ImGui::InputFloat2("Gravity", config.gravity);
		if (world->Gravity.x != config.gravity[0] || world->Gravity.y != config.gravity[1])
		{
			world->Gravity = { config.gravity[0], config.gravity[1] };
		}

		const char* accelNames[] = { "Simple", "BVH" };
		int accelIndex = static_cast<int>(config.accel);
		if (ImGui::Combo("Broadphase", &accelIndex, accelNames, IM_ARRAYSIZE(accelNames)))
		{
			config.accel = static_cast<FizzWorld::AccelStruct>(accelIndex);
			world->broadphase(config.accel);
		}
	}
};
