#pragma once

#include <vector>
#include <memory>
#include <variant>
#include <deque>
#include <string.h>
#include <nfd.h>

#include "ImGuiWindow.h"

namespace ngin
{
struct DockConfig
{
	float pos[2];
	float dim[2];
};

struct DockGroupSettings
{
	std::vector<std::string> names;
	DockConfig settings;
};

struct FileFilter
{
	std::string name;
	std::string extensions;
};

struct EditorConfig
{
	std::vector<FileFilter> openFilters;
	std::vector<FileFilter> saveFilters;
	std::string defaultPath;
};

struct OpenRequest { std::string path; };
struct SaveAsRequest { std::string path; };
struct SaveRequest { std::string path; };

using Request = std::variant<OpenRequest, SaveAsRequest, SaveRequest>;

class EditorUI
{
private:
	std::vector<std::unique_ptr<ImGuiWindow>> mWindows;
	std::vector<DockGroupSettings> mPendingDockGroups;

	std::string savePath;

	std::deque<Request> requests;

	void _DrawDockSpace();
	void _DrawMainMenuBar();
	void _ApplyPendingDockGroups();

	void Open();
	void SaveAs();
	void Save();

	void _PushRequest(const Request& request) { requests.push_back(request); }
public:
	EditorConfig config;

	std::deque<Request> TakeRequests();
	void DockWindowsTogether(DockGroupSettings settings)
	{
		mPendingDockGroups.push_back(settings);
	}

	template<typename... Windows>
	void AddDockedWindows(DockConfig config, std::unique_ptr<Windows>... windows) {
		static_assert((std::is_base_of_v<ImGuiWindow, Windows> && ...), "All windows must derive from EditorWindow");

		std::vector<std::string> names = { windows->GetName()... };
		(AddEditorWindow(std::move(windows)), ...);

		DockWindowsTogether({ .names = std::move(names), .settings = config });
	}

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
}
