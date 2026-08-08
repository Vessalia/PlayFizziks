#pragma once

#include <string>
#include "Fizziks/Vec.h"

class ImGuiWindow
{
private:
	bool mShowWindow = false;
	std::string mName;

	Fizziks::Vec2 mSize{};

	int mWindowFlags{};

protected:

	void SetFlags(int flags);

	virtual void StylePush() {};
	virtual void StylePop() {};

	virtual void DrawWindow() = 0;

public:
	ImGuiWindow(const std::string& name, bool showWindow = false);
	virtual ~ImGuiWindow() {}

	void Show(); 
	void Hide(); 

	Fizziks::Vec2 GetSize() const { return mSize; }

	bool IsShowing() const;

	void Draw();

	const std::string& GetName() const;
};
