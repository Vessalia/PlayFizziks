#pragma once

#include "ImGuiWindow.h"

#include <imgui.h>

#include "Fizziks/FizzWorld.h"

using namespace Fizziks;

struct EnvironmentConfig
{
	bool clear = false;
	bool paused = false;
	bool drawDebug = false;

	float gravity[2] = { 0, -9.81 };
	float timescale = 1;
	FizzWorld::AccelStruct accel = FizzWorld::AccelStruct::BVH;

	bool dirty = false;
};

class EnvironmentWindow : public ImGuiWindow
{
private:
	EnvironmentConfig& config;

public:
	EnvironmentWindow(EnvironmentConfig& config, bool showWindow = true)
	: ImGuiWindow("Environment", showWindow)
	, config(config) { }

protected:
	virtual void DrawWindow() override
	{
		if (ImGui::Button("Clear Scene"))
		{
			config.clear = true;
			config.dirty = true;
		}

		if (!config.paused)
		{
			if (ImGui::Button("Pause"))
			{
				config.paused = true;
				config.dirty = true;
			}
		}
		else
		{
			if (ImGui::Button("Resume"))
			{
				config.paused = false;
				config.dirty = true;
			}
		}

		if (ImGui::Checkbox("Draw Broadphase Visualization", &config.drawDebug))
		{
			config.dirty = true;
		}

		if (ImGui::SliderFloat("Timescale", &config.timescale, 0, 2))
		{
			config.dirty = true;
		}
		
		if (ImGui::InputFloat2("Gravity", config.gravity))
		{
			config.dirty = true;
		}

		const char* accelNames[2] = { "Simple", "BVH" };
		int accelIndex = static_cast<int>(config.accel);
		if (ImGui::Combo("Broadphase", &accelIndex, accelNames, IM_ARRAYSIZE(accelNames)))
		{
			config.accel = static_cast<FizzWorld::AccelStruct>(accelIndex);
			config.dirty = true;
		}
	}
};
