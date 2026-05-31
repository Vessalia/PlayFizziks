#include <iostream>
#include <chrono>
#include <vector>
#include <numeric>

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

const unsigned int SCREEN_WIDTH = 920;
const unsigned int SCREEN_HEIGHT = 920;

static SDL_Window* gWindow = nullptr;
static SDL_Renderer* gRenderer = nullptr;

static FizzWorld world = FizzWorld(20, 20, 5, 1 / 30.f, Fizziks::FizzWorld::AccelStruct::BVH);

static std::vector<RigidBody> bodies;

Uint32 lt = SDL_GetTicks();

val_t timescale = 1.f;

void draw();
void close();

void renderCircle(const Circle& circle, const Vec2& pos, val_t angle);
void renderEllipse(const Ellipse& ellipse, const Vec2& pos, const Vec2& centeroidPos, val_t angle, const SDL_FColor& color);
void renderRect(const Rect& rect, const Vec2& pos, const Vec2& centeroidPos, val_t angle, const SDL_FColor& color);
void renderPolygon(const Polygon& polygon, const Vec2& pos, const Vec2& centeroidPos, val_t angle, const SDL_FColor& color);
void renderCapsule(const Capsule& capsule, const Vec2& pos, const Vec2& centroidPos, val_t angle, const SDL_FColor& color);

Vec2 transformToScreenSpace(Vec2 pos);

int main(int argc, char** argv)
{
	gWindow = SDL_CreateWindow("Fizziks Test", SCREEN_WIDTH, SCREEN_HEIGHT, 0);
	gRenderer = SDL_CreateRenderer(gWindow, NULL);

	world.Gravity = Vec2::Zero();

	Fizziks::SinkOptions options;
	options.threadSafe = true;
	addLogSink([](Fizziks::LogLevel level, std::string_view msg, std::string_view file, int line)
		{ 
			std::cout << "level = " << toString(level) << ": msg = " << msg << ": file = " << file << ": line = " << line << std::endl;
		}, options
	);

	BodyDef big = BodyDefBuilder().setInitPosition({ 20, 5 })
						 		  .setInitVelocity({ -3, 0 })
								  .setInitAngularVelocity(1)
								  .setColliderDefs({ createColliderDef(createPolygon({Vec2(0, 0), Vec2(1, 0), Vec2(1, 1), Vec2(2, 1), Vec2(2, 2), Vec2(0, 2)}), 10)})
								  .build();
	bodies.push_back(world.createBody(big));

	BodyDef small;
	small.colliderDefs.push_back({ createColliderDef(createRect(0.35, 0.35), 1) });
	for (int i = 0; i < 100; ++i)
	{
		for (int j = 0; j < 210; ++j)
		{
			small.initPosition = { i * 0.4f, j * 0.4f};
			bodies.push_back(world.createBody(small));
		}
	}

	bool quit = false;
	bool pause = false;
	while (!quit)
	{
		SDL_Event e;
		while (SDL_PollEvent(&e))
		{
			if (e.type == SDL_EVENT_QUIT)
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

				case SDLK_SPACE:
					pause = !pause;
					break;

				case SDLK_G:
					world.Gravity = world.Gravity == Vec2::Zero() ? Vec2(0, -9.8) : Vec2::Zero();
					break;

				case SDLK_PLUS:
					if (timescale >= 1) ++timescale;
					else
					{
						int x = static_cast<int>(std::round(1 / timescale));
						timescale = x > 2 ? 1.f / (x - 1) : 1;
					}
					break;

				case SDLK_MINUS:
					if (timescale > 1) --timescale;
					else
					{
						int x = static_cast<int>(std::round(1 / timescale));
						timescale = 1.f / (x + 1);
					}
					break;
				}
			}
		}

		float dt = (SDL_GetTicks() - lt) / 1000.f;
		lt = SDL_GetTicks();

		if (pause) dt = 0;
		world.tick(dt * timescale);
		draw();
	}

	close();
}

void draw()
{
	SDL_SetRenderDrawColor(gRenderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
	SDL_RenderClear(gRenderer);

	auto info = world.getBroadphaseDebugInfo();
	SDL_SetRenderDrawColor(gRenderer, 255, 0, 255, 255);
	for (const auto& aabb : info)
	{
		SDL_FRect rect;
		Vec2 dim = transformToScreenSpace(aabb.max - aabb.min);
		Vec2 topLeft = transformToScreenSpace(aabb.min);
		Vec2 topRight = transformToScreenSpace(aabb.max);

		rect.x = topLeft.x; rect.y = SCREEN_HEIGHT - topRight.y;
		rect.w = dim.x; rect.h = dim.y;
		SDL_RenderRect(gRenderer, &rect);
	}

	for(const auto& body : bodies)
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
		for (auto collider : colliders)
		{
			val_t angle = rot + collider.rotation;
			Vec2 localPos = bodyPos + collider.pos;
			Vec2 pos = transformToScreenSpace(localPos);
			Shape shape = collider.shape;
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

	SDL_RenderPresent(gRenderer);
}

void renderCircle(const Circle& circle, const Vec2& pos, val_t angle)
{
	val_t r = circle.radius;
	int sr = (int)transformToScreenSpace(Vec2{ r, 0 }).x;

	int x = (int)pos.x;
	int y = SCREEN_HEIGHT - (int)pos.y;

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

void renderEllipse(const Ellipse& ellipse, const Vec2& pos, const Vec2& centeroidPos, val_t angle, const SDL_FColor& color)
{
	const int segments = 64;

	std::vector<SDL_Vertex> sdlVerts;
	sdlVerts.reserve(segments + 1);

	SDL_Vertex center;
	center.position = SDL_FPoint{ pos.x, SCREEN_HEIGHT - pos.y };
	center.color = color;
	center.tex_coord = SDL_FPoint{ 0, 0 };
	sdlVerts.push_back(center);

	for (int i = 0; i < segments; ++i)
	{
		float theta = (TWO_PI * i) / segments;
		Vec2 local = Vec2{ ellipse.rx * std::cos(theta), ellipse.ry * std::sin(theta) }.rotated(angle);
		Vec2 screen = transformToScreenSpace(centeroidPos + local);

		SDL_Vertex v;
		v.position = SDL_FPoint{ screen.x, SCREEN_HEIGHT - screen.y };
		v.color = color;
		v.tex_coord = SDL_FPoint{ 0, 0 };
		sdlVerts.push_back(v);
	}

	std::vector<int> indices;
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

	std::vector<SDL_Vertex> sdlVerts;
	sdlVerts.reserve(4);

	for (const auto& corner : corners)
	{
		Vec2 screen = transformToScreenSpace(centeroidPos + corner.rotated(angle));

		SDL_Vertex v;
		v.position = SDL_FPoint{ screen.x, SCREEN_HEIGHT - screen.y };
		v.color = color;
		v.tex_coord = SDL_FPoint{ 0, 0 };
		sdlVerts.push_back(v);
	}

	// Two triangles: [0,1,2] and [0,2,3]
	std::vector<int> indices = { 0, 1, 2, 0, 2, 3 };

	SDL_RenderGeometry(
		gRenderer,
		nullptr,
		sdlVerts.data(), static_cast<int>(sdlVerts.size()),
		indices.data(), static_cast<int>(indices.size())
	);
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

	std::vector<SDL_Vertex> sdlVerts;
	sdlVerts.reserve(verts.size());

	for (const auto& vert : verts)
	{
		Vec2 screenV = transformToScreenSpace(centeroidPos + (vert - centroid).rotated(angle));
		SDL_Vertex sdlVert;
		sdlVert.position = SDL_FPoint{ screenV.x, SCREEN_HEIGHT - screenV.y };
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

	std::vector<int> indices;
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
		v.position = SDL_FPoint{ screen.x, SCREEN_HEIGHT - screen.y };
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
		v.position = SDL_FPoint{ screen.x, SCREEN_HEIGHT - screen.y };
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
	Vec2 scale = world.worldScale();
	return { pos.x * SCREEN_WIDTH / scale.x, pos.y * SCREEN_HEIGHT / scale.y };
}

void close()
{
	SDL_DestroyRenderer(gRenderer);
	SDL_DestroyWindow(gWindow);
}
