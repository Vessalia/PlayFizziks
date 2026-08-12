#pragma once

#include <vector>
#include <memory>

#include "ImGuiWindow.h"

enum class RequestType
{
	OPEN, SAVE
};

struct Request
{
	RequestType type;
};

class EditorUI
{
private:
	std::vector<std::unique_ptr<ImGuiWindow>> mWindows;
	nfdu8char_t* savePath;

	std::deque<Request> requests;

	void _DrawDockSpace();
	void _DrawMainMenuBar();
public:
	std::vector<nfdu8filteritem_t> openFilters;
	std::vector<nfdu8filteritem_t> saveFilters;

	template<typename T>
	T& AddEditorWindow(std::unique_ptr<T> window)
	{
		static_assert(std::is_base_of<ImGuiWindow, T>());

		auto& result = *window;
		mWindows.emplace_back(std::move(window));

		return result;
	}

	void OnImguiRender();
};
