#include <iostream>
#include <chrono>
#include <vector>
#include <numeric>

#include "nfd.h"

#include "EditorUI.h"
#include "SpawnerWindow.h"
#include "EnvironmentWindow.h"

#include "RequestHandler.h"

#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_sdl3.h"
#include "imgui/backends/imgui_impl_sdlrenderer3.h"

#include "SDL3/SDL.h"

#include "Fizziks/Fizziks.h"
#include "Fizziks/FizzWorld.h"
#include "Fizziks/RigidBody.h"
#include "Fizziks/Shape.h"
#include "Fizziks/Vec.h"
#include "Fizziks/MathUtils.h"
#include "Fizziks/Log.h"
#include "Fizziks/BodyDef.h"
#include "Fizziks/ColliderDef.h"

using namespace Fizziks;

const unsigned int SCREEN_WIDTH  = 720;
const unsigned int SCREEN_HEIGHT = 720;
float mainScale = 1;

#define WIDTH (SCREEN_WIDTH * mainScale)
#define HEIGHT (SCREEN_HEIGHT * mainScale)

static SDL_Window* gWindow = nullptr;
static SDL_Renderer* gRenderer = nullptr;

static std::unique_ptr<EditorUI> editor;
static EnvironmentConfig enviroConfig;
static SpawnerConfig spawnerConfig;

static FizzWorld* world = new FizzWorld(20, 20, 5, 1 / 30.f, Fizziks::FizzWorld::AccelStruct::BVH);

Uint32 lt = SDL_GetTicks();

void draw();
void close();

Vec2 worldToScreen(Vec2 worldPos);
Vec2 screenToWorld(Vec2 screenPos);

std::vector<Vec2> circleGenerator(Vec2 center, float radius, int segments = 48);
std::vector<Vec2> ellipseGenerator(Vec2 center, float rx, float ry, float angle, int segments = 48);
std::vector<Vec2> rectGenerator(Vec2 center, float width, float height, float angle);
std::vector<Vec2> capsuleGenerator(Vec2 center, float bodyWidth, float bodyHeight, float capHeight, float angle, int capSegments = 24);
std::vector<Vec2> polygonGenerator(Vec2 center, const std::vector<Vec2>& localVerts, float angle);

std::vector<SDL_Vertex> buildScreenVerts(const std::vector<Vec2>& worldPts, const SDL_FColor& color);

void drawBody(const RigidBody& rb);
void drawShape(Vec2 worldCenter, const std::vector<Vec2>& worldPts, bool filled, const SDL_FColor& color);
void drawCircle(Vec2 worldCenter, float radius, float angle, bool filled, const SDL_FColor& color);
void drawEllipse(Vec2 worldCenter, float rx, float ry, float angle, bool filled, const SDL_FColor& color);
void drawRect(Vec2 worldCenter, float width, float height, float angle, bool filled, const SDL_FColor& color);
void drawCapsule(Vec2 worldCenter, float bodyWidth, float bodyHeight, float capHeight, float angle, bool filled, const SDL_FColor& color);
void drawPolygon(Vec2 worldCenter, const std::vector<Vec2>& localVerts, float angle, bool filled, const SDL_FColor& color);

void drawGhost();
void drawPolygonInProgress(Vec2 mouseWorld);

void createStage()
{
	BodyDef left = BodyDefBuilder()
		.setInitPosition({ 0, 0 })
		.setBodyType(BodyType::STATIC)
		.setColliderDefs({ ColliderDefBuilder().setShape(createRect(1, 20)).build() })
		.setRestitution(0.0f)
		.build();

	BodyDef right = BodyDefBuilder()
		.setInitPosition({ 20, 0 })
		.setBodyType(BodyType::STATIC)
		.setColliderDefs({ ColliderDefBuilder().setShape(createRect(1, 20)).build() })
		.setRestitution(0.0f)
		.build();

	BodyDef bottom = BodyDefBuilder()
		.setInitPosition({ 10, 0.5 })
		.setBodyType(BodyType::STATIC)
		.setColliderDefs({ ColliderDefBuilder().setShape(createRect(20, 1)).build() })
		.setRestitution(0.1f)
		.build();

	world->createBody(left);
	world->createBody(right);
	world->createBody(bottom);
}

void createBalls()
{
	Vec2 xRange = { 0, 10 };
	Vec2 yRange = { 0, 10 };

	for (int x = xRange.x; x < xRange.y; ++x)
	{
		for (int y = yRange.x; y < yRange.y; ++y)
		{
			float normalizedX = (float)(x - xRange.x) / (xRange.y - xRange.x);
			float normalizedY = (float)(y - yRange.x) / (yRange.y - yRange.x);

			float posX = 7.5 + normalizedX * 5;
			float posY = 7.5 + normalizedY * 5;

			auto ballBuilder = BodyDefBuilder()
				.setInitPosition({ posX, posY })
				.setBodyType(BodyType::DYNAMIC)
				.setRestitution(0.3f)
				.setColliderDefs({ ColliderDefBuilder().setShape(createCircle(0.2f)).setMass(0.1f).build() });

			world->createBody(ballBuilder.build());
		}
	}
}

void createScene()
{
	createStage();
	createBalls();
}

bool initSDL()
{
	if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD))
	{
		SDL_Log("Error: SDL_Init(): %s\n", SDL_GetError());
		return false;
	}

	mainScale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());
	SDL_WindowFlags window_flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN | SDL_WINDOW_HIGH_PIXEL_DENSITY;
	gWindow = SDL_CreateWindow("PlayFizziks", (int)(WIDTH), (int)(HEIGHT), window_flags);
	if (gWindow == nullptr)
	{
		SDL_Log("Error: SDL_CreateWindow(): %s\n", SDL_GetError());
		return false;
	}

	SDL_SetHint(SDL_HINT_RENDER_LINE_METHOD, "2");
	gRenderer = SDL_CreateRenderer(gWindow, NULL);
	if (gRenderer == nullptr)
	{
		SDL_Log("Error: SDL_CreateRenderer(): %s\n", SDL_GetError());
		return false;
	}

	SDL_SetWindowPosition(gWindow, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
	SDL_ShowWindow(gWindow);

	return true;
}

bool initImGui()
{
	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // Enable Docking

	ImGui::StyleColorsDark();

	// Setup scaling
	ImGuiStyle& style = ImGui::GetStyle();
	style.ScaleAllSizes(mainScale);
	style.FontScaleDpi = mainScale;

	// Setup Platform/Renderer backends
	ImGui_ImplSDL3_InitForSDLRenderer(gWindow, gRenderer);
	ImGui_ImplSDLRenderer3_Init(gRenderer);

	if (NFD_Init() != NFD_OKAY)
	{
		printf("Error: NFD_Init(): %s\n", NFD_GetError());
		return false;
	}

	editor = std::make_unique<EditorUI>();
	editor->config.openFilters = { FileFilter{"Load a Scene", "fizz"} };
	editor->config.saveFilters = { FileFilter{"Save a Scene", "fizz"} };

	std::unique_ptr<SpawnerWindow> spawner = std::make_unique<SpawnerWindow>(spawnerConfig);
	editor->AddEditorWindow(std::move(spawner));

	std::unique_ptr<EnvironmentWindow> enviro = std::make_unique<EnvironmentWindow>(enviroConfig);
	editor->AddEditorWindow(std::move(enviro));

	return true;
}

// this whole pattern feels like it should be virtualized
void handleSpawnerConfig()
{
	world->createBody(spawnerConfig.def);
}

void handleEnvironmentConfig()
{
	world->timescale = enviroConfig.timescale;
	if (enviroConfig.paused)
	{
		world->timescale = 0;
	}
	world->broadphase(enviroConfig.accel);
	world->Gravity = { enviroConfig.gravity[0], enviroConfig.gravity[1] };
	if (enviroConfig.clear)
	{
		world->destroyAllBodies();
		enviroConfig.clear = false;
	}
}

void handleConfigs()
{
	if (spawnerConfig.dirty)
	{
		handleSpawnerConfig();
		spawnerConfig.dirty = false;
	}

	if (enviroConfig.dirty)
	{
		handleEnvironmentConfig();
		enviroConfig.dirty = false;
	}
}

int main(int argc, char** argv)
{
	bool sdl_error = !initSDL();
	bool imgui_error = !initImGui();

	if (sdl_error || imgui_error)
	{
		printf("Program failure, exiting...\n");
		return 1;
	}

	Fizziks::SinkOptions options;
	options.threadSafe = true;
	addLogSink([](Fizziks::LogLevel level, std::string_view msg, std::string_view file, int line)
		{
			std::cout << "level = " << toString(level) << ": msg = " << msg << ": file = " << file << ": line = " << line << std::endl;
		}, options
	);

	createScene();

	float skipTime = 0;
	bool quit = false;
	while (!quit)
	{
		SDL_Event e;
		while (SDL_PollEvent(&e))
		{
			ImGui_ImplSDL3_ProcessEvent(&e);
			if (e.type == SDL_EVENT_QUIT)
			{
				quit = true;
			}
			else if (e.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && e.window.windowID == SDL_GetWindowID(gWindow))
			{
				quit = true;
			}
			else if (e.type == SDL_EVENT_KEY_DOWN)
			{
				switch (e.key.key)
				{
				case SDLK_ESCAPE:
					quit = true;
					break;
				}
			}
			else if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN && !ImGui::GetIO().WantCaptureMouse)
			{
				if (e.button.button == SDL_BUTTON_LEFT)
				{
					Vec2 worldPos = screenToWorld({ e.button.x, e.button.y });

					if (spawnerConfig.placeMode == SpawnerConfig::PlaceMode::PlacePoint)
					{
						spawnerConfig.position[0] = worldPos.x;
						spawnerConfig.position[1] = worldPos.y;
						spawnerConfig.placeMode = SpawnerConfig::PlaceMode::None;
					}
					else if (spawnerConfig.placeMode == SpawnerConfig::PlaceMode::PlacePolygonVertex)
					{
						spawnerConfig.polyVerts.push_back(worldPos);
					}
				}
				else if (e.button.button == SDL_BUTTON_RIGHT)
				{
					if (spawnerConfig.placeMode == SpawnerConfig::PlaceMode::PlacePolygonVertex)
					{
						Vec2 centroid = Fizziks::getCentroid(spawnerConfig.polyVerts); // place the poly where the user was building it
						spawnerConfig.position[0] = centroid.x;
						spawnerConfig.position[1] = centroid.y;
						spawnerConfig.placeMode = SpawnerConfig::PlaceMode::None;
					}
				}
			}
		}

		float time = SDL_GetTicks() - skipTime;
		float dt = (time - lt) / 1000.f;
		lt = time;

		world->tick(dt);
		draw();

		// need to handle current dt compensation death spiral from file dialog
		skipTime = SDL_GetTicks();
		for (auto& req : editor->TakeRequests())
		{
			RequestResult result = std::visit(RequestHandler{ &world }, req);
			if (result == RequestResult::LOAD_FAILURE || result == RequestResult::SAVE_FAILURE || result == RequestResult::SAVE_AS_FAILURE)
			{
				printf("The requested operation failed\n");
			}
			else if (result == RequestResult::LOAD_SUCCESS)
			{
				enviroConfig.paused = true;
				enviroConfig.dirty = true;
			}
		}
		skipTime = SDL_GetTicks() - skipTime; // this doesn't seem to work...

		handleConfigs();
	}

	close();
}

void draw()
{
	ImGui_ImplSDLRenderer3_NewFrame();
	ImGui_ImplSDL3_NewFrame();
	ImGui::NewFrame();

	editor->OnImguiRender();

	ImGui::Render();

	SDL_SetRenderDrawColor(gRenderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
	SDL_RenderClear(gRenderer);

	if (enviroConfig.drawDebug)
	{
		auto info = world->getBroadphaseDebugInfo();
		SDL_SetRenderDrawColor(gRenderer, 255, 0, 255, 255);
		for (const auto& aabb : info)
		{
			SDL_FRect rect;
			Vec2 topLeft = worldToScreen(aabb.min);
			Vec2 bottomRight = worldToScreen(aabb.max);

			rect.x = topLeft.x;
			rect.y = bottomRight.y;
			rect.w = bottomRight.x - topLeft.x;
			rect.h = topLeft.y - bottomRight.y;
			SDL_RenderRect(gRenderer, &rect);
		}
	}

	for (const auto& body : world->getActiveBodies())
	{
		drawBody(body);
	}

	drawGhost();

	ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), gRenderer);
	SDL_RenderPresent(gRenderer);
}

Vec2 worldToScreen(Vec2 worldPos)
{
	Vec2 scale = { (float)world->worldScale().x, (float)world->worldScale().y };
	return { worldPos.x * WIDTH / scale.x, HEIGHT * (1 - worldPos.y / scale.y) };
}
 
Vec2 screenToWorld(Vec2 screenPos)
{
	Vec2 scale = { (float)world->worldScale().x, (float)world->worldScale().y };
	return { screenPos.x * scale.x / WIDTH, scale.y * (1 - screenPos.y / HEIGHT) };
}

void drawBody(const RigidBody& body)
{
	SDL_FColor color = SDL_FColor{ 1, 1, 1, SDL_ALPHA_OPAQUE };
	if (body.bodyType() == BodyType::STATIC)
	{
		color = { 160 / 255.f, 150 / 255.f, 30 / 255.f, SDL_ALPHA_OPAQUE };
	}
	else if (body.bodyType() == BodyType::DYNAMIC)
	{
		color = { 100 / 255.f, 60 / 255.f, 150 / 255.f, SDL_ALPHA_OPAQUE };
	}
	else
	{
		color = { 60 / 255.f, 150 / 255.f, 100 / 255.f, SDL_ALPHA_OPAQUE };
	}

	const auto colliders = body.colliders();
	Vec2 bodyPos = body.centroidPosition();
	float rot = body.rotation();
	for (const auto& collider : colliders)
	{
		float angle = rot + collider.rotation;
		Vec2 worldCenter = bodyPos + collider.position;
		std::visit([&worldCenter, &angle, &color](const auto& s)
		{
			using T = std::decay_t<decltype(s)>;
			if constexpr (std::is_same_v<T, Fizziks::Circle>)
			{
				drawCircle(worldCenter, s.radius, angle, true, color);
			}
			else if constexpr (std::is_same_v<T, Fizziks::Ellipse>)
			{
				drawEllipse(worldCenter, s.rx, s.ry, angle, true, color);
			}
			else if constexpr (std::is_same_v<T, Fizziks::Rect>)
			{
				drawRect(worldCenter, s.width, s.height, angle, true, color);
			}
			else if constexpr (std::is_same_v<T, Fizziks::Polygon>)
			{
				drawPolygon(worldCenter, s.vertices, angle, true, color);
			}
			else if constexpr (std::is_same_v<T, Fizziks::Capsule>)
			{
				drawCapsule(worldCenter, s.body.width, s.body.height, s.capHeight, angle, true, color);
			}
			else
			{
				static_assert(false, "non-exhaustive visitor!");
			}
		}, collider.shape);
	}
}

std::vector<Vec2> circleGenerator(Vec2 center, float radius, int segments)
{
	std::vector<Vec2> pts;
	pts.reserve(segments);
	for (int i = 0; i < segments; ++i)
	{
		float t = Fizziks::TWO_PI * i / segments;
		pts.push_back(center + Vec2{ radius * std::cos(t), radius * std::sin(t) });
	}

	return pts;
}

std::vector<Vec2> ellipseGenerator(Vec2 center, float rx, float ry, float angle, int segments)
{
	std::vector<Vec2> pts;
	pts.reserve(segments);
	for (int i = 0; i < segments; ++i)
	{
		float t = Fizziks::TWO_PI * i / segments;
		Vec2 local = Vec2{ rx * std::cos(t), ry * std::sin(t) }.rotated(angle);
		pts.push_back(center + local);
	}

	return pts;
}

std::vector<Vec2> rectGenerator(Vec2 center, float width, float height, float angle)
{
	float hw = width * 0.5f;
	float hh = height * 0.5f;
	Vec2 corners[4] = { { -hw, -hh }, { hw, -hh }, { hw, hh }, { -hw, hh } };

	std::vector<Vec2> pts;
	pts.reserve(4);
	for (auto& c : corners)
	{
		pts.push_back(center + c.rotated(angle));
	}

	return pts;
}

std::vector<Vec2> capsuleGenerator(Vec2 center, float bodyWidth, float bodyHeight, float capHeight, float angle, int capSegments)
{
	const float rx = bodyWidth * 0.5f;
	const float ry = capHeight;
	const float capOffset = bodyHeight * 0.5f;

	std::vector<Vec2> pts;
	pts.reserve(capSegments * 2);

	// capsule's rect body is implicitly between the caps

	for (int i = 0; i < capSegments; ++i)
	{
		float t = PI * i / (capSegments - 1);
		Vec2 local = Vec2{ rx * std::cos(t), capOffset + ry * std::sin(t) }.rotated(angle);
		pts.push_back(center + local);
	}


	for (int i = 0; i < capSegments; ++i)
	{
		float t = PI * i / (capSegments - 1);
		Vec2 local = Vec2{ -rx * std::cos(t), -capOffset - ry * std::sin(t) }.rotated(angle);
		pts.push_back(center + local);
	}

	return pts;
}

std::vector<Vec2> polygonGenerator(Vec2 center, const std::vector<Vec2>& localVerts, float angle)
{
	Vec2 centroid = Fizziks::getCentroid(localVerts);

	std::vector<Vec2> pts;
	pts.reserve(localVerts.size());
	for (const auto& v : localVerts)
	{
		pts.push_back(center + (v - centroid).rotated(angle));
	}

	return pts;
}

// avoid rebuilding these buffers every frame
static std::vector<SDL_Vertex> sdlVerts;
static std::vector<int> indices;
 
std::vector<SDL_Vertex> buildScreenVerts(const std::vector<Vec2>& worldPts, const SDL_FColor& color)
{
	std::vector<SDL_Vertex> verts;
	verts.reserve(worldPts.size());
	for (const auto& p : worldPts)
	{
		Vec2 s = worldToScreen(p);
		SDL_Vertex v;
		v.position = SDL_FPoint{ s.x, s.y };
		v.color = color;
		v.tex_coord = SDL_FPoint{ 0, 0 };
		verts.push_back(v);
	}

	return verts;
}

void drawShape(Vec2 worldCenter, const std::vector<Vec2>& worldPts, bool filled, const SDL_FColor& color)
{
	if (filled)
	{
		if (worldPts.size() < 3) return;
 
		Vec2 centerScreen = worldToScreen(worldCenter);
		SDL_Vertex centerVert;
		centerVert.position = SDL_FPoint{ centerScreen.x, centerScreen.y };
		centerVert.color = color;
		centerVert.tex_coord = SDL_FPoint{ 0, 0 };
 
		sdlVerts.clear();
		sdlVerts.reserve(worldPts.size() + 1);
		sdlVerts.push_back(centerVert);
		auto rim = buildScreenVerts(worldPts, color);
		sdlVerts.insert(sdlVerts.end(), rim.begin(), rim.end());
 
		size_t n = worldPts.size();
		indices.clear();
		indices.reserve(n * 3);
		for (size_t i = 1; i <= n; ++i)
		{
			indices.push_back(0);
			indices.push_back((int)i);
			indices.push_back((int)(i % n + 1));
		}
 
		SDL_RenderGeometry(gRenderer, nullptr,
			sdlVerts.data(), static_cast<int>(sdlVerts.size()),
			indices.data(), static_cast<int>(indices.size()));
	}
	else
	{
		if (worldPts.size() < 2) return;
 
		SDL_SetRenderDrawColor(gRenderer, (Uint8)color.r, (Uint8)color.g, (Uint8)color.b, (Uint8)color.a);
		
		size_t n = worldPts.size();
		for (size_t i = 0; i < n; ++i)
		{
			Vec2 a = worldToScreen(worldPts[i]);
			Vec2 b = worldToScreen(worldPts[(i + 1) % n]);
			SDL_RenderLine(gRenderer, (int)a.x, (int)a.y, (int)b.x, (int)b.y);
		}
	}
}

void drawCircle(Vec2 worldCenter, float radius, float angle, bool filled, const SDL_FColor& color)
{
	drawShape(worldCenter, circleGenerator(worldCenter, radius), filled, color);
 
	if (filled)
	{
		// small marker line showing the body's rotation
		Vec2 dir = Vec2{ radius, 0 }.rotated(angle);
		Vec2 s0 = worldToScreen(worldCenter - dir);
		Vec2 s1 = worldToScreen(worldCenter + dir);
		SDL_SetRenderDrawColor(gRenderer, 60, 150, 100, SDL_ALPHA_OPAQUE);
		SDL_RenderLine(gRenderer, (int)s0.x, (int)s0.y, (int)s1.x, (int)s1.y);
	}
}
 
void drawEllipse(Vec2 worldCenter, float rx, float ry, float angle, bool filled, const SDL_FColor& color)
{
	drawShape(worldCenter, ellipseGenerator(worldCenter, rx, ry, angle), filled, color);
}
 
void drawRect(Vec2 worldCenter, float width, float height, float angle, bool filled, const SDL_FColor& color)
{
	drawShape(worldCenter, rectGenerator(worldCenter, width, height, angle), filled, color);
}
 
void drawCapsule(Vec2 worldCenter, float bodyWidth, float bodyHeight, float capHeight, float angle, bool filled, const SDL_FColor& color)
{
	drawShape(worldCenter, capsuleGenerator(worldCenter, bodyWidth, bodyHeight, capHeight, angle), filled, color);
}

void drawPolygon(Vec2 worldCenter, const std::vector<Vec2>& localVerts, float angle, bool filled, const SDL_FColor& color)
{
	std::vector<Vec2> worldPts = polygonGenerator(worldCenter, localVerts, angle);
 
	if (!filled)
	{
		drawShape(worldCenter, worldPts, false, color);
		return;
	}

	// Ear-clipping is rotation-invariant, so just use the original vertices
	sdlVerts = buildScreenVerts(worldPts, color);
 
	auto pointInTriangle = [&](int p, int a, int b, int c) {
		const auto& P = localVerts[p]; const auto& A = localVerts[a]; const auto& B = localVerts[b]; const auto& C = localVerts[c];
		return (B - A).cross(P - A) >= 0.f && (C - B).cross(P - B) >= 0.f && (A - C).cross(P - C) >= 0.f;
	};
 
	float signedArea = 0.0f;
	for (size_t i = 0; i < localVerts.size(); ++i)
	{
		signedArea += localVerts[i].cross(localVerts[(i + 1) % localVerts.size()]);
	}
	bool isCCW = signedArea > 0.0f;
 
	std::vector<int> remaining(localVerts.size());
	std::iota(remaining.begin(), remaining.end(), 0);
	if (!isCCW)
	{
		std::reverse(remaining.begin(), remaining.end());
	}
 
	indices.clear();
	indices.reserve((localVerts.size() - 2) * 3);
 
	while (remaining.size() > 3)
	{
		bool clipped = false;
		size_t n = remaining.size();
		for (size_t i = 0; i < n; ++i)
		{
			int prev = remaining[(i + n - 1) % n];
			int curr = remaining[i];
			int next = remaining[(i + 1) % n];
 
			const auto& A = localVerts[prev]; const auto& B = localVerts[curr]; const auto& C = localVerts[next];
			if ((A - B).cross(C - B) >= 0) continue; // reflex or collinear
 
			bool isEar = true;
			for (size_t j = 0; j < n; ++j)
			{
				int idx = remaining[j];
				if (idx == prev || idx == curr || idx == next) continue;
				if (pointInTriangle(idx, prev, curr, next)) { isEar = false; break; }
			}
 
			if (isEar)
			{
				indices.push_back(prev);
				indices.push_back(curr);
				indices.push_back(next);
				remaining.erase(remaining.begin() + i);
				clipped = true;
				break;
			}
		}
		if (!clipped) break;
	}
 
	if (remaining.size() == 3)
	{
		indices.push_back(remaining[0]);
		indices.push_back(remaining[1]);
		indices.push_back(remaining[2]);
	}
 
	SDL_RenderGeometry(gRenderer, nullptr,
		sdlVerts.data(), static_cast<int>(sdlVerts.size()),
		indices.data(), static_cast<int>(indices.size()));
}

static int CROSSHAIR_SIZE = 10;
void drawPolygonInProgress(Vec2 mouseWorld, const SDL_FColor& color)
{
	const auto& verts = spawnerConfig.polyVerts;

	SDL_SetRenderDrawColor(gRenderer, color.r, color.g, color.b, color.a);
	for (size_t i = 0; i + 1 < verts.size(); ++i)
	{
		Vec2 a = worldToScreen(verts[i]);
		Vec2 b = worldToScreen(verts[i + 1]);
		SDL_RenderLine(gRenderer, a.x, a.y, b.x, b.y);
	}
 
	Vec2 cursor = worldToScreen(mouseWorld);
	int cx = (int)cursor.x, cy = (int)cursor.y;
	
	if (!verts.empty())
	{
		Vec2 last = worldToScreen(verts.back());
		SDL_RenderLine(gRenderer, last.x, last.y, cx, cy);
	}
 
	if (verts.size() >= 3)
	{
		Vec2 first = worldToScreen(verts.front());
		SDL_RenderLine(gRenderer, cx, cy, first.x, first.y);
	}

	// crosshair at the cursor so the very first vertex has visual feedback
	SDL_SetRenderDrawColor(gRenderer, 255, 255, 255, 255);
	SDL_RenderLine(gRenderer, cx - CROSSHAIR_SIZE, cy, cx + CROSSHAIR_SIZE, cy);
	SDL_RenderLine(gRenderer, cx, cy - CROSSHAIR_SIZE, cx, cy + CROSSHAIR_SIZE);
}
 
void drawGhost()
{
	bool isPolygon = spawnerConfig.shapeType == SpawnerConfig::ShapeType::Polygon;
	bool isPlacingPolygon = isPolygon && spawnerConfig.placeMode == SpawnerConfig::PlaceMode::PlacePolygonVertex;
	bool isPlacingPoint = spawnerConfig.placeMode == SpawnerConfig::PlaceMode::PlacePoint;
 
	if (!isPlacingPolygon && !isPlacingPoint) return;
 
	float mx, my;
	SDL_GetMouseState(&mx, &my);
	Vec2 mouseWorld = screenToWorld({ mx, my });
 
	const SDL_FColor ghostColor = { 255, 255, 255, 140 };
 
	switch (spawnerConfig.shapeType)
	{
	case SpawnerConfig::ShapeType::Circle:
		drawCircle(mouseWorld, spawnerConfig.circleRadius, spawnerConfig.rotation, false, ghostColor);
		break;
	case SpawnerConfig::ShapeType::Ellipse:
		drawEllipse(mouseWorld, spawnerConfig.ellipseRx, spawnerConfig.ellipseRy, spawnerConfig.rotation, false, ghostColor);
		break;
	case SpawnerConfig::ShapeType::Rect:
		drawRect(mouseWorld, spawnerConfig.rectWidth, spawnerConfig.rectHeight, spawnerConfig.rotation, false, ghostColor);
		break;
	case SpawnerConfig::ShapeType::Capsule:
		drawCapsule(mouseWorld, spawnerConfig.capsuleBodyWidth, spawnerConfig.capsuleBodyHeight, spawnerConfig.capsuleCapHeight, spawnerConfig.rotation, false, ghostColor);
		break;
	case SpawnerConfig::ShapeType::Polygon:
		if (isPlacingPoint)
		{
			drawPolygon(mouseWorld, spawnerConfig.polyVerts, spawnerConfig.rotation, false, ghostColor);
		}
		else if (isPlacingPolygon)
		{
			drawPolygonInProgress(mouseWorld, ghostColor);
		}
		break;
	}
}

void close()
{
	delete(world);

	NFD_Quit();

	ImGui_ImplSDLRenderer3_Shutdown();
	ImGui_ImplSDL3_Shutdown();
	ImGui::DestroyContext();

	SDL_DestroyRenderer(gRenderer);
	SDL_DestroyWindow(gWindow);
	SDL_Quit();
}