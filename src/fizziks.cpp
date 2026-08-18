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
#include "Fizziks/RigidDef.h"
#include "Fizziks/Shape.h"
#include "Fizziks/Vec.h"
#include "Fizziks/MathUtils.h"
#include "Fizziks/Log.h"
#include "Fizziks/BodyDefBuilder.h"

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

static std::vector<RigidBody> bodies;

Uint32 lt = SDL_GetTicks();

void draw();
void close();

void renderBody(const RigidBody& rb);
void renderCircle(const Circle& circle, const Vec2& pos, val_t angle);
void renderEllipse(const Ellipse& ellipse, const Vec2& pos, const Vec2& centeroidPos, val_t angle, const SDL_FColor& color);
void renderRect(const Rect& rect, const Vec2& pos, const Vec2& centeroidPos, val_t angle, const SDL_FColor& color);
void renderPolygon(const Polygon& polygon, const Vec2& pos, const Vec2& centeroidPos, val_t angle, const SDL_FColor& color);
void renderCapsule(const Capsule& capsule, const Vec2& pos, const Vec2& centroidPos, val_t angle, const SDL_FColor& color);

Vec2 transformToScreenSpace(Vec2 pos);

void createStage()
{
	BodyDef left = BodyDefBuilder().setInitPosition({ 0, 0 })
		.setBodyType(BodyType::STATIC)
		.setColliderDefs({ createColliderDef(createRect(1, 20)) })
		.setRestitution(0.0f)
		.build();

	BodyDef right = BodyDefBuilder().setInitPosition({ 20, 0 })
		.setBodyType(BodyType::STATIC)
		.setColliderDefs({ createColliderDef(createRect(1, 20)) })
		.setRestitution(0.0f)
		.build();

	BodyDef bottom = BodyDefBuilder().setInitPosition({ 10, 0.5 })
		.setBodyType(BodyType::STATIC)
		.setColliderDefs({ createColliderDef(createRect(20, 1)) })
		.setRestitution(0.1f)
		.build();

	bodies.push_back(world->createBody(left));
	bodies.push_back(world->createBody(right));
	bodies.push_back(world->createBody(bottom));
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
				.setGravityScale(1.0f)
				.setRestitution(0.3f)
				.setColliderDefs({ createColliderDef(createCircle(.2f), 0.1f) });

			bodies.push_back(world->createBody(ballBuilder.build()));
		}
	}
}

bool initSDL()
{
	if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD))
	{
		printf("Error: SDL_Init(): %s\n", SDL_GetError());
		return false;
	}

	mainScale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());
	SDL_WindowFlags window_flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN | SDL_WINDOW_HIGH_PIXEL_DENSITY;
	gWindow = SDL_CreateWindow("PlayFizziks", (int)(WIDTH), (int)(HEIGHT), window_flags);
	if (gWindow == nullptr)
	{
		printf("Error: SDL_CreateWindow(): %s\n", SDL_GetError());
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

	std::unique_ptr<SpawnerWindow> spawner = std::make_unique<SpawnerWindow>(&world, spawnerConfig);
	editor->AddEditorWindow(std::move(spawner));

	std::unique_ptr<EnvironmentWindow> enviro = std::make_unique<EnvironmentWindow>(&world, enviroConfig);
	editor->AddEditorWindow(std::move(enviro));

	return true;
}

int main(int argc, char** argv)
{
	bool sdl_error = initSDL();
	bool imgui_error = initImGui();

	if(sdl_error || imgui_error)
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

	createStage();
	createBalls();

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
		}

		float dt = (SDL_GetTicks() - lt) / 1000.f;
		lt = SDL_GetTicks();

		world->tick(dt);
		draw();

		for (auto& req : editor->TakeRequests())
		{
			FizzWorld* newWorld = new FizzWorld();
			bool result = std::visit(RequestHandler{ newWorld }, req);
			if (result)
			{
				delete(world);
				world = newWorld;
			}
			else
			{
				delete(newWorld);
			}
		}
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
			Vec2 dim = transformToScreenSpace(aabb.max - aabb.min);
			Vec2 topLeft = transformToScreenSpace(aabb.min);
			Vec2 topRight = transformToScreenSpace(aabb.max);

			rect.x = topLeft.x; rect.y = HEIGHT - topRight.y;
			rect.w = dim.x; rect.h = dim.y;
			SDL_RenderRect(gRenderer, &rect);
		}
	}

	for (const auto& body : world->getActiveBodies())
	{
		renderBody(body);
	}

	ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), gRenderer);
	SDL_RenderPresent(gRenderer);
}

void renderBody(const RigidBody& body)
{
	SDL_FColor color = SDL_FColor{ 255, 255, 255, SDL_ALPHA_OPAQUE };
	if (body.bodyType() == BodyType::STATIC)
	{
		color = { 160, 150, 30, SDL_ALPHA_OPAQUE };
	}
	else if (body.bodyType() == BodyType::DYNAMIC)
	{
		color = { 100, 60, 150, SDL_ALPHA_OPAQUE };
	}
	else
	{
		color = { 60, 150, 100, SDL_ALPHA_OPAQUE };
	}
	SDL_SetRenderDrawColor(gRenderer, color.r, color.g, color.b, color.a);

	const auto colliders = body.colliders();
	Vec2 bodyPos = body.centroidPosition();
	val_t rot = body.rotation();
	for (const auto& collider : colliders)
	{
		val_t angle = rot + collider.rotation;
		Vec2 localPos = bodyPos + collider.pos;
		Vec2 pos = transformToScreenSpace(localPos);
		const Shape& shape = collider.shape;
		if (std::holds_alternative<Circle>(shape))
		{
			renderCircle(std::get<Circle>(shape), pos, angle);
		}
		else if (std::holds_alternative<Ellipse>(shape))
		{
			renderEllipse(std::get<Ellipse>(shape), pos, localPos, angle, color);
		}
		else if (std::holds_alternative<Rect>(shape))
		{
			renderRect(std::get<Rect>(shape), pos, localPos, angle, color);
		}
		else if (std::holds_alternative<Polygon>(shape))
		{
			renderPolygon(std::get<Polygon>(shape), pos, localPos, angle, color);
		}
		else if (std::holds_alternative<Capsule>(shape))
		{
			renderCapsule(std::get<Capsule>(shape), pos, localPos, angle, color);
		}
	}
}

void renderCircle(const Circle& circle, const Vec2& pos, val_t angle)
{
	val_t r = circle.radius;
	int sr = (int)transformToScreenSpace(Vec2{ r, 0 }).x;

	int x = (int)pos.x;
	int y = HEIGHT - (int)pos.y;

	for (int i = -sr; i <= sr; ++i)
	{
		int dx = (int)std::sqrt(sr * sr - i * i);
		SDL_RenderLine(gRenderer, x - dx, y + i, x + dx, y + i);
	}

	float dx = std::cos(-angle);
	float dy = std::sin(-angle);

	// Endpoints of the diameter
	int x1 = (int)(x - dx * sr);
	int y1 = (int)(y - dy * sr);
	int x2 = (int)(x + dx * sr);
	int y2 = (int)(y + dy * sr);

	SDL_SetRenderDrawColor(gRenderer, 60, 150, 100, SDL_ALPHA_OPAQUE);
	SDL_RenderLine(gRenderer, x1, y1, x2, y2);
}

static std::vector<SDL_Vertex> sdlVerts;
std::vector<int> indices;
void renderEllipse(const Ellipse& ellipse, const Vec2& pos, const Vec2& centeroidPos, val_t angle, const SDL_FColor& color)
{
	const int segments = 64;
	sdlVerts.clear();
	sdlVerts.reserve(segments + 1);

	SDL_Vertex center;
	center.position = SDL_FPoint{ pos.x, HEIGHT - pos.y };
	center.color = color;
	center.tex_coord = SDL_FPoint{ 0, 0 };
	sdlVerts.push_back(center);

	for (int i = 0; i < segments; ++i)
	{
		float theta = (TWO_PI * i) / segments;
		Vec2 local = Vec2{ ellipse.rx * std::cos(theta), ellipse.ry * std::sin(theta) }.rotated(angle);
		Vec2 screen = transformToScreenSpace(centeroidPos + local);

		SDL_Vertex v;
		v.position = SDL_FPoint{ screen.x, HEIGHT - screen.y };
		v.color = color;
		v.tex_coord = SDL_FPoint{ 0, 0 };
		sdlVerts.push_back(v);
	}

	indices.clear();
	indices.reserve(segments * 3);

	for (int i = 1; i <= segments; ++i)
	{
		indices.push_back(0);
		indices.push_back(i);
		indices.push_back(i % segments + 1);
	}

	SDL_RenderGeometry(
		gRenderer,
		nullptr,
		sdlVerts.data(), static_cast<int>(sdlVerts.size()),
		indices.data(), static_cast<int>(indices.size())
	);
}

void renderRect(const Rect& rect, const Vec2& pos, const Vec2& centeroidPos, val_t angle, const SDL_FColor& color)
{
	// Four corners in local space (centered on origin)
	float hw = rect.width * 0.5f;
	float hh = rect.height * 0.5f;

	Vec2 corners[4] = {
		Vec2{ -hw, -hh },
		Vec2{  hw, -hh },
		Vec2{  hw,  hh },
		Vec2{ -hw,  hh },
	};

	SDL_Vertex vertices[4];

	for (int i = 0; i < 4; ++i)
	{
		Vec2 screen = transformToScreenSpace(centeroidPos + corners[i].rotated(angle));
		vertices[i].position = SDL_FPoint{ screen.x, HEIGHT - screen.y };
		vertices[i].color = color;
		vertices[i].tex_coord = SDL_FPoint{ 0, 0 };
	}

	int indices[] = { 0, 1, 2, 0, 2, 3 };

	SDL_RenderGeometry(gRenderer, nullptr, vertices, 4, indices, 6);
}

void renderPolygon(const Polygon& polygon, const Vec2& pos, const Vec2& centeroidPos, val_t angle, const SDL_FColor& color)
{
	const auto& verts = polygon.vertices;

	// Compute centroid of the raw vertices so we can render centroid-relative,
	// matching how the physics internally represents the shape
	Vec2 centroid = Vec2::Zero();
	val_t area = 0;
	for (size_t i = 0; i < verts.size(); ++i)
	{
		const Vec2& v0 = verts[i];
		const Vec2& v1 = verts[(i + 1) % verts.size()];
		val_t cross = v0.cross(v1);
		centroid += (v0 + v1) * cross;
		area += cross;
	}
	if (area != 0) centroid /= (3 * area);

	sdlVerts.clear();
	sdlVerts.reserve(verts.size());

	for (const auto& vert : verts)
	{
		Vec2 screenV = transformToScreenSpace(centeroidPos + (vert - centroid).rotated(angle));
		SDL_Vertex sdlVert;
		sdlVert.position = SDL_FPoint{ screenV.x, HEIGHT - screenV.y };
		sdlVert.color = color;
		sdlVert.tex_coord = SDL_FPoint{ 0, 0 };
		sdlVerts.push_back(sdlVert);
	}

	auto pointInTriangle = [&](int p, int a, int b, int c) {
		const auto& P = verts[p]; const auto& A = verts[a]; const auto& B = verts[b]; const auto& C = verts[c];
		return (B - A).cross(P - A) >= 0.f &&
			(C - B).cross(P - B) >= 0.f &&
			(A - C).cross(P - C) >= 0.f;
		};

	std::vector<int> remaining(verts.size());
	std::iota(remaining.begin(), remaining.end(), 0);

	indices.clear();
	indices.reserve((verts.size() - 2) * 3);

	while (remaining.size() > 3)
	{
		bool clipped = false;
		for (size_t i = 0; i < remaining.size(); ++i)
		{
			int prev = remaining[(i + remaining.size() - 1) % remaining.size()];
			int curr = remaining[i];
			int next = remaining[(i + 1) % remaining.size()];

			const auto& A = verts[prev]; const auto& B = verts[curr]; const auto& C = verts[next];
			if ((A - B).cross(C - B) >= 0) continue; // reflex or collinear

			bool isEar = true;
			for (size_t j = 0; j < remaining.size(); ++j)
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

void renderCapsule(const Capsule& capsule, const Vec2& pos, const Vec2& centroidPos, val_t angle, const SDL_FColor& color)
{
	const int hemisphereSegments = 16; // per hemisphere
	const float r = capsule.capHeight; // for now we'll just assume the cap perfectly covers the top/bottom
	const float halfLen = capsule.body.height * 0.5f;

	// Build verts: top hemisphere → bottom hemisphere
	std::vector<SDL_Vertex> sdlVerts;
	sdlVerts.reserve(hemisphereSegments * 2);

	// Top hemisphere (center offset +halfLen along local Y)
	for (int i = 0; i < hemisphereSegments; ++i)
	{
		float theta = (PI * i) / (hemisphereSegments - 1); // 0 → PI (left to right)
		Vec2 local = Vec2{ r * std::cos(theta), halfLen + r * std::sin(theta) }.rotated(angle);
		Vec2 screen = transformToScreenSpace(centroidPos + local);

		SDL_Vertex v;
		v.position = SDL_FPoint{ screen.x, HEIGHT - screen.y };
		v.color = color;
		v.tex_coord = SDL_FPoint{ 0, 0 };
		sdlVerts.push_back(v);
	}

	// Bottom hemisphere (center offset -halfLen along local Y)
	for (int i = 0; i < hemisphereSegments; ++i)
	{
		float theta = (PI * i) / (hemisphereSegments - 1); // PI → 0 (right to left)
		Vec2 local = Vec2{ -r * std::cos(theta), -halfLen - r * std::sin(theta) }.rotated(angle);
		Vec2 screen = transformToScreenSpace(centroidPos + local);

		SDL_Vertex v;
		v.position = SDL_FPoint{ screen.x, HEIGHT - screen.y };
		v.color = color;
		v.tex_coord = SDL_FPoint{ 0, 0 };
		sdlVerts.push_back(v);
	}

	// Triangle fan from vertex 0 around the whole perimeter
	std::vector<int> indices;
	const int total = static_cast<int>(sdlVerts.size());
	indices.reserve((total - 2) * 3);

	for (int i = 1; i + 1 < total; ++i)
	{
		indices.push_back(0);
		indices.push_back(i);
		indices.push_back(i + 1);
	}

	// Close the shape: last vert back to first
	indices.push_back(0);
	indices.push_back(total - 1);
	indices.push_back(0); // degenerate close — or add a final stitch if needed

	SDL_RenderGeometry(
		gRenderer,
		nullptr,
		sdlVerts.data(), static_cast<int>(sdlVerts.size()),
		indices.data(), static_cast<int>(indices.size())
	);
}

Vec2 transformToScreenSpace(Vec2 pos)
{
	Vec2 scale = world->worldScale();
	return { pos.x * WIDTH / scale.x, pos.y * HEIGHT / scale.y };
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
