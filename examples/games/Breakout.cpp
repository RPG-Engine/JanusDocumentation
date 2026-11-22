// Breakout: bounce the ball off your paddle to clear every brick. You have three balls.
//
// Controls: A / D or Left / Right to move, Space to launch the ball, R to restart.
//
// Shows: velocity-based movement, bouncing off walls and rectangles, destroying
// entities on hit, counting entities with a query to detect a win, and several
// input actions sharing keys. Uses no files.

#include <Module.hpp>
#include <Log/Module.hpp>
#include <Input/Module.hpp>
#include <Editor/Module.hpp>
#include <Filesystem/Module.hpp>
#include <Types/Graphics.hpp>
#include <Gameplay/ECS/Module.hpp>
#include <Gameplay/ECS/Modules/TransformModule.hpp>
#include <Gameplay/ECS/Modules/GraphicsModule.hpp>
#include <Rendering/Components/RenderingComponent.hpp>

#include <imgui.h>
#include <glm/geometric.hpp>
#include <glm/common.hpp>
#include <algorithm>
#include <cmath>
#include <cstdlib>

// The engine's components (FTransformComponent, FTexture2DComponent, ...).
using namespace Components;

DECLARE_LOGGER(Breakout, debug)

//! [components]
DEFINE_INPUT_ACTION(MoveX, float)
DEFINE_INPUT_ACTION(Launch, bool)
DEFINE_INPUT_ACTION(Restart, bool)

namespace Components {
    DEFINE_COMPONENT(FPaddle)
    DEFINE_COMPONENT(FBall,
        ECS_MEMBER(glm::vec2, Velocity, glm::vec2(0, 0)))
    DEFINE_COMPONENT(FBrick,
        ECS_MEMBER(float, Points, 10.f))
}
//! [components]

constexpr float ScreenW = 320.f;
constexpr float ScreenH = 180.f;
constexpr float BallSize = 5.f;
constexpr float StartSpeed = 140.f;

struct SBreakoutState {
    int Score = 0;
    int Lives = 3;
    bool bBallOnPaddle = true;
    bool bWon = false;
    bool bLost = false;
};
static SBreakoutState Game;

// ---- Scene helpers (the same in every example) -------------------------------
static void RegisterSolidTexture(const std::string& AssetPath, int Size) {
    SDL_Surface* surface = SDL_CreateSurface(Size, Size, SDL_PIXELFORMAT_ABGR8888);
    const Uint32 white = SDL_MapSurfaceRGBA(surface, 255, 255, 255, 255);
    SDL_FillSurfaceRect(surface, nullptr, white);

    TObjectPtr<Graphics::STexture2D> texture;
    texture->Surface = surface;
    texture->Width = static_cast<float>(Size);
    texture->Height = static_cast<float>(Size);
    texture->AssetPath = AssetPath;

    TObjectPtr<Filesystem::SAssetData> assetData;
    assetData->AssetPath = AssetPath;
    assetData->AssetName = AssetPath;
    assetData->AssetType = Filesystem::TEXTURE;
    assetData->ObjectData = texture;
    Filesystem::RegisterAssetData(assetData);
}

static void CreateCameraAndBackground(const flecs::world& World, SDL_FColor Color) {
    World.entity("Camera")
        .set<FTransformComponent>({})
        .set<FCameraComponent>({
            .CameraSize = {ScreenW, ScreenH},
            .Boundaries = {0, 0, 0, 0},
        })
        .set<FLightComponent>({ .Radius = 100.f, .Intensity = 2.4f });

    World.entity("Background")
        .set<FTransformComponent>({})
        .set<FTexture2DComponent>({
            .Sprite = { .SpriteSheet = {"White"} },
            .TextureSize = {ScreenW, ScreenH},
            .Color = Color,
        })
        .set<FRenderingComponent>({ .RenderLayer = Renderer::Background });
}

static flecs::entity SpawnRect(const flecs::world& World, glm::vec2 Position,
                               glm::vec2 Size, SDL_FColor Color) {
    return World.entity()
        .set<FTransformComponent>({ .Location = {Position.x, Position.y, 0.f} })
        .set<FTexture2DComponent>({
            .Sprite = { .SpriteSheet = {"White"} },
            .TextureSize = Size,
            .Color = Color,
        })
        .set<FRenderingComponent>({ .RenderLayer = Renderer::Entities });
}

static glm::vec2 PositionOf(const FTransformComponent& Transform) {
    return {Transform.Location.x, Transform.Location.y};
}

static bool Overlaps(glm::vec2 PosA, glm::vec2 SizeA, glm::vec2 PosB, glm::vec2 SizeB) {
    return PosA.x < PosB.x + SizeB.x && PosB.x < PosA.x + SizeA.x &&
           PosA.y < PosB.y + SizeB.y && PosB.y < PosA.y + SizeA.y;
}

//! [brick_wall]
// 10 columns x 5 rows of bricks. Higher rows are worth more.
static void SpawnBricks(const flecs::world& World) {
    const SDL_FColor rowColors[5] = {
        {1.f, 0.35f, 0.35f, 1.f}, {1.f, 0.6f, 0.3f, 1.f}, {1.f, 0.85f, 0.3f, 1.f},
        {0.45f, 0.85f, 0.4f, 1.f}, {0.35f, 0.65f, 1.f, 1.f}};
    for (int row = 0; row < 5; ++row) {
        for (int col = 0; col < 10; ++col) {
            const glm::vec2 position {11.f + col * 30.f, 22.f + row * 10.f};
            const float points = static_cast<float>((5 - row) * 10);
            SpawnRect(World, position, {28.f, 8.f}, rowColors[row])
                .set<FBrick>({ .Points = points });
        }
    }
}
//! [brick_wall]

class BreakoutHud : public ImGUIWrapper::IEditorTool {
protected:
    void Draw_Internal() override {
        ImGui::SetWindowCollapsed("Dear ImGui Demo", true, ImGuiCond_Once);

        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        const ImVec2 topRight {viewport->Pos.x + viewport->Size.x - 16.f,
                               viewport->Pos.y + 16.f};
        ImGui::SetNextWindowPos(topRight, ImGuiCond_Always, {1.f, 0.f});
        const ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration
                                     | ImGuiWindowFlags_AlwaysAutoResize
                                     | ImGuiWindowFlags_NoInputs;
        ImGui::Begin("Breakout", nullptr, flags);
        ImGui::Text("Score %d    Balls %d", Game.Score, Game.Lives);
        const ImVec4 green {0.5f, 1.f, 0.5f, 1.f};
        const ImVec4 red {1.f, 0.4f, 0.4f, 1.f};
        if (Game.bWon) {
            ImGui::TextColored(green, "Cleared! Press R to play again.");
        } else if (Game.bLost) {
            ImGui::TextColored(red, "Out of balls. Press R to restart.");
        } else if (Game.bBallOnPaddle) {
            ImGui::TextDisabled("Space to launch");
        }
        ImGui::End();
    }
};

int main() {
    RegisterSolidTexture("CollisionTexture", 8);
    RegisterSolidTexture("White", 8);

    Engine::InitializePlugins();
    flecs::world& world = ECS::GetWorld();

    ECS_REGISTER_COMPONENT(FPaddle, world);
    ECS_REGISTER_COMPONENT(FBall, world);
    ECS_REGISTER_COMPONENT(FBrick, world);

    Input::AddInputBindings_Hold(&Input::Actions::MoveX, {
        {SDL_SCANCODE_A, -1.f}, {SDL_SCANCODE_LEFT, -1.f},
        {SDL_SCANCODE_D,  1.f}, {SDL_SCANCODE_RIGHT, 1.f},
    });
    Input::AddInputBindings_Press(&Input::Actions::Launch, {
        {SDL_SCANCODE_SPACE, true},
    });
    Input::AddInputBindings_Press(&Input::Actions::Restart, { {SDL_SCANCODE_R, true} });

    //! [scene]
    CreateCameraAndBackground(world, {0.08f, 0.06f, 0.12f, 1.f});

    const glm::vec2 paddleStart {(ScreenW - 40.f) / 2.f, 166.f};
    SpawnRect(world, paddleStart, {40.f, 5.f}, {0.9f, 0.9f, 1.f, 1.f})
        .set_name("Paddle")
        .add<FPaddle>();
    SpawnRect(world, {0.f, 0.f}, {BallSize, BallSize}, {1.f, 1.f, 1.f, 1.f})
        .set_name("Ball")
        .set<FBall>({});
    SpawnBricks(world);

    auto bricks = world.query<const FTransformComponent,
                              const FTexture2DComponent,
                              const FBrick>();
    //! [scene]

    //! [paddle]
    // Paddle: follows the MoveX action.
    world.system<FTransformComponent,
                 const FTexture2DComponent,
                 const FPaddle>("MovePaddle")
        .each([](flecs::iter& it, size_t, FTransformComponent& transform,
                 const FTexture2DComponent& sprite, const FPaddle&) {
            const float moveX = Input::GetValue(Input::Actions::MoveX);
            transform.Location.x += moveX * 180.f * it.delta_time();
            const float maxX = ScreenW - sprite.TextureSize.x;
            transform.Location.x = std::clamp(transform.Location.x, 0.f, maxX);
        });
    //! [paddle]

    // Ball: sits on the paddle until launched, then moves and bounces.
    world.system<FTransformComponent, FBall>("MoveBall")
        .each([bricks](flecs::iter& it, size_t, FTransformComponent& transform,
                       FBall& ball) {
            const flecs::world world = it.world();

            if (Input::IsActionTriggered(Input::Actions::Restart)) {
                bricks.each([](flecs::entity brick, const FTransformComponent&,
                               const FTexture2DComponent&, const FBrick&) {
                    brick.destruct();
                });
                SpawnBricks(world);
                Game = {};
                return;
            }
            if (Game.bWon || Game.bLost) {
                return;
            }

            //! [ball_launch]
            const flecs::entity paddle = world.lookup("Paddle");
            const glm::vec2 paddlePos = PositionOf(paddle.get<FTransformComponent>());
            const glm::vec2 paddleSize = paddle.get<FTexture2DComponent>().TextureSize;

            if (Game.bBallOnPaddle) {
                const float x = paddlePos.x + (paddleSize.x - BallSize) / 2.f;
                const float y = paddlePos.y - BallSize - 1.f;
                transform.Location = {x, y, 0.f};
                if (Input::IsActionTriggered(Input::Actions::Launch)) {
                    ball.Velocity = glm::normalize(glm::vec2(0.5f, -1.f)) * StartSpeed;
                    Game.bBallOnPaddle = false;
                }
                return;
            }
            //! [ball_launch]

            glm::vec2 pos = PositionOf(transform) + ball.Velocity * it.delta_time();

            //! [ball_walls]
            // Walls
            if (pos.x < 0.f) {                       // left wall
                pos.x = 0.f;
                ball.Velocity.x = std::abs(ball.Velocity.x);
            }
            if (pos.x > ScreenW - BallSize) {        // right wall
                pos.x = ScreenW - BallSize;
                ball.Velocity.x = -std::abs(ball.Velocity.x);
            }
            if (pos.y < 0.f) {                       // ceiling
                pos.y = 0.f;
                ball.Velocity.y = std::abs(ball.Velocity.y);
            }
            if (pos.y > ScreenH) {                   // missed: back to the paddle
                Game.bBallOnPaddle = true;
                Game.bLost = --Game.Lives <= 0;
                return;
            }

            // Paddle: the further from the centre the ball hits, the sharper the angle.
            const glm::vec2 ballSize {BallSize, BallSize};
            const bool bFalling = ball.Velocity.y > 0.f;
            if (bFalling && Overlaps(pos, ballSize, paddlePos, paddleSize)) {
                const float speed = glm::length(ball.Velocity);
                const float ballCentre = pos.x + BallSize / 2.f;
                const float paddleCentre = paddlePos.x + paddleSize.x / 2.f;
                const float offset = (ballCentre - paddleCentre) / (paddleSize.x / 2.f);
                const float sideways = std::clamp(offset, -1.f, 1.f) * 1.2f;
                ball.Velocity = glm::normalize(glm::vec2(sideways, -1.f)) * speed;
                pos.y = paddlePos.y - BallSize;
            }
            //! [ball_walls]

            //! [ball_bricks]
            // Bricks: bounce off the first one hit,
            // along the axis with the smaller overlap.
            bool bHitBrick = false;
            bricks.each([&](flecs::entity brick,
                            const FTransformComponent& brickTransform,
                            const FTexture2DComponent& brickSprite,
                            const FBrick& brickData) {
                const glm::vec2 brickPos = PositionOf(brickTransform);
                const glm::vec2 brickSize = brickSprite.TextureSize;
                if (bHitBrick || !Overlaps(pos, ballSize, brickPos, brickSize)) {
                    return;
                }
                bHitBrick = true;

                // How far the ball and the brick overlap on each axis
                const glm::vec2 overlapMin = glm::max(pos, brickPos);
                const glm::vec2 overlapMax = glm::min(pos + ballSize,
                                                      brickPos + brickSize);
                const float overlapX = overlapMax.x - overlapMin.x;
                const float overlapY = overlapMax.y - overlapMin.y;
                if (overlapX < overlapY) {
                    ball.Velocity.x = -ball.Velocity.x;
                } else {
                    ball.Velocity.y = -ball.Velocity.y;
                }
                ball.Velocity *= 1.02f;   // speed up a little with every brick

                Game.Score += static_cast<int>(brickData.Points);
                brick.destruct();
            });

            // The destroyed brick is still counted until the deferred destruct
            // is applied, so compare with 1.
            if (bHitBrick && bricks.count() <= 1) {
                Game.bWon = true;
                Log::Breakout::Info("Cleared the board with {} points", Game.Score);
            }
            //! [ball_bricks]

            transform.Location = {pos.x, pos.y, 0.f};
        });

    CoreEditor::Open<BreakoutHud>();
    Log::Breakout::Info("Breakout started. Close the window to quit.");

    Engine::Run();
    Engine::TeardownPlugins();
    // Known issue: avoid the crash in static destructors after teardown.
    std::quick_exit(0);
}
