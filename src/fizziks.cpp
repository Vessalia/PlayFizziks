#include <iostream>
#include <chrono>
#include <vector>

#include "SDL3/SDL.h"

#include "Fizziks/Fizziks.h"
#include "Fizziks/FizzWorld.h"
#include "Fizziks/RigidBody.h"
#include "Fizziks/RigidDef.h"
#include "Fizziks/Shape.h"
#include "Fizziks/Vec.h"
#include "Fizziks/MathUtils.h"

using namespace Fizziks;

const unsigned int SCREEN_WIDTH = 920;
const unsigned int SCREEN_HEIGHT = 920;

static SDL_Window* gWindow = nullptr;
static SDL_Renderer* gRenderer = nullptr;

static FizzWorld world;

static std::vector<RigidBody> bodies;

Uint32 lt = SDL_GetTicks();

val_t timescale = 1.f;

void draw();
void close();

Vec2 transformToScreenSpace(Vec2 pos);

int main(int argc, char** argv) 
{
    gWindow = SDL_CreateWindow("Fizziks Test", SCREEN_WIDTH, SCREEN_HEIGHT, 0);
    gRenderer = SDL_CreateRenderer(gWindow, NULL);

    world.Gravity = Vec2::Zero();

    BodyDef small;
    small.colliderDefs.push_back({ createCollider(createRect(0.25, 0.25), 1, 0), Vec2::Zero() });
    for (int i = 0; i < 10; ++i)
    {
        for (int j = 0; j < 10; ++j)
        {
            small.initPosition = { i / 2.f, j / 2.f };
            bodies.push_back(world.createBody(small));
        }
    }

    BodyDef big;
    big.colliderDefs.push_back({ createCollider(createCircle(1.4), 10, 0), Vec2::Zero() });
    big.initPosition = { 20, 5 };
    big.initVelocity = { -3, 0 };
    bodies.push_back(world.createBody(big));

    bool quit = false;
    while (!quit)
    {
        SDL_Event e;
        while (SDL_PollEvent(&e))
        {
            if (e.type == SDL_EVENT_QUIT)
            {
                quit = true;
            }
        }

        float dt = (SDL_GetTicks() - lt) / 1000.f;
        lt = SDL_GetTicks();

        SDL_SetRenderDrawColor(gRenderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
        SDL_RenderClear(gRenderer);

        world.tick(dt / timescale);
        draw();
    }

    close();
}

void draw()
{
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
        for (auto [collider, colliderPos] : colliders)
        {
            val_t angle = rot + collider.rotation;
            Vec2 localPos = bodyPos + colliderPos;
            Vec2 pos = transformToScreenSpace(localPos);
            Shape shape = collider.shape;
            if (shape.type == ShapeType::CIRCLE)
            {
                val_t r = std::get<Circle>(shape.data).radius;
                int sr = (int)transformToScreenSpace(Vec2{r, 0}).x;

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
            else if (shape.type == ShapeType::POLYGON)
            {
                const auto& verts = std::get<Polygon>(shape.data).vertices;

                std::vector<SDL_Vertex> sdlVerts;
                sdlVerts.reserve(verts.size());

                for (const auto& vert : verts)
                {
                    auto v = vert.rotated(angle);
                    Vec2 screenV = transformToScreenSpace(localPos + v);

                    SDL_Vertex sdlVert;
                    sdlVert.position = SDL_FPoint{ screenV.x, SCREEN_HEIGHT - screenV.y };
                    sdlVert.color = color;
                    sdlVert.tex_coord = SDL_FPoint{ 0, 0 };
                    sdlVerts.push_back(sdlVert);
                }

                // Build triangle fan indices
                std::vector<int> indices;
                indices.reserve((verts.size() - 2) * 3);

                for (size_t i = 1; i + 1 < sdlVerts.size(); ++i)
                {
                    indices.push_back(0);
                    indices.push_back(static_cast<int>(i));
                    indices.push_back(static_cast<int>(i + 1));
                }

                // Single draw call
                SDL_RenderGeometry(
                    gRenderer,
                    nullptr,
                    sdlVerts.data(), static_cast<int>(sdlVerts.size()),
                    indices.data(), static_cast<int>(indices.size())
                );
            }
        }
    }
    SDL_RenderPresent(gRenderer);
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
