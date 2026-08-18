#pragma once

#include <vector>
#include <memory>
#include <variant>
#include <deque>
#include <string.h>
#include <nfd.h>

#include "ImGuiWindow.h"

struct FileFilter
{
	std::string name;
	std::string extensions;
};

struct EditorConfig
{
	std::vector<FileFilter> openFilters;
	std::vector<FileFilter> saveFilters;
};

struct OpenRequest { std::string path; };
struct SaveAsRequest { std::string path; };
struct SaveRequest { std::string path; };

using Request = std::variant<OpenRequest, SaveAsRequest, SaveRequest>;

class EditorUI
{
private:
	std::vector<std::unique_ptr<ImGuiWindow>> mWindows;

	std::string savePath;

	std::deque<Request> requests;

	void _DrawDockSpace();
	void _DrawMainMenuBar();

	void Open();
	void SaveAs();
	void Save();

	void _PushRequest(const Request& request) { requests.push_back(request); }
public:
	EditorConfig config;

	std::deque<Request> TakeRequests();

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
