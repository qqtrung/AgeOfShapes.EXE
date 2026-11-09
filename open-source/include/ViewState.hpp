#ifndef VIEWSTATE_HPP
#define VIEWSTATE_HPP

#include "ClientGame.hpp"
#include "DeterministicRandom.hpp"
#include "GameState.hpp"
#include "SystemManager.hpp"
#include "SystemViewState.hpp"
#include "ViewStateDetail.hpp"

#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/RenderTexture.hpp>
#include <SFML/Graphics/Shader.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>

#include <memory>
#include <unordered_map>

class ViewState
{
public:
    struct ProfilerData
    {
        float                        updateTime    = 0.f;
        float                        renderTime    = 0.f;
        int                          fps           = 0;
        int                          friendlyCount = 0;
        int                          opponentCount = 0;
        int                          projCount     = 0;
        int                          vfxCount      = 0;
        std::map<std::string, float> systemTimes;
        std::map<std::string, float> renderTimes;
    };

public:
    using PresentationPack = SystemManager<SystemViewState::SystemCamera,
                                           SystemViewState::SystemAnimation,
                                           SystemViewState::SystemDragDrop,
                                           SystemViewState::SystemSelection>;

    ClientGame* mClientGame{nullptr};

    DeterministicRandom mRenderRandom{54321};
    size_t              mCurrentFrame = 0;
    ProfilerData        mProfiler;
    sf::VertexArray     mVertices{sf::PrimitiveType::Triangles};
    sf::VertexArray     mGrid{sf::PrimitiveType::Lines};
    sf::VertexArray     mVisionTriangles{sf::PrimitiveType::Triangles};
    uint64_t            mLastVisionVersion = 0;
    std::vector<bool>   mProcessedVision;

    ViewStateDetail::Camera3D mCamera3D;

    sf::Texture            mRawFogTexture;
    sf::RenderTexture      mTempFogTexture;
    sf::RenderTexture      mFogTexture;
    std::vector<sf::Color> mFogPixels;

    static std::unique_ptr<sf::Shader> sFogFilterShader;
    static bool                        sFogFilterShaderLoaded;
    static std::unique_ptr<sf::Shader> sFogShader;
    static bool                        sFogShaderLoaded;
    static void                        initFogShader();
    void                               drawFogCanopy(sf::RenderTarget&                target,
                                                     const ViewStateDetail::Camera3D& cam,
                                                     const sf::FloatRect&             frustum,
                                                     sf::Vector2f                     mapDim,
                                                     float                            time);
    void                               drawFogMeshWithShader(sf::RenderTarget& target, const sf::VertexArray& mesh);
    void                               filterFogTexture();

    PresentationPack mSystems;

    ViewState(ClientGame* clientGame);

    [[nodiscard]] float timeScale() const
    {
        return static_cast<float>(mClientGame->gameState().timeScale());
    }

    [[nodiscard]] SystemViewState::SystemSelection::DebugOptions& debug()
    {
        return mSystems.get<SystemViewState::SystemSelection>().mDebug;
    }
    [[nodiscard]] const SystemViewState::SystemSelection::DebugOptions& debug() const
    {
        return mSystems.get<SystemViewState::SystemSelection>().mDebug;
    }
    [[nodiscard]] CInput& input()
    {
        return mSystems.get<SystemViewState::SystemSelection>().mInput;
    }
    [[nodiscard]] const CInput& input() const
    {
        return mSystems.get<SystemViewState::SystemSelection>().mInput;
    }
    [[nodiscard]] sf::View& worldView()
    {
        return mSystems.get<SystemViewState::SystemCamera>().mWorldView;
    }
    [[nodiscard]] const sf::View& worldView() const
    {
        return mSystems.get<SystemViewState::SystemCamera>().mWorldView;
    }
    [[nodiscard]] sf::View& uiView()
    {
        return mClientGame->mGame->uiView();
    }

    [[nodiscard]] const sf::View& uiView() const
    {
        return mClientGame->mGame->uiView();
    }

    [[nodiscard]] sf::RenderWindow& window()
    {
        return mClientGame->window();
    }

    [[nodiscard]] const sf::RenderWindow& window() const
    {
        return mClientGame->window();
    }

    [[nodiscard]] float& zoomLevel()
    {
        return mSystems.get<SystemViewState::SystemCamera>().mZoomLevel;
    }

    [[nodiscard]] const float& zoomLevel() const
    {
        return mSystems.get<SystemViewState::SystemCamera>().mZoomLevel;
    }

    [[nodiscard]] const SystemGameState::SystemInit::CampaignState& campaign() const
    {
        return mClientGame->gameState().campaign();
    }

    [[nodiscard]] float mapWidth() const
    {
        return static_cast<float>(mClientGame->gameState().mapWidth());
    }

    [[nodiscard]] float mapHeight() const
    {
        return static_cast<float>(mClientGame->gameState().mapHeight());
    }

    [[nodiscard]] ViewStateDetail::Camera3D& camera3D()
    {
        return mCamera3D;
    }

    [[nodiscard]] const ViewStateDetail::Camera3D& camera3D() const
    {
        return mCamera3D;
    }

    [[nodiscard]] sf::Vector2f project3D(float x, float y, float z = 0.f) const
    {
        return mCamera3D.project(x, y, z);
    }

    [[nodiscard]] sf::Vector2f project3D(sf::Vector2f groundPos, float z = 0.f) const
    {
        return mCamera3D.project(groundPos.x, groundPos.y, z);
    }

    [[nodiscard]] sf::Vector2f screenToWorldGround(sf::Vector2i pixelPos);

    int               mPlayerEntityId = -1;
    [[nodiscard]] int playerEntityId() const
    {
        return mPlayerEntityId;
    }
    [[nodiscard]] int  getPlayerClientId(int entityId) const;
    [[nodiscard]] int  localTeamId() const;
    [[nodiscard]] bool isOpponent(int entityId) const;
    [[nodiscard]] bool isFriendly(int entityId) const;
    [[nodiscard]] bool isLocalVictory() const;

    std::vector<Command> mPendingCommands;
    sf::Vector2f         mLastSentMoveDir{0.f, 0.f};

    bool input(const sf::Event& event);
    void doAction(const Action& action);
    void sampleInputCommands();
    void flushCommand();
    void updateVisuals();
    void render();

    void handleEvents(const std::vector<GameEvent>& events);
    void playSound(const EventSound& sound);

    void send(const Command& command);
};

#endif
