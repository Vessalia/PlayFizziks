#pragma once

#include "ImGuiWindow.h"

class SpawnerWindow : public ImGuiWindow
{
public:
	SpawnerWindow(bool showWindow = true) : ImGuiWindow("Spawn", showWindow) { }

protected:
	virtual void DrawWindow() override
	{
		
	}
}