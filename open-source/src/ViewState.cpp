#include "ViewState.hpp"

#include "Debug.hpp"
#include "GameStateDetail.hpp"
#include "GuiCombat.hpp"
#include "Profiler.hpp"
#include "ViewStateDetail.hpp"

#include <SFML/Graphics/CircleShape.hpp>

#include <SFML/System/Vector2.hpp>

#include <numbers>
#include <sstream>

#include <cmath>

#if defined(ANDROID) || defined(__ANDROID__) || defined(SFML_OPENGL_ES)
static const char* kFogPassthroughVertexShader =
    "#version 100\n"
    "attribute vec2 position;\n"
    "attribute vec4 color;\n"
    "attribute vec2 texCoord;\n"
    "varying vec4 sf_color;\n"
    "varying vec2 sf_texCoord;\n"
    "uniform mat4 sf_modelview;\n"
    "uniform mat4 sf_projection;\n"
    "uniform mat4 sf_texture;\n"
    "void main()\n"
    "{\n"
    "    sf_color = color;\n"
    "    vec4 coord = sf_texture * vec4(texCoord, 0.0, 1.0);\n"
    "    sf_texCoord = coord.xy;\n"
    "    gl_Position = sf_projection * sf_modelview * vec4(position, 0.0, 1.0);\n"
    "}\n";

static const char* kFogCanopyVertexShader =
    "#version 100\n"
    "precision highp float;\n"
    "attribute vec2 position;\n"
    "attribute vec4 color;\n"
    "attribute vec2 texCoord;\n"
    "varying vec4 sf_color;\n"
    "varying vec2 sf_texCoord;\n"
    "uniform mat4 sf_modelview;\n"
    "uniform mat4 sf_projection;\n"
    "uniform mat4 sf_texture;\n"
    "\n"
    "uniform vec2  u_camCenter;\n"
    "uniform float u_focalDist;\n"
    "uniform float u_sinPitch;\n"
    "uniform float u_cosPitch;\n"
    "uniform float u_heightScale;\n"
    "uniform float u_heightPersp;\n"
    "\n"
    "uniform vec4  u_bounds;\n"
    "uniform vec2  u_mapDim;\n"
    "uniform vec2  u_texSize;\n"
    "uniform float u_time;\n"
    "uniform float u_layerMode;\n"
    "uniform float u_baseHoverZ;\n"
    "uniform float u_wAmp;\n"
    "uniform float u_swayAmp;\n"
    "uniform float u_phase;\n"
    "uniform vec4  u_layerColor;\n"
    "\n"
    "vec2 project3D(vec3 worldPos)\n"
    "{\n"
    "    float dx = worldPos.x - u_camCenter.x;\n"
    "    float dy = worldPos.y - u_camCenter.y;\n"
    "    float denom = max(max(u_focalDist * 0.25, 1.0), u_focalDist - dy * u_sinPitch - worldPos.z * u_heightPersp);\n"
    "    float s = u_focalDist / denom;\n"
    "    return vec2(u_camCenter.x + dx * s, u_camCenter.y + (dy * u_cosPitch - worldPos.z * u_heightScale) * s);\n"
    "}\n"
    "\n"
    "void main()\n"
    "{\n"
    "    vec2 gridNorm = position;\n"
    "    vec2 wPos = mix(u_bounds.xy, u_bounds.zw, gridNorm);\n"
    "    vec3 world3D;\n"
    "    vec2 normUV;\n"
    "    vec2 safeMapDim = max(u_mapDim, vec2(1.0, 1.0));\n"
    "    if (u_layerMode < 0.5)\n"
    "    {\n"
    "        float sSwell = sin(wPos.x * 0.007 + wPos.y * 0.005 + u_time) * 2.5;\n"
    "        world3D = vec3(wPos.x + 15.0 + sSwell, wPos.y - 10.0 - sSwell * 0.6, 1.0);\n"
    "        normUV = clamp(wPos / safeMapDim, 0.0, 1.0);\n"
    "    }\n"
    "    else\n"
    "    {\n"
    "        float swell  = sin(wPos.x * 0.0075 + u_time * 1.05 + u_phase) * 3.5;\n"
    "        float wz     = u_baseHoverZ + swell * u_wAmp;\n"
    "        float swayX  = cos(wPos.x * 0.0068 + u_time * 0.80 + u_phase) * u_swayAmp;\n"
    "        world3D      = vec3(wPos.x + swayX, wPos.y, wz);\n"
    "        normUV       = clamp((wPos + vec2(swayX, 0.0) * 0.25) / safeMapDim, 0.0, 1.0);\n"
    "    }\n"
    "    vec2 projPos = project3D(world3D);\n"
    "    sf_color = u_layerColor;\n"
    "    vec2 pixelUV = normUV * u_texSize;\n"
    "    vec4 coord = sf_texture * vec4(pixelUV, 0.0, 1.0);\n"
    "    sf_texCoord = coord.xy;\n"
    "    gl_Position = sf_projection * sf_modelview * vec4(projPos, 0.0, 1.0);\n"
    "}\n";

static const char* kFogFilterFragShader =
    "#version 100\n"
    "#ifdef GL_FRAGMENT_PRECISION_HIGH\n"
    "precision highp float;\n"
    "#else\n"
    "precision mediump float;\n"
    "#endif\n"
    "varying vec4 sf_color;\n"
    "varying vec2 sf_texCoord;\n"
    "uniform sampler2D texture;\n"
    "uniform vec2 u_direction;\n"
    "uniform float u_pass;\n"
    "void main()\n"
    "{\n"
    "    vec2 uv = sf_texCoord;\n"
    "    vec4 sum = vec4(0.0);\n"
    "    float totalWeight = 0.0;\n"
    "    for (int i = -3; i <= 3; ++i)\n"
    "    {\n"
    "        float w = (i == 0) ? 0.30 : ((i == -1 || i == 1) ? 0.22 : ((i == -2 || i == 2) ? 0.10 : 0.03));\n"
    "        vec2 sampleUv = clamp(uv + float(i) * u_direction, 0.0, 1.0);\n"
    "        vec4 c = texture2D(texture, sampleUv);\n"
    "        float brightWeight = 1.0 + (1.0 - c.a) * 2.0;\n"
    "        float finalW = w * brightWeight;\n"
    "        sum += c * finalW;\n"
    "        totalWeight += finalW;\n"
    "    }\n"
    "    vec4 res = sum / max(totalWeight, 0.0001);\n"
    "    if (u_pass > 0.5)\n"
    "    {\n"
    "        res.a = pow(clamp(res.a, 0.0, 1.0), 1.25);\n"
    "    }\n"
    "    gl_FragColor = res;\n"
    "}\n";

static const char* kFogFragShader =
    "#version 100\n"
    "#ifdef GL_FRAGMENT_PRECISION_HIGH\n"
    "precision highp float;\n"
    "#else\n"
    "precision mediump float;\n"
    "#endif\n"
    "varying vec4 sf_color;\n"
    "varying vec2 sf_texCoord;\n"
    "uniform sampler2D texture;\n"
    "uniform vec2 u_texelSize;\n"
    "void main()\n"
    "{\n"
    "    vec2 uv = sf_texCoord;\n"
    "    vec2 off = u_texelSize * 0.4;\n"
    "    vec4 col = texture2D(texture, clamp(uv, 0.0, 1.0)) * 0.4;\n"
    "    col += texture2D(texture, clamp(uv + vec2(-off.x, -off.y), 0.0, 1.0)) * 0.15;\n"
    "    col += texture2D(texture, clamp(uv + vec2( off.x, -off.y), 0.0, 1.0)) * 0.15;\n"
    "    col += texture2D(texture, clamp(uv + vec2(-off.x,  off.y), 0.0, 1.0)) * 0.15;\n"
    "    col += texture2D(texture, clamp(uv + vec2( off.x,  off.y), 0.0, 1.0)) * 0.15;\n"
    "    gl_FragColor = col * sf_color;\n"
    "}\n";
#else
static const char* kFogPassthroughVertexShader =
    "#version 120\n"
    "void main()\n"
    "{\n"
    "    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;\n"
    "    gl_TexCoord[0] = gl_TextureMatrix[0] * gl_MultiTexCoord0;\n"
    "    gl_FrontColor = gl_Color;\n"
    "}\n";

static const char* kFogCanopyVertexShader =
    "#version 120\n"
    "uniform vec2  u_camCenter;\n"
    "uniform float u_focalDist;\n"
    "uniform float u_sinPitch;\n"
    "uniform float u_cosPitch;\n"
    "uniform float u_heightScale;\n"
    "uniform float u_heightPersp;\n"
    "\n"
    "uniform vec4  u_bounds;\n"
    "uniform vec2  u_mapDim;\n"
    "uniform vec2  u_texSize;\n"
    "uniform float u_time;\n"
    "uniform float u_layerMode;\n"
    "uniform float u_baseHoverZ;\n"
    "uniform float u_wAmp;\n"
    "uniform float u_swayAmp;\n"
    "uniform float u_phase;\n"
    "uniform vec4  u_layerColor;\n"
    "\n"
    "vec2 project3D(vec3 worldPos)\n"
    "{\n"
    "    float dx = worldPos.x - u_camCenter.x;\n"
    "    float dy = worldPos.y - u_camCenter.y;\n"
    "    float denom = max(max(u_focalDist * 0.25, 1.0), u_focalDist - dy * u_sinPitch - worldPos.z * u_heightPersp);\n"
    "    float s = u_focalDist / denom;\n"
    "    return vec2(u_camCenter.x + dx * s, u_camCenter.y + (dy * u_cosPitch - worldPos.z * u_heightScale) * s);\n"
    "}\n"
    "\n"
    "void main()\n"
    "{\n"
    "    vec2 gridNorm = gl_Vertex.xy;\n"
    "    vec2 wPos = mix(u_bounds.xy, u_bounds.zw, gridNorm);\n"
    "    vec3 world3D;\n"
    "    vec2 normUV;\n"
    "    vec2 safeMapDim = max(u_mapDim, vec2(1.0, 1.0));\n"
    "    if (u_layerMode < 0.5)\n"
    "    {\n"
    "        float sSwell = sin(wPos.x * 0.007 + wPos.y * 0.005 + u_time) * 2.5;\n"
    "        world3D = vec3(wPos.x + 15.0 + sSwell, wPos.y - 10.0 - sSwell * 0.6, 1.0);\n"
    "        normUV = clamp(wPos / safeMapDim, 0.0, 1.0);\n"
    "    }\n"
    "    else\n"
    "    {\n"
    "        float swell  = sin(wPos.x * 0.0075 + u_time * 1.05 + u_phase) * 3.5;\n"
    "        float wz     = u_baseHoverZ + swell * u_wAmp;\n"
    "        float swayX  = cos(wPos.x * 0.0068 + u_time * 0.80 + u_phase) * u_swayAmp;\n"
    "        world3D      = vec3(wPos.x + swayX, wPos.y, wz);\n"
    "        normUV       = clamp((wPos + vec2(swayX, 0.0) * 0.25) / safeMapDim, 0.0, 1.0);\n"
    "    }\n"
    "    vec2 projPos = project3D(world3D);\n"
    "    gl_FrontColor = u_layerColor;\n"
    "    vec2 pixelUV = normUV * u_texSize;\n"
    "    gl_TexCoord[0] = gl_TextureMatrix[0] * vec4(pixelUV, 0.0, 1.0);\n"
    "    gl_Position = gl_ModelViewProjectionMatrix * vec4(projPos, 0.0, 1.0);\n"
    "}\n";

static const char* kFogFilterFragShader =
    "#version 120\n"
    "uniform sampler2D texture;\n"
    "uniform vec2 u_direction;\n"
    "uniform float u_pass;\n"
    "void main()\n"
    "{\n"
    "    vec2 uv = gl_TexCoord[0].xy;\n"
    "    vec4 sum = vec4(0.0);\n"
    "    float totalWeight = 0.0;\n"
    "    for (int i = -3; i <= 3; ++i)\n"
    "    {\n"
    "        float w = (i == 0) ? 0.30 : ((i == -1 || i == 1) ? 0.22 : ((i == -2 || i == 2) ? 0.10 : 0.03));\n"
    "        vec4 c = texture2D(texture, uv + float(i) * u_direction);\n"
    "        float brightWeight = 1.0 + (1.0 - c.a) * 2.0;\n"
    "        float finalW = w * brightWeight;\n"
    "        sum += c * finalW;\n"
    "        totalWeight += finalW;\n"
    "    }\n"
    "    vec4 res = sum / max(totalWeight, 0.0001);\n"
    "    if (u_pass > 0.5)\n"
    "    {\n"
    "        res.a = pow(clamp(res.a, 0.0, 1.0), 1.25);\n"
    "    }\n"
    "    gl_FragColor = res;\n"
    "}\n";

static const char* kFogFragShader =
    "#version 120\n"
    "uniform sampler2D texture;\n"
    "uniform vec2 u_texelSize;\n"
    "void main()\n"
    "{\n"
    "    vec2 uv = gl_TexCoord[0].xy;\n"
    "    vec2 off = u_texelSize * 0.4;\n"
    "    vec4 col = texture2D(texture, uv) * 0.4;\n"
    "    col += texture2D(texture, uv + vec2(-off.x, -off.y)) * 0.15;\n"
    "    col += texture2D(texture, uv + vec2( off.x, -off.y)) * 0.15;\n"
    "    col += texture2D(texture, uv + vec2(-off.x,  off.y)) * 0.15;\n"
    "    col += texture2D(texture, uv + vec2( off.x,  off.y)) * 0.15;\n"
    "    gl_FragColor = col * gl_Color;\n"
    "}\n";
#endif

std::unique_ptr<sf::Shader> ViewState::sFogFilterShader;
bool                        ViewState::sFogFilterShaderLoaded = false;
std::unique_ptr<sf::Shader> ViewState::sFogShader;
bool                        ViewState::sFogShaderLoaded = false;

void ViewState::initFogShader()
{
    PROFILE_FUNCTION();
    static bool sInitAttempted = false;
    if (sInitAttempted)
    {
        return;
    }
    sInitAttempted = true;

    if (!sf::Shader::isAvailable())
    {
        logout("[Shader] sf::Shader::isAvailable() is false - Shaders not supported on device.", true);
        return;
    }

    logout("[Shader] sf::Shader::isAvailable() is true - Shaders supported on device.");

    std::stringstream errStream;
    std::streambuf*   oldErrBuf = sf::err().rdbuf(errStream.rdbuf());

    if (!sFogShaderLoaded)
    {
        if (!sFogShader)
        {
            sFogShader = std::make_unique<sf::Shader>();
        }
        errStream.str("");
        errStream.clear();
        sFogShaderLoaded = sFogShader->loadFromMemory(kFogCanopyVertexShader, kFogFragShader);
        if (!sFogShaderLoaded)
        {
            std::string logMsg     = "[Shader] Failed to load kFogFragShader!";
            std::string errDetails = errStream.str();
            if (!errDetails.empty())
            {
                logMsg += "\n[Shader Log]: " + errDetails;
            }
            logout(logMsg, true);
        }
        else
        {
            logout("[Shader] kFogFragShader loaded successfully.");
        }
    }

    if (!sFogFilterShaderLoaded)
    {
        if (!sFogFilterShader)
        {
            sFogFilterShader = std::make_unique<sf::Shader>();
        }
        errStream.str("");
        errStream.clear();
        sFogFilterShaderLoaded = sFogFilterShader->loadFromMemory(kFogPassthroughVertexShader, kFogFilterFragShader);
        if (!sFogFilterShaderLoaded)
        {
            std::string logMsg     = "[Shader] Failed to load kFogFilterFragShader (Gaussian 7x7)!";
            std::string errDetails = errStream.str();
            if (!errDetails.empty())
            {
                logMsg += "\n[Shader Log]: " + errDetails;
            }
            logout(logMsg, true);
        }
        else
        {
            logout("[Shader] kFogFilterFragShader loaded successfully.");
        }
    }

    sf::err().rdbuf(oldErrBuf);
}

void ViewState::drawFogCanopy(sf::RenderTarget&                target,
                              const ViewStateDetail::Camera3D& cam,
                              const sf::FloatRect&             frustum,
                              sf::Vector2f                     mapDim,
                              float                            time)
{
    PROFILE_FUNCTION();
    if (!sFogShaderLoaded)
    {
        initFogShader();
    }

    const sf::Vector2u texSize = mFogTexture.getSize();
    if (texSize.x == 0 || texSize.y == 0)
    {
        return;
    }

    const float xStart = std::clamp(frustum.position.x, 0.f, mapDim.x);
    const float yStart = std::clamp(frustum.position.y, 0.f, mapDim.y);
    const float xEnd   = std::clamp(frustum.position.x + frustum.size.x, 0.f, mapDim.x);
    const float yEnd   = std::clamp(frustum.position.y + frustum.size.y, 0.f, mapDim.y);

    if (xEnd <= xStart || yEnd <= yStart)
    {
        return;
    }

    constexpr int kCols = 28;
    constexpr int kRows = 20;

    static sf::VertexBuffer sGridVbo(sf::PrimitiveType::Triangles, sf::VertexBuffer::Usage::Static);
    static sf::VertexArray  sGridVa(sf::PrimitiveType::Triangles);
    static bool             sGridInitialized = false;

    if (!sGridInitialized)
    {
        sGridInitialized = true;
        std::vector<sf::Vertex> vertices;
        vertices.reserve(kRows * kCols * 6);
        for (int r = 0; r < kRows; ++r)
        {
            const float y0 = static_cast<float>(r) / static_cast<float>(kRows);
            const float y1 = static_cast<float>(r + 1) / static_cast<float>(kRows);
            for (int c = 0; c < kCols; ++c)
            {
                const float x0 = static_cast<float>(c) / static_cast<float>(kCols);
                const float x1 = static_cast<float>(c + 1) / static_cast<float>(kCols);

                vertices.push_back(sf::Vertex{.position = {x0, y0}});
                vertices.push_back(sf::Vertex{.position = {x1, y0}});
                vertices.push_back(sf::Vertex{.position = {x1, y1}});

                vertices.push_back(sf::Vertex{.position = {x1, y1}});
                vertices.push_back(sf::Vertex{.position = {x0, y1}});
                vertices.push_back(sf::Vertex{.position = {x0, y0}});
            }
        }

        sGridVa.resize(vertices.size());
        for (size_t i = 0; i < vertices.size(); ++i)
        {
            sGridVa[i] = vertices[i];
        }

        if (sf::VertexBuffer::isAvailable())
        {
            (void)sGridVbo.create(vertices.size());
            (void)sGridVbo.update(vertices.data());
        }
    }

    if (sFogShaderLoaded && sFogShader)
    {
        sf::RenderStates states;
        states.texture = &mFogTexture.getTexture();
        states.shader  = sFogShader.get();

        sFogShader->setUniform("texture", sf::Shader::CurrentTexture);
        sFogShader->setUniform("u_texelSize",
                               sf::Glsl::Vec2(1.f / static_cast<float>(texSize.x), 1.f / static_cast<float>(texSize.y)));

        sFogShader->setUniform("u_camCenter", sf::Glsl::Vec2(cam.center.x, cam.center.y));
        sFogShader->setUniform("u_focalDist", std::max(1.f, cam.focalDist));
        sFogShader->setUniform("u_sinPitch", cam.sinPitch);
        sFogShader->setUniform("u_cosPitch", cam.cosPitch);
        sFogShader->setUniform("u_heightScale", cam.heightScale);
        sFogShader->setUniform("u_heightPersp", cam.heightPersp);

        sFogShader->setUniform("u_bounds", sf::Glsl::Vec4(xStart, yStart, xEnd, yEnd));
        sFogShader->setUniform("u_mapDim", sf::Glsl::Vec2(std::max(1.f, mapDim.x), std::max(1.f, mapDim.y)));
        sFogShader->setUniform("u_texSize", sf::Glsl::Vec2(static_cast<float>(texSize.x), static_cast<float>(texSize.y)));
        sFogShader->setUniform("u_time", time);

        auto drawGrid = [&](const sf::RenderStates& st) { target.draw(sGridVa, st); };

        sFogShader->setUniform("u_layerMode", 1.0f);
        sFogShader->setUniform("u_baseHoverZ", 23.f);
        sFogShader->setUniform("u_wAmp", 0.55f);
        sFogShader->setUniform("u_swayAmp", 2.2f);
        sFogShader->setUniform("u_phase", 0.0f);
        sFogShader->setUniform("u_layerColor", sf::Glsl::Vec4(1.f, 1.f, 1.f, 216.f / 255.f));
        drawGrid(states);
    }
}

void ViewState::filterFogTexture()
{
    PROFILE_FUNCTION();
    if (!sFogFilterShaderLoaded)
    {
        initFogShader();
    }

    sf::Vector2u size = mRawFogTexture.getSize();
    if (size.x == 0 || size.y == 0)
        return;

    const sf::View filterView(sf::FloatRect({0.f, 0.f}, {static_cast<float>(size.x), static_cast<float>(size.y)}));

    if (!sFogFilterShaderLoaded || !sFogFilterShader)
    {
        sf::Sprite rawSprite(mRawFogTexture);
        mFogTexture.setView(filterView);
        mFogTexture.clear(sf::Color(0, 0, 0, 0));
        mFogTexture.draw(rawSprite);
        mFogTexture.display();
        return;
    }

    const float invW = 1.f / static_cast<float>(size.x);
    const float invH = 1.f / static_cast<float>(size.y);

    const sf::Vertex dummy[2] = {sf::Vertex{.position = {-1000.f, -1000.f}}, sf::Vertex{.position = {-1000.f, -1000.f}}};

    {
        PROFILE_SCOPE("Fog of War - Blur H (GPU)");
        mTempFogTexture.setView(filterView);
        mTempFogTexture.draw(dummy, 2, sf::PrimitiveType::Lines, sf::RenderStates::Default);
        mTempFogTexture.clear(sf::Color(0, 0, 0, 0));

        sf::Sprite rawSprite(mRawFogTexture);
        sFogFilterShader->setUniform("texture", sf::Shader::CurrentTexture);
        sFogFilterShader->setUniform("u_direction", sf::Glsl::Vec2(invW, 0.f));
        sFogFilterShader->setUniform("u_pass", 0.f);

        mTempFogTexture.draw(rawSprite, sFogFilterShader.get());
        mTempFogTexture.display();
    }

    {
        PROFILE_SCOPE("Fog of War - Blur V (GPU)");
        mFogTexture.setView(filterView);
        mFogTexture.draw(dummy, 2, sf::PrimitiveType::Lines, sf::RenderStates::Default);
        mFogTexture.clear(sf::Color(0, 0, 0, 0));

        sf::Sprite tempSprite(mTempFogTexture.getTexture());
        sFogFilterShader->setUniform("texture", sf::Shader::CurrentTexture);
        sFogFilterShader->setUniform("u_direction", sf::Glsl::Vec2(0.f, invH));
        sFogFilterShader->setUniform("u_pass", 1.f);

        mFogTexture.draw(tempSprite, sFogFilterShader.get());
        mFogTexture.display();
    }
}

void ViewState::drawFogMeshWithShader(sf::RenderTarget& target, const sf::VertexArray& mesh)
{
    PROFILE_FUNCTION();
    if (!sFogShaderLoaded)
    {
        initFogShader();
    }

    sf::RenderStates states;
    states.texture = &mFogTexture.getTexture();

    if (sFogShaderLoaded && sFogShader)
    {
        sf::Vector2u texSize = mFogTexture.getSize();
        if (texSize.x > 0 && texSize.y > 0)
        {
            sFogShader->setUniform("texture", sf::Shader::CurrentTexture);
            sFogShader->setUniform("u_texelSize",
                                   sf::Glsl::Vec2(1.f / static_cast<float>(texSize.x), 1.f / static_cast<float>(texSize.y)));
            states.shader = sFogShader.get();
        }
    }
    target.draw(mesh, states);
}

ViewState::ViewState(ClientGame* clientGame) : mClientGame(clientGame), mSystems(this)
{
    initFogShader();
}

int ViewState::getPlayerClientId(int entityId) const
{
    const auto&      pool = mClientGame->gameState().admin().mPool;
    std::vector<int> playerEntities;
    for (int pid : pool.viewIndex<CPlayer>())
    {
        playerEntities.push_back(pid);
    }
    std::ranges::sort(playerEntities);

    for (size_t i = 0; i < playerEntities.size(); ++i)
    {
        if (playerEntities[i] == entityId)
        {
            return static_cast<int>(i);
        }
    }
    return 0;
}

int ViewState::localTeamId() const
{
    const auto& gs   = mClientGame->gameState();
    const auto& pool = gs.admin().mPool;
    const int   team = GameStateDetail::getEntityTeam(pool, mPlayerEntityId);
    if (team >= 0)
        return team;
    if (gs.getGameMode() == GameMode::PvP && mClientGame && mClientGame->mId >= 0)
        return mClientGame->mId;
    return 0;
}

bool ViewState::isOpponent(int entityId) const
{
    const auto& pool       = mClientGame->gameState().admin().mPool;
    const int   entityTeam = GameStateDetail::getEntityTeam(pool, entityId);
    if (entityTeam < 0)
        return false;
    return entityTeam != localTeamId();
}

bool ViewState::isFriendly(int entityId) const
{
    const auto& pool       = mClientGame->gameState().admin().mPool;
    const int   entityTeam = GameStateDetail::getEntityTeam(pool, entityId);
    if (entityTeam < 0)
        return false;
    return entityTeam == localTeamId();
}

bool ViewState::isLocalVictory() const
{
    const auto& gs = mClientGame->gameState();
    if (gs.getGameMode() == GameMode::PvP)
    {
        return gs.winningTeam() == localTeamId();
    }
    return gs.campaign().victory;
}

sf::Vector2f ViewState::screenToWorldGround(sf::Vector2i pixelPos)
{
    mCamera3D.setup(worldView());
    const sf::Vector2f viewPos = window().mapPixelToCoords(pixelPos, worldView());
    return mCamera3D.unprojectToGround(viewPos);
}

void ViewState::send(const Command& command)
{
    if (command.type() != CommandType::NoOp)
    {
        mPendingCommands.push_back(command);
    }
}

void ViewState::flushCommand()
{
    sampleInputCommands();

    uint64_t currentSimulatedTick = mClientGame->gameState().mTick > 0 ? (mClientGame->gameState().mTick - 1) : 0;
    size_t   targetTick           = currentSimulatedTick + GameConfig::Game::kCommandDelayTicks;

    CommandOnTick packet;
    packet.tick = targetTick;
    for (auto& cmd : mPendingCommands)
    {
        cmd.setTick(targetTick);
    }
    packet.commands = std::move(mPendingCommands);
    mPendingCommands.clear();

    if (mClientGame->gameState().mPlayerCount == 1 || (mClientGame->mGame && !mClientGame->mGame->isConnected()))
    {
        mClientGame->pushCommand(std::move(packet));
        return;
    }

    mClientGame->queueOutgoingCommand(std::move(packet));
}

void ViewState::sampleInputCommands()
{
    const auto& gameState = mClientGame->gameState();
    const auto& admin     = gameState.admin();

    const auto* playerMask = admin.mPool.getMask(mPlayerEntityId);
    if (mPlayerEntityId >= 0 && admin.mPool.hasComponent<CPlayer>(playerMask))
    {
        const auto player = admin.mPool.getComponent<CPlayer>(mPlayerEntityId);
        if (player->respawnTimer > 0)
        {
            mLastSentMoveDir = {0.f, 0.f};
            return;
        }
    }

    sf::Vector2f moveDir = {0.f, 0.f};
    if (input().isKeyPressed(sf::Keyboard::Key::W) || input().isKeyPressed(sf::Keyboard::Key::Up))
        moveDir.y -= 1.f;
    if (input().isKeyPressed(sf::Keyboard::Key::S) || input().isKeyPressed(sf::Keyboard::Key::Down))
        moveDir.y += 1.f;
    if (input().isKeyPressed(sf::Keyboard::Key::A) || input().isKeyPressed(sf::Keyboard::Key::Left))
        moveDir.x -= 1.f;
    if (input().isKeyPressed(sf::Keyboard::Key::D) || input().isKeyPressed(sf::Keyboard::Key::Right))
        moveDir.x += 1.f;

    if (std::hypot(input().joystickDir.x, input().joystickDir.y) > 0.05f)
    {
        moveDir = input().joystickDir;
    }

    if (moveDir != mLastSentMoveDir)
    {
        mLastSentMoveDir = moveDir;
        send(Command(CommandType::MovePlayer,
                     CmdMovePlayer{.playerId = mPlayerEntityId, .dirX = moveDir.x, .dirY = moveDir.y}));
    }
}

void ViewState::updateVisuals()
{
    PROFILE_FUNCTION();
    mCurrentFrame = mClientGame ? mClientGame->mCurrentFrame : (mCurrentFrame + 1);

    mSystems.get<SystemViewState::SystemCamera>().update();

    input().mouseWorldPos = screenToWorldGround(input().mousePixelPos);

    mSystems.get<SystemViewState::SystemDragDrop>().update();
    mSystems.get<SystemViewState::SystemSelection>().update();
    mSystems.get<SystemViewState::SystemAnimation>().update();

    input().clearJustPressedReleased();
}

bool ViewState::input(const sf::Event& event)
{
    if (const auto* keyPressed = event.getIf<sf::Event::KeyPressed>())
    {
        Action action(Action::Type::EventKeyPressed, KeyPressedData{keyPressed->code});
        mClientGame->pushAction(action);
    }
    else if (const auto* keyReleased = event.getIf<sf::Event::KeyReleased>())
    {
        Action action(Action::Type::EventKeyReleased, KeyReleasedData{keyReleased->code});
        mClientGame->pushAction(action);
    }
    else if (const auto* touchBegan = event.getIf<sf::Event::TouchBegan>())
    {
        Action action(Action::Type::EventTouchBegan,
                      TouchBeganData{.finger   = static_cast<int>(touchBegan->finger),
                                     .position = {touchBegan->position.x, touchBegan->position.y}});
        mClientGame->pushAction(action);
    }
    else if (const auto* touchEnded = event.getIf<sf::Event::TouchEnded>())
    {
        Action action(Action::Type::EventTouchEnded,
                      TouchEndedData{.finger   = static_cast<int>(touchEnded->finger),
                                     .position = {touchEnded->position.x, touchEnded->position.y}});
        mClientGame->pushAction(action);
    }
    else if (const auto* touch = event.getIf<sf::Event::TouchMoved>())
    {
        Action action(Action::Type::EventTouchMoved,
                      TouchMovedData{.finger   = static_cast<int>(touch->finger),
                                     .position = {touch->position.x, touch->position.y}});
        mClientGame->pushAction(action);
    }
    else if (const auto* mouseButton = event.getIf<sf::Event::MouseButtonPressed>())
    {
        Action action(Action::Type::EventMouseButtonPressed,
                      MouseButtonPressedData{.button   = mouseButton->button,
                                             .position = {mouseButton->position.x, mouseButton->position.y}});
        mClientGame->pushAction(action);
    }
    else if (const auto* mouseReleased = event.getIf<sf::Event::MouseButtonReleased>())
    {
        Action action(Action::Type::EventMouseButtonReleased,
                      MouseButtonReleasedData{.button   = mouseReleased->button,
                                              .position = {mouseReleased->position.x, mouseReleased->position.y}});
        mClientGame->pushAction(action);
    }
    else if (const auto* mouseMoved = event.getIf<sf::Event::MouseMoved>())
    {
        Action action(Action::Type::EventMouseMoved, MouseMovedData{{mouseMoved->position.x, mouseMoved->position.y}});
        mClientGame->pushAction(action);
    }
    else if (const auto* wheelScrolled = event.getIf<sf::Event::MouseWheelScrolled>())
    {
        Action action(Action::Type::EventMouseWheelScrolled, MouseWheelScrolledData{wheelScrolled->delta});
        mClientGame->pushAction(action);
    }

    return false;
}

void ViewState::render()
{
    PROFILE_FUNCTION();
    if (mClientGame)
    {
        mCurrentFrame = mClientGame->mCurrentFrame;
    }

    const auto& gameState = mClientGame->gameState();

    const auto& mAdmin = gameState.admin();

    window().setView(worldView());
    mCamera3D.setup(worldView());
    const auto& cam = mCamera3D;

    const sf::FloatRect cameraBounds = cam.getGroundFrustumBounds(160.f);
    const float         camLeft      = cameraBounds.position.x;
    const float         camTop       = cameraBounds.position.y;
    const float         camRight     = camLeft + cameraBounds.size.x;
    const float         camBottom    = camTop + cameraBounds.size.y;

    static const auto kUnitPolygon4 = []()
    {
        std::array<sf::Vector2f, 4> pts;
        for (int p = 0; p < 4; ++p)
        {
            const float a = (p + 0.5f) * 2.f * std::numbers::pi_v<float> / 4.f;
            pts[p]        = sf::Vector2f(std::cos(a), std::sin(a));
        }
        return pts;
    }();

    static const auto kUnitPolygon6 = []()
    {
        std::array<sf::Vector2f, 6> pts;
        for (int p = 0; p < 6; ++p)
        {
            const float a = p * 2.f * std::numbers::pi_v<float> / 6.f;
            pts[p]        = sf::Vector2f(std::cos(a), std::sin(a));
        }
        return pts;
    }();

    static const auto kUnitPolygon8 = []()
    {
        std::array<sf::Vector2f, 8> pts;
        for (int p = 0; p < 8; ++p)
        {
            const float a = p * 2.f * std::numbers::pi_v<float> / 8.f;
            pts[p]        = sf::Vector2f(std::cos(a), std::sin(a));
        }
        return pts;
    }();

    static const auto kUnitPolygon10 = []()
    {
        std::array<sf::Vector2f, 10> pts;
        for (int p = 0; p < 10; ++p)
        {
            const float a = p * 2.f * std::numbers::pi_v<float> / 10.f;
            pts[p]        = sf::Vector2f(std::cos(a), std::sin(a));
        }
        return pts;
    }();

    static const auto kUnitPolygon12 = []()
    {
        std::array<sf::Vector2f, 12> pts;
        for (int p = 0; p < 12; ++p)
        {
            const float a = p * 2.f * std::numbers::pi_v<float> / 12.f;
            pts[p]        = sf::Vector2f(std::cos(a), std::sin(a));
        }
        return pts;
    }();

    static const auto kUnitPolygon16 = []()
    {
        std::array<sf::Vector2f, 16> pts;
        for (int p = 0; p < 16; ++p)
        {
            const float a = p * 2.f * std::numbers::pi_v<float> / 16.f;
            pts[p]        = sf::Vector2f(std::cos(a), std::sin(a));
        }
        return pts;
    }();

    auto appendQuad2D =
        [](sf::VertexArray& va,
           sf::Vector2f     p0,
           sf::Vector2f     p1,
           sf::Vector2f     p2,
           sf::Vector2f     p3,
           sf::Color        cBottom,
           sf::Color        cTop,
           bool             cullBackface = false)
    {
        if (cullBackface)
        {
            const float cross = (p1.x - p0.x) * (p2.y - p0.y) - (p1.y - p0.y) * (p2.x - p0.x);
            if (cross >= 0.f)
            {
                return;
            }
        }
        va.append(sf::Vertex(p0, cBottom));
        va.append(sf::Vertex(p1, cBottom));
        va.append(sf::Vertex(p2, cTop));
        va.append(sf::Vertex(p2, cTop));
        va.append(sf::Vertex(p3, cTop));
        va.append(sf::Vertex(p0, cBottom));
    };

    auto addBox3D =
        [&](sf::VertexArray& va,
            sf::VertexArray* edgeLines,
            float            x,
            float            y,
            float            zBottom,
            float            w,
            float            h,
            float            zHeight,
            sf::Color        baseColor,
            sf::Color        edgeColor = sf::Color(0, 0, 0, 0))
    {
        const float zTop = zBottom + zHeight;

        const sf::Vector2f bNW = cam.project(x, y, zBottom);
        const sf::Vector2f bNE = cam.project(x + w, y, zBottom);
        const sf::Vector2f bSE = cam.project(x + w, y + h, zBottom);
        const sf::Vector2f bSW = cam.project(x, y + h, zBottom);

        const sf::Vector2f tNW = cam.project(x, y, zTop);
        const sf::Vector2f tNE = cam.project(x + w, y, zTop);
        const sf::Vector2f tSE = cam.project(x + w, y + h, zTop);
        const sf::Vector2f tSW = cam.project(x, y + h, zTop);

        const sf::Color cNorth = ViewStateDetail::computeLitColor(baseColor, 0.f, -1.f, 0.f);
        appendQuad2D(va, bNE, bNW, tNW, tNE, ViewStateDetail::shadeColor(cNorth, 0.72f), cNorth, true);

        const sf::Color cWest = ViewStateDetail::computeLitColor(baseColor, -1.f, 0.f, 0.f);
        appendQuad2D(va, bNW, bSW, tSW, tNW, ViewStateDetail::shadeColor(cWest, 0.74f), cWest, true);

        const sf::Color cEast = ViewStateDetail::computeLitColor(baseColor, 1.f, 0.f, 0.f);
        appendQuad2D(va, bSE, bNE, tNE, tSE, ViewStateDetail::shadeColor(cEast, 0.74f), cEast, true);

        const sf::Color cSouth = ViewStateDetail::computeLitColor(baseColor, 0.f, 1.f, 0.f);
        appendQuad2D(va, bSW, bSE, tSE, tSW, ViewStateDetail::shadeColor(cSouth, 0.76f), cSouth, true);

        const sf::Color cRoof = ViewStateDetail::computeLitColor(baseColor, 0.f, 0.f, 1.f);
        appendQuad2D(va, tSW, tSE, tNE, tNW, cRoof, ViewStateDetail::shadeColor(cRoof, 1.05f, 0.04f), false);

        if (edgeLines && edgeColor.a > 0)
        {
            auto addEdge = [&](sf::Vector2f a, sf::Vector2f b)
            {
                edgeLines->append(sf::Vertex(a, edgeColor));
                edgeLines->append(sf::Vertex(b, edgeColor));
            };
            addEdge(tNW, tNE);
            addEdge(tNE, tSE);
            addEdge(tSE, tSW);
            addEdge(tSW, tNW);
            addEdge(bSW, tSW);
            addEdge(bSE, tSE);
        }
    };

    auto addOrientedBox3D =
        [&](sf::VertexArray& va,
            sf::Vector2f     centerGround,
            sf::Vector2f     dir,
            float            halfLen,
            float            halfWidth,
            float            zBottom,
            float            zTop,
            sf::Color        baseColor)
    {
        const sf::Vector2f perp(-dir.y, dir.x);
        const sf::Vector2f g0 = centerGround - dir * halfLen - perp * halfWidth;
        const sf::Vector2f g1 = centerGround + dir * halfLen - perp * halfWidth;
        const sf::Vector2f g2 = centerGround + dir * halfLen + perp * halfWidth;
        const sf::Vector2f g3 = centerGround - dir * halfLen + perp * halfWidth;

        const sf::Vector2f b0 = cam.project(g0, zBottom);
        const sf::Vector2f b1 = cam.project(g1, zBottom);
        const sf::Vector2f b2 = cam.project(g2, zBottom);
        const sf::Vector2f b3 = cam.project(g3, zBottom);

        const sf::Vector2f t0 = cam.project(g0, zTop);
        const sf::Vector2f t1 = cam.project(g1, zTop);
        const sf::Vector2f t2 = cam.project(g2, zTop);
        const sf::Vector2f t3 = cam.project(g3, zTop);

        const sf::Color c01 = ViewStateDetail::computeLitColor(baseColor, -perp.x, -perp.y, 0.f);
        appendQuad2D(va, b1, b0, t0, t1, ViewStateDetail::shadeColor(c01, 0.75f), c01, true);

        const sf::Color c12 = ViewStateDetail::computeLitColor(baseColor, dir.x, dir.y, 0.f);
        appendQuad2D(va, b2, b1, t1, t2, ViewStateDetail::shadeColor(c12, 0.75f), c12, true);

        const sf::Color c23 = ViewStateDetail::computeLitColor(baseColor, perp.x, perp.y, 0.f);
        appendQuad2D(va, b3, b2, t2, t3, ViewStateDetail::shadeColor(c23, 0.75f), c23, true);

        const sf::Color c30 = ViewStateDetail::computeLitColor(baseColor, -dir.x, -dir.y, 0.f);
        appendQuad2D(va, b0, b3, t3, t0, ViewStateDetail::shadeColor(c30, 0.75f), c30, true);

        const sf::Color cTop = ViewStateDetail::computeLitColor(baseColor, 0.f, 0.f, 1.f);
        appendQuad2D(va, t3, t2, t1, t0, cTop, ViewStateDetail::shadeColor(cTop, 1.06f, 0.04f), false);
    };

    auto addPrismLUT3D =
        [&](sf::VertexArray& va,
            sf::Vector2f     center,
            float            rBottom,
            float            rTop,
            float            zBottom,
            float            zTop,
            const auto&      lut,
            sf::Color        baseColor,
            bool             drawCap = true)
    {
        constexpr int               N = static_cast<int>(std::tuple_size_v<std::decay_t<decltype(lut)>>);
        std::array<sf::Vector2f, N> bPts;
        std::array<sf::Vector2f, N> tPts;
        for (int i = 0; i < N; ++i)
        {
            bPts[i] = cam.project(center.x + lut[i].x * rBottom, center.y + lut[i].y * rBottom, zBottom);
            tPts[i] = cam.project(center.x + lut[i].x * rTop, center.y + lut[i].y * rTop, zTop);
        }

        for (int i = 0; i < N; ++i)
        {
            const int   next = (i + 1) % N;
            float       nx   = (lut[i].x + lut[next].x) * 0.5f;
            float       ny   = (lut[i].y + lut[next].y) * 0.5f;
            const float nLen = std::hypot(nx, ny);
            if (nLen > 0.001f)
            {
                nx /= nLen;
                ny /= nLen;
            }
            const sf::Color faceCol = ViewStateDetail::computeLitColor(baseColor, nx, ny, 0.15f);
            appendQuad2D(va, bPts[next], bPts[i], tPts[i], tPts[next], ViewStateDetail::shadeColor(faceCol, 0.74f), faceCol, true);
        }

        if (drawCap && rTop > 0.1f)
        {
            const sf::Vector2f tCenter   = cam.project(center.x, center.y, zTop);
            const sf::Color    capOuter  = ViewStateDetail::computeLitColor(baseColor, 0.f, 0.f, 1.f);
            const sf::Color    capCenter = ViewStateDetail::shadeColor(capOuter, 1.12f, 0.06f);
            for (int i = 0; i < N; ++i)
            {
                const int next = (i + 1) % N;
                va.append(sf::Vertex(tCenter, capCenter));
                va.append(sf::Vertex(tPts[i], capOuter));
                va.append(sf::Vertex(tPts[next], capOuter));
            }
        }
    };

    auto addOctahedron3D =
        [&](sf::VertexArray& va, sf::Vector2f center, float radius, float zBottom, float zMid, float zTop, float rotRad, sf::Color baseColor)
    {
        const sf::Vector2f pBot = cam.project(center.x, center.y, zBottom);
        const sf::Vector2f pTop = cam.project(center.x, center.y, zTop);

        std::array<sf::Vector2f, 4> mPts;
        std::array<sf::Vector2f, 4> dirs;
        for (int i = 0; i < 4; ++i)
        {
            const float a = rotRad + i * 0.5f * std::numbers::pi_v<float>;
            dirs[i]       = {std::cos(a), std::sin(a)};
            mPts[i]       = cam.project(center.x + dirs[i].x * radius, center.y + dirs[i].y * radius, zMid);
        }

        for (int i = 0; i < 4; ++i)
        {
            const int   next = (i + 1) % 4;
            const float nx   = (dirs[i].x + dirs[next].x) * 0.7071f;
            const float ny   = (dirs[i].y + dirs[next].y) * 0.7071f;

            const sf::Vector2f& v0    = mPts[next];
            const sf::Vector2f& v1    = mPts[i];
            const float         cross = (v1.x - v0.x) * (pTop.y - v0.y) - (v1.y - v0.y) * (pTop.x - v0.x);
            if (cross < 0.f)
            {
                const sf::Color cUpper = ViewStateDetail::computeLitColor(baseColor, nx, ny, 0.55f);
                const sf::Color cApex  = ViewStateDetail::shadeColor(cUpper, 1.15f, 0.12f);
                va.append(sf::Vertex(v0, cUpper));
                va.append(sf::Vertex(v1, cUpper));
                va.append(sf::Vertex(pTop, cApex));

                const sf::Color cLower = ViewStateDetail::computeLitColor(baseColor, nx, ny, -0.45f);
                va.append(sf::Vertex(v1, cLower));
                va.append(sf::Vertex(v0, cLower));
                va.append(sf::Vertex(pBot, ViewStateDetail::shadeColor(cLower, 0.72f)));
            }
        }
    };

    auto addGroundRing3D =
        [&](sf::VertexArray& va,
            sf::Vector2f     center,
            float            worldZ,
            float            radius,
            float            thickness,
            sf::Color        color,
            int              segments = 24,
            float            rotDeg   = 0.f)
    {
        if (thickness <= 0.f || radius <= 0.f || segments < 3)
            return;
        const float rInner = std::max(0.f, radius - thickness * 0.5f);
        const float rOuter = radius + thickness * 0.5f;
        const float rotRad = rotDeg * std::numbers::pi_v<float> / 180.f;
        const float step   = 2.f * std::numbers::pi_v<float> / static_cast<float>(segments);

        for (int k = 0; k < segments; ++k)
        {
            const float a1 = rotRad + static_cast<float>(k) * step;
            const float a2 = rotRad + static_cast<float>(k + 1) * step;
            const float c1 = std::cos(a1), s1 = std::sin(a1);
            const float c2 = std::cos(a2), s2 = std::sin(a2);

            const sf::Vector2f i1 = cam.project(center.x + c1 * rInner, center.y + s1 * rInner, worldZ);
            const sf::Vector2f o1 = cam.project(center.x + c1 * rOuter, center.y + s1 * rOuter, worldZ);
            const sf::Vector2f i2 = cam.project(center.x + c2 * rInner, center.y + s2 * rInner, worldZ);
            const sf::Vector2f o2 = cam.project(center.x + c2 * rOuter, center.y + s2 * rOuter, worldZ);

            va.append(sf::Vertex(i1, color));
            va.append(sf::Vertex(o1, color));
            va.append(sf::Vertex(o2, color));
            va.append(sf::Vertex(o2, color));
            va.append(sf::Vertex(i2, color));
            va.append(sf::Vertex(i1, color));
        }
    };

    auto addGroundDisc3D =
        [&](sf::VertexArray& va,
            sf::Vector2f     center,
            float            worldZ,
            float            radius,
            sf::Color        centerCol,
            sf::Color        edgeCol,
            int              segments = 16,
            float            rotDeg   = 0.f)
    {
        if (radius <= 0.f || segments < 3)
            return;
        const sf::Vector2f cProj = cam.project(center.x, center.y, worldZ);
        if (rotDeg == 0.f)
        {
            auto appendLUT = [&](const auto& lut)
            {
                constexpr int N = static_cast<int>(std::tuple_size_v<std::decay_t<decltype(lut)>>);
                for (int k = 0; k < N; ++k)
                {
                    const int next = (k + 1) % N;
                    const sf::Vector2f p1 = cam.project(center.x + lut[k].x * radius, center.y + lut[k].y * radius, worldZ);
                    const sf::Vector2f p2 = cam.project(center.x + lut[next].x * radius, center.y + lut[next].y * radius, worldZ);
                    va.append(sf::Vertex(cProj, centerCol));
                    va.append(sf::Vertex(p1, edgeCol));
                    va.append(sf::Vertex(p2, edgeCol));
                }
            };
            if (segments == 6)
            {
                appendLUT(kUnitPolygon6);
                return;
            }
            if (segments == 8)
            {
                appendLUT(kUnitPolygon8);
                return;
            }
            if (segments == 10)
            {
                appendLUT(kUnitPolygon10);
                return;
            }
            if (segments == 12)
            {
                appendLUT(kUnitPolygon12);
                return;
            }
            if (segments == 16)
            {
                appendLUT(kUnitPolygon16);
                return;
            }
        }
        const float rotRad = rotDeg * std::numbers::pi_v<float> / 180.f;
        const float step   = 2.f * std::numbers::pi_v<float> / static_cast<float>(segments);
        for (int k = 0; k < segments; ++k)
        {
            const float a1 = rotRad + static_cast<float>(k) * step;
            const float a2 = rotRad + static_cast<float>(k + 1) * step;
            const sf::Vector2f p1 = cam.project(center.x + std::cos(a1) * radius, center.y + std::sin(a1) * radius, worldZ);
            const sf::Vector2f p2 = cam.project(center.x + std::cos(a2) * radius, center.y + std::sin(a2) * radius, worldZ);
            va.append(sf::Vertex(cProj, centerCol));
            va.append(sf::Vertex(p1, edgeCol));
            va.append(sf::Vertex(p2, edgeCol));
        }
    };

    auto addBillboardSegment3D =
        [&](sf::VertexArray& va, sf::Vector3f p1_3d, sf::Vector3f p2_3d, float thickness, sf::Color c1, sf::Color c2)
    {
        const sf::Vector2f v1   = cam.project(p1_3d);
        const sf::Vector2f v2   = cam.project(p2_3d);
        const sf::Vector2f diff = v2 - v1;
        const float        len  = std::hypot(diff.x, diff.y);
        if (len < 0.01f)
            return;
        const float avgScale = 0.5f * (cam.getScale(p1_3d.x, p1_3d.y, p1_3d.z) + cam.getScale(p2_3d.x, p2_3d.y, p2_3d.z));
        const sf::Vector2f normal(-diff.y / len, diff.x / len);
        const sf::Vector2f off = normal * (thickness * 0.5f * avgScale);

        va.append(sf::Vertex(v1 + off, c1));
        va.append(sf::Vertex(v1 - off, c1));
        va.append(sf::Vertex(v2 - off, c2));
        va.append(sf::Vertex(v2 - off, c2));
        va.append(sf::Vertex(v2 + off, c2));
        va.append(sf::Vertex(v1 + off, c1));
    };

    const sf::Color voidColor = (campaign().currentMap == 3) ? sf::Color(10, 24, 42) : sf::Color(14, 18, 28);
    window().clear(voidColor);

    const auto rT0 = std::chrono::high_resolution_clock::now();
    {
        PROFILE_SCOPE("3D Arena & Grid");

        const int       currentMap  = std::clamp(campaign().currentMap, 1, 3);
        const sf::Color terrainBase = (currentMap == 1)   ? sf::Color(100, 192, 82)
                                      : (currentMap == 2) ? sf::Color(210, 168, 110)
                                                          : sf::Color(36, 145, 178);

        const float mapW = static_cast<float>(mapWidth());
        const float mapH = static_cast<float>(mapHeight());

        static sf::VertexArray arena3DBatch(sf::PrimitiveType::Triangles);
        arena3DBatch.clear();

        constexpr float kCliffMid     = -32.f;
        constexpr float kCliffDepth   = -85.f;
        const sf::Color cliffSouthTop = ViewStateDetail::shadeColor(terrainBase, 0.68f);
        const sf::Color cliffSouthMid = ViewStateDetail::shadeColor(terrainBase, 0.45f);
        const sf::Color cliffSouthBot = ViewStateDetail::shadeColor(terrainBase, 0.22f);
        const sf::Color cliffSideTop  = ViewStateDetail::shadeColor(terrainBase, 0.54f);
        const sf::Color cliffSideMid  = ViewStateDetail::shadeColor(terrainBase, 0.36f);
        const sf::Color cliffSideBot  = ViewStateDetail::shadeColor(terrainBase, 0.18f);

        constexpr int kCliffSegX = 4;
        constexpr int kCliffSegY = 4;
        const float   cliffStepX = mapW / static_cast<float>(kCliffSegX);
        const float   cliffStepY = mapH / static_cast<float>(kCliffSegY);

        if (camBottom >= mapH - 120.f)
        {
            for (int gx = 0; gx < kCliffSegX; ++gx)
            {
                const float x0 = gx * cliffStepX;
                const float x1 = (gx + 1) * cliffStepX;
                if (x1 < camLeft - 50.f || x0 > camRight + 50.f)
                    continue;

                appendQuad2D(arena3DBatch,
                             cam.project(x0, mapH + 10.f, kCliffDepth),
                             cam.project(x1, mapH + 10.f, kCliffDepth),
                             cam.project(x1, mapH + 4.f, kCliffMid),
                             cam.project(x0, mapH + 4.f, kCliffMid),
                             cliffSouthBot,
                             cliffSouthMid,
                             false);
                appendQuad2D(arena3DBatch,
                             cam.project(x0, mapH + 4.f, kCliffMid),
                             cam.project(x1, mapH + 4.f, kCliffMid),
                             cam.project(x1, mapH, 0.f),
                             cam.project(x0, mapH, 0.f),
                             cliffSouthMid,
                             cliffSouthTop,
                             false);
            }
        }

        if (camTop <= 120.f)
        {
            for (int gx = 0; gx < kCliffSegX; ++gx)
            {
                const float x0 = gx * cliffStepX;
                const float x1 = (gx + 1) * cliffStepX;
                if (x1 < camLeft - 50.f || x0 > camRight + 50.f)
                    continue;

                appendQuad2D(arena3DBatch,
                             cam.project(x1, 0.f, kCliffDepth),
                             cam.project(x0, 0.f, kCliffDepth),
                             cam.project(x0, 0.f, 0.f),
                             cam.project(x1, 0.f, 0.f),
                             cliffSideBot,
                             cliffSideTop,
                             true);
            }
        }

        if (camLeft <= 120.f)
        {
            for (int gy = 0; gy < kCliffSegY; ++gy)
            {
                const float y0 = gy * cliffStepY;
                const float y1 = (gy + 1) * cliffStepY;
                if (y1 < camTop - 50.f || y0 > camBottom + 50.f)
                    continue;

                appendQuad2D(arena3DBatch,
                             cam.project(-8.f, y0, kCliffDepth),
                             cam.project(-8.f, y1, kCliffDepth),
                             cam.project(-3.f, y1, kCliffMid),
                             cam.project(-3.f, y0, kCliffMid),
                             cliffSideBot,
                             cliffSideMid,
                             true);
                appendQuad2D(arena3DBatch,
                             cam.project(-3.f, y0, kCliffMid),
                             cam.project(-3.f, y1, kCliffMid),
                             cam.project(0.f, y1, 0.f),
                             cam.project(0.f, y0, 0.f),
                             cliffSideMid,
                             cliffSideTop,
                             true);
            }
        }

        if (camRight >= mapW - 120.f)
        {
            for (int gy = 0; gy < kCliffSegY; ++gy)
            {
                const float y0 = gy * cliffStepY;
                const float y1 = (gy + 1) * cliffStepY;
                if (y1 < camTop - 50.f || y0 > camBottom + 50.f)
                    continue;

                appendQuad2D(arena3DBatch,
                             cam.project(mapW + 8.f, y1, kCliffDepth),
                             cam.project(mapW + 8.f, y0, kCliffDepth),
                             cam.project(mapW + 3.f, y0, kCliffMid),
                             cam.project(mapW + 3.f, y1, kCliffMid),
                             cliffSideBot,
                             cliffSideMid,
                             true);
                appendQuad2D(arena3DBatch,
                             cam.project(mapW + 3.f, y1, kCliffMid),
                             cam.project(mapW + 3.f, y0, kCliffMid),
                             cam.project(mapW, y0, 0.f),
                             cam.project(mapW, y1, 0.f),
                             cliffSideMid,
                             cliffSideTop,
                             true);
            }
        }

        constexpr int kGridY = 12;
        const float   stepY  = mapH / static_cast<float>(kGridY);
        constexpr int kGridX = 2;
        const float   stepX  = mapW / static_cast<float>(kGridX);

        for (int gy = 0; gy < kGridY; ++gy)
        {
            const float y0 = gy * stepY;
            const float y1 = (gy + 1) * stepY;
            if (y1 < camTop - 50.f || y0 > camBottom + 50.f)
                continue;

            const float     ty0   = static_cast<float>(gy) / static_cast<float>(kGridY);
            const float     ty1   = static_cast<float>(gy + 1) / static_cast<float>(kGridY);
            const sf::Color colY0 = ViewStateDetail::shadeColor(terrainBase, 0.90f + 0.14f * ty0);
            const sf::Color colY1 = ViewStateDetail::shadeColor(terrainBase, 0.90f + 0.14f * ty1);

            for (int gx = 0; gx < kGridX; ++gx)
            {
                const float x0 = gx * stepX;
                const float x1 = (gx + 1) * stepX;
                if (x1 < camLeft - 50.f || x0 > camRight + 50.f)
                    continue;

                appendQuad2D(arena3DBatch,
                             cam.project(x0, y1, 0.f),
                             cam.project(x1, y1, 0.f),
                             cam.project(x1, y0, 0.f),
                             cam.project(x0, y0, 0.f),
                             colY1,
                             colY0,
                             false);
            }
        }
        window().draw(arena3DBatch);

        if (mClientGame->mGame->mGraphicsLevel >= 1)
        {
            mGrid.clear();
            const sf::Color gridColor(40, 55, 85, 85);
            const sf::Color rimColor(255, 230, 180, 135);
            const float     spacing = 150.f;

            const float yClampedStart = std::max(0.f, camTop - spacing);
            const float yClampedEnd   = std::min(mapH, camBottom + spacing);
            const float xClampedStart = std::max(0.f, camLeft - spacing);
            const float xClampedEnd   = std::min(mapW, camRight + spacing);

            if (yClampedStart < yClampedEnd)
            {
                const float startX = std::max(spacing, std::floor((camLeft - spacing) / spacing + 1.f) * spacing);
                const float endX   = std::min(mapW - 1.f, std::floor((camRight + spacing) / spacing) * spacing);
                for (float x = startX; x <= endX; x += spacing)
                {
                    mGrid.append(sf::Vertex(cam.project(x, yClampedStart, 0.5f), gridColor));
                    mGrid.append(sf::Vertex(cam.project(x, yClampedEnd, 0.5f), gridColor));
                }
            }

            if (xClampedStart < xClampedEnd)
            {
                const float startY = std::max(spacing, std::floor((camTop - spacing) / spacing + 1.f) * spacing);
                const float endY   = std::min(mapH - 1.f, std::floor((camBottom + spacing) / spacing) * spacing);
                for (float y = startY; y <= endY; y += spacing)
                {
                    mGrid.append(sf::Vertex(cam.project(xClampedStart, y, 0.5f), gridColor));
                    mGrid.append(sf::Vertex(cam.project(xClampedEnd, y, 0.5f), gridColor));
                }
            }

            const sf::Vector2f cNW = cam.project(0.f, 0.f, 1.f);
            const sf::Vector2f cNE = cam.project(mapW, 0.f, 1.f);
            const sf::Vector2f cSE = cam.project(mapW, mapH, 1.f);
            const sf::Vector2f cSW = cam.project(0.f, mapH, 1.f);
            mGrid.append(sf::Vertex(cNW, rimColor));
            mGrid.append(sf::Vertex(cNE, rimColor));
            mGrid.append(sf::Vertex(cNE, rimColor));
            mGrid.append(sf::Vertex(cSE, rimColor));
            mGrid.append(sf::Vertex(cSE, rimColor));
            mGrid.append(sf::Vertex(cSW, rimColor));
            mGrid.append(sf::Vertex(cSW, rimColor));
            mGrid.append(sf::Vertex(cNW, rimColor));

            window().draw(mGrid);
        }
    }
    const auto rT1 = std::chrono::high_resolution_clock::now();
    if (debug().showPanel)
        mProfiler.renderTimes["1. Terrain & Grid"] = static_cast<float>(
            std::chrono::duration_cast<std::chrono::microseconds>(rT1 - rT0).count());

    const int selectedWallId = input().dragSelectedId;

    const auto& wallRects = mAdmin.mPool.viewComponent<CRectDragDrop>();
    const auto& wallIds   = mAdmin.mPool.viewIndex<CRectDragDrop>();
    const int   wallCount = static_cast<int>(wallRects.size());

    const auto&      unitIds   = mAdmin.mPool.viewIndex<CUnit>();
    const auto&      playerIds = mAdmin.mPool.viewIndex<CPlayer>();
    std::vector<int> agentIds;
    agentIds.reserve(unitIds.size() + playerIds.size());
    agentIds.insert(agentIds.end(), unitIds.begin(), unitIds.end());
    agentIds.insert(agentIds.end(), playerIds.begin(), playerIds.end());
    const int agentCount = static_cast<int>(agentIds.size());

    const auto rTRoads0 = std::chrono::high_resolution_clock::now();
    {
        PROFILE_SCOPE("Roads & Hazards 3D");
        const int   currentLevel = mClientGame->gameState().campaign().currentMap;
        const auto& roads        = GameConfig::getLevelRoads(currentLevel);
        if (!roads.empty())
        {
            static sf::VertexArray roadsBatch(sf::PrimitiveType::Triangles);
            roadsBatch.clear();
            ViewStateDetail::drawRoads3D(roadsBatch, cam, roads, currentLevel);
            if (roadsBatch.getVertexCount() > 0)
            {
                window().draw(roadsBatch);
            }
        }
    }

    {
        const auto& envZones = mAdmin.mPool.viewComponent<CEnvironmentZone>();
        if (!envZones.empty())
        {
            static sf::VertexArray hazardPoolsBatch(sf::PrimitiveType::Triangles);
            hazardPoolsBatch.clear();
            const int currentLevel = mClientGame->gameState().campaign().currentMap;

            for (const auto& zone : envZones)
            {
                if (zone.kind == ZoneKind::Water || zone.kind == ZoneKind::Quicksand || zone.kind == ZoneKind::Lava)
                {
                    const sf::Vector2f zPos(static_cast<float>(zone.pos.x), static_cast<float>(zone.pos.y));
                    const sf::Vector2f zSize(static_cast<float>(zone.size.x), static_cast<float>(zone.size.y));
                    if (zPos.x + zSize.x < camLeft || zPos.x > camRight || zPos.y + zSize.y < camTop || zPos.y > camBottom)
                    {
                        continue;
                    }

                    ViewStateDetail::drawHazardPool3D(hazardPoolsBatch, cam, zPos, zSize, zone.kind, currentLevel, mCurrentFrame);
                }
            }

            if (hazardPoolsBatch.getVertexCount() > 0)
            {
                window().draw(hazardPoolsBatch);
            }
        }
    }
    const auto rTRoads1 = std::chrono::high_resolution_clock::now();
    if (debug().showPanel)
        mProfiler.renderTimes["2. Roads & Hazards"] = static_cast<float>(
            std::chrono::duration_cast<std::chrono::microseconds>(rTRoads1 - rTRoads0).count());

    const auto rTShadows0 = std::chrono::high_resolution_clock::now();
    {
        PROFILE_SCOPE("Ground Shadows 3D");
        if (mClientGame->mGame->mGraphicsLevel >= 1)
        {
            static sf::VertexArray shadowsBatched(sf::PrimitiveType::Triangles);
            shadowsBatched.clear();
            const sf::Color shadowColor(0, 0, 0, 82);
            const sf::Color shadowEdge(0, 0, 0, 28);
            const sf::Color enemyShadowColor(225, 30, 30, 150);
            const sf::Color enemyShadowEdge(255, 55, 55, 65);

            auto addShadowRect3D =
                [&](const sf::Vector2f& pos, const sf::Vector2f& size, float zLift = 0.f, bool isEnemy = false)
            {
                if (isEnemy)
                {
                    const float        pad = 9.f;
                    const sf::Vector2f offset(4.f + zLift * 0.15f, -2.f - zLift * 0.10f);
                    const float        x0 = pos.x - pad + offset.x;
                    const float        y0 = pos.y - pad + offset.y;
                    const float        x1 = pos.x + size.x + pad + offset.x;
                    const float        y1 = pos.y + size.y + pad + offset.y;

                    appendQuad2D(shadowsBatched,
                                 cam.project(x0, y1, 0.4f),
                                 cam.project(x1, y1, 0.4f),
                                 cam.project(x1, y0, 0.4f),
                                 cam.project(x0, y0, 0.4f),
                                 enemyShadowColor,
                                 enemyShadowEdge,
                                 false);
                    return;
                }

                const sf::Vector2f offset(11.f + zLift * 0.35f, -7.f - zLift * 0.22f);
                const float        x0 = pos.x + offset.x;
                const float        y0 = pos.y + offset.y;
                const float        x1 = x0 + size.x;
                const float        y1 = y0 + size.y;

                appendQuad2D(shadowsBatched,
                             cam.project(x0, y1, 0.4f),
                             cam.project(x1, y1, 0.4f),
                             cam.project(x1, y0, 0.4f),
                             cam.project(x0, y0, 0.4f),
                             shadowColor,
                             shadowEdge,
                             false);
            };

            auto addShadowCircle3D = [&](const sf::Vector2f& center, float radius, float zLift = 0.f, bool isEnemy = false)
            {
                if (isEnemy)
                {
                    const sf::Vector2f offset(3.f + zLift * 0.15f, -2.f - zLift * 0.10f);
                    addGroundDisc3D(shadowsBatched, center + offset, 0.4f, radius * 1.25f, enemyShadowColor, enemyShadowEdge, 12);
                    return;
                }

                const sf::Vector2f offset(7.f + zLift * 0.35f, -5.f - zLift * 0.22f);
                addGroundDisc3D(shadowsBatched, center + offset, 0.4f, radius * 1.08f, shadowColor, shadowEdge, 10);
            };

            for (int i = 0; i < wallCount; i++)
            {
                const int   wallId   = wallIds[i];
                const auto* wallMask = mAdmin.mPool.getMask(wallId);
                const bool  isBase   = mAdmin.mPool.hasComponent<CBase>(wallMask);
                const auto* wallComp = mAdmin.mPool.hasComponent<CWall>(wallMask)
                                           ? &*mAdmin.mPool.getComponent<CWall>(wallId)
                                           : nullptr;
                const bool  isPortal = (wallComp && wallComp->kind == WallKind::DesignatedPoint);

                if (isPortal)
                {
                    continue;
                }

                const sf::Vector2f& wPos  = wallRects[i].pos;
                const sf::Vector2f& wSize = wallRects[i].size;

                if (selectedWallId != wallId && (wPos.x + wSize.x + 30.f < camLeft || wPos.x - 30.f > camRight ||
                                                 wPos.y + wSize.y + 30.f < camTop || wPos.y - 30.f > camBottom))
                {
                    continue;
                }

                const bool isEnemyStruct = isOpponent(wallId);

                if (isBase)
                {
                    const bool         isMainBase = wSize.x > 120.f;
                    const sf::Vector2f center     = wPos + wSize * 0.5f;

                    if (isMainBase)
                    {
                        addShadowRect3D(wPos, wSize, 12.f, isEnemyStruct);
                        if (selectedWallId == wallId && (input().leftMousePressed || input().activeWorldFingerId != -1))
                        {
                            const sf::Vector2f dragPos{input().mouseWorldPos.x - input().dragOffsetX,
                                                       input().mouseWorldPos.y - input().dragOffsetY};
                            addShadowRect3D(dragPos, wSize, 28.f, isEnemyStruct);
                        }
                    }
                    else
                    {
                        const float radius = wSize.x * 0.48f;
                        addShadowCircle3D(center, radius, 16.f, isEnemyStruct);
                        if (selectedWallId == wallId && (input().leftMousePressed || input().activeWorldFingerId != -1))
                        {
                            const sf::Vector2f dragCenter{input().mouseWorldPos.x - input().dragOffsetX + wSize.x * 0.5f,
                                                          input().mouseWorldPos.y - input().dragOffsetY + wSize.y * 0.5f};
                            addShadowCircle3D(dragCenter, radius, 28.f, isEnemyStruct);
                        }
                    }
                }
                else
                {
                    const auto* wallComponent = mAdmin.mPool.hasComponent<CWall>(wallMask)
                                                    ? &*mAdmin.mPool.getComponent<CWall>(wallId)
                                                    : nullptr;
                    if (wallComponent && wallComponent->kind == WallKind::Rock)
                    {
                        const sf::Vector2f center = wPos + wSize * 0.5f;
                        const float        radius = std::min(wSize.x, wSize.y) * 0.45f;
                        addShadowCircle3D(center, radius, 14.f, false);
                    }
                    else
                    {
                        addShadowRect3D(wPos, wSize, 6.f, isEnemyStruct);
                        if (selectedWallId == wallId && (input().leftMousePressed || input().activeWorldFingerId != -1))
                        {
                            const sf::Vector2f dragPos{input().mouseWorldPos.x - input().dragOffsetX,
                                                       input().mouseWorldPos.y - input().dragOffsetY};
                            addShadowRect3D(dragPos, wSize, 24.f, isEnemyStruct);
                        }
                    }
                }
            }

            const auto& visionGrid = mClientGame->gameState().nav().getVisionGrid(localTeamId());
            const int   spacing    = mClientGame->gameState().nav().visionSpacing > 0
                                         ? mClientGame->gameState().nav().visionSpacing
                                         : 16;

            for (int i = 0; i < agentCount; i++)
            {
                const int   entityId = agentIds[i];
                const auto* mask     = mAdmin.mPool.getMask(entityId);
                const bool  isPlayer = mAdmin.mPool.hasComponent<CPlayer>(mask);
                if (isPlayer)
                {
                    const auto player = mAdmin.mPool.getComponent<CPlayer>(entityId);
                    if (player->respawnTimer > 0)
                    {
                        continue;
                    }
                }

                if (mAdmin.mPool.hasComponent<CPosition>(mask) && mAdmin.mPool.hasComponent<CBoundCircle>(mask))
                {
                    const auto bound    = mAdmin.mPool.getComponent<CBoundCircle>(entityId);
                    const auto position = mAdmin.mPool.getComponent<CPosition>(entityId);
                    const sf::Vector2f renderPos(static_cast<float>(position->pos.x), static_cast<float>(position->pos.y));

                    if (renderPos.x + bound->radius + 25.f < camLeft || renderPos.x - bound->radius - 25.f > camRight ||
                        renderPos.y + bound->radius + 25.f < camTop || renderPos.y - bound->radius - 25.f > camBottom)
                    {
                        continue;
                    }

                    const int  vx             = static_cast<int>(renderPos.x) / spacing;
                    const int  vy             = static_cast<int>(renderPos.y) / spacing;
                    const bool isOpponentUnit = isOpponent(entityId);
                    if (visionGrid.isValidCell(vx, vy) &&
                        (visionGrid[vx][vy] == 0 || (isOpponentUnit && visionGrid[vx][vy] == 1)))
                    {
                        continue;
                    }

                    const float     shadowR = bound->radius * (isPlayer ? 0.85f : 0.65f);
                    const sf::Color unitShadowCenter(0, 0, 0, 72);
                    const sf::Color unitShadowEdge(0, 0, 0, 0);
                    addGroundDisc3D(shadowsBatched,
                                    renderPos + sf::Vector2f(0.f, 0.8f),
                                    0.15f,
                                    shadowR,
                                    unitShadowCenter,
                                    unitShadowEdge,
                                    8);
                }
            }

            window().draw(shadowsBatched);
        }
    }
    const auto rTShadows1 = std::chrono::high_resolution_clock::now();
    if (debug().showPanel)
        mProfiler.renderTimes["3. Ground Shadows 3D"] = static_cast<float>(
            std::chrono::duration_cast<std::chrono::microseconds>(rTShadows1 - rTShadows0).count());

    static sf::VertexArray groundRangeBatch(sf::PrimitiveType::Triangles);
    static sf::VertexArray healthBars(sf::PrimitiveType::Triangles);
    static sf::VertexArray scene3DTriBatch(sf::PrimitiveType::Triangles);
    static sf::VertexArray scene3DLineBatch(sf::PrimitiveType::Lines);
    static sf::VertexArray vfxBatched(sf::PrimitiveType::Triangles);

    groundRangeBatch.clear();
    healthBars.clear();
    scene3DTriBatch.clear();
    scene3DLineBatch.clear();
    vfxBatched.clear();

    auto& bulletsBatched = vfxBatched;
    auto& beamsBatched   = vfxBatched;
    auto& ringsBatched   = vfxBatched;
    auto& sparks         = vfxBatched;
    auto& shards         = vfxBatched;
    auto& healBatched    = vfxBatched;

    const auto rTScene0 = std::chrono::high_resolution_clock::now();
    {
        PROFILE_SCOPE("3D Depth-Sorted Scene");

        enum class DrawItemKind : uint8_t
        {
            WallOrBase      = 0,
            Agent           = 1,
            Pickup          = 2,
            EnvironmentBush = 3,
            UtilityPole     = 4
        };
        struct DrawItem3D
        {
            float        sortY;
            DrawItemKind kind;
            int          indexOrId;
        };
        static std::vector<DrawItem3D> drawQueue;
        drawQueue.clear();
        drawQueue.reserve(wallCount + agentCount + 64);

        const auto& visionGrid = mClientGame->gameState().nav().getVisionGrid(localTeamId());
        const int   spacing    = mClientGame->gameState().nav().visionSpacing > 0
                                     ? mClientGame->gameState().nav().visionSpacing
                                     : 16;

        for (int i = 0; i < wallCount; i++)
        {
            const int     wallId     = wallIds[i];
            const auto*   wallMask   = mAdmin.mPool.getMask(wallId);
            const bool    isSelected = selectedWallId == wallId;
            const bool    isDragging = isSelected && (input().leftMousePressed || input().activeWorldFingerId != -1);
            const bool    isBase     = mAdmin.mPool.hasComponent<CBase>(wallMask);
            CRectDragDrop wallRect   = wallRects[i];

            if (isDragging)
            {
                const auto* wallComp = mAdmin.mPool.hasComponent<CWall>(wallMask)
                                           ? &*mAdmin.mPool.getComponent<CWall>(wallId)
                                           : nullptr;
                const bool  isPortal = (wallComp && wallComp->kind == WallKind::DesignatedPoint);
                if (!isPortal)
                {
                    addBox3D(scene3DTriBatch,
                             &scene3DLineBatch,
                             wallRects[i].pos.x,
                             wallRects[i].pos.y,
                             0.f,
                             wallRects[i].size.x,
                             wallRects[i].size.y,
                             isBase ? 26.f : 32.f,
                             sf::Color(70, 140, 210, 48),
                             sf::Color(100, 215, 255, 165));
                }

                const sf::Vector2f origCenter = wallRects[i].pos + wallRects[i].size * 0.5f;
                const sf::Vector2f dragCenter{input().mouseWorldPos.x - input().dragOffsetX + wallRects[i].size.x * 0.5f,
                                              input().mouseWorldPos.y - input().dragOffsetY + wallRects[i].size.y * 0.5f};
                addBillboardSegment3D(scene3DTriBatch,
                                      {origCenter.x, origCenter.y, 16.f},
                                      {dragCenter.x, dragCenter.y, 16.f},
                                      2.5f,
                                      isPortal ? sf::Color(255, 215, 35, 110) : sf::Color(100, 200, 255, 110),
                                      isPortal ? sf::Color(255, 245, 150, 210) : sf::Color(100, 255, 255, 210));

                wallRect.pos.x = input().mouseWorldPos.x - input().dragOffsetX;
                wallRect.pos.y = input().mouseWorldPos.y - input().dragOffsetY;
            }

            if (!isSelected && !isDragging)
            {
                const sf::Vector2f center = wallRect.pos + wallRect.size * 0.5f;
                float              reach = static_cast<float>(std::max(wallRect.size.x, wallRect.size.y)) * 0.5f + 45.f;
                if (isBase)
                {
                    const auto* baseComponent = &*mAdmin.mPool.getComponent<CBase>(wallId);
                    if (baseComponent)
                        reach = std::max(reach, static_cast<float>(baseComponent->defenseRange));
                }
                else if (mAdmin.mPool.hasComponent<CWall>(wallMask))
                {
                    const auto* wallComponent = &*mAdmin.mPool.getComponent<CWall>(wallId);
                    if (wallComponent && wallComponent->kind != WallKind::Defensive)
                        reach = std::max(reach, static_cast<float>(wallComponent->effectRange));
                }

                if (center.x + reach < camLeft || center.x - reach > camRight || center.y + reach < camTop ||
                    center.y - reach > camBottom)
                {
                    continue;
                }
            }

            const float sortY = wallRect.pos.y + wallRect.size.y * 0.5f;
            drawQueue.push_back(DrawItem3D{sortY, DrawItemKind::WallOrBase, i});
        }

        for (int i = 0; i < agentCount; i++)
        {
            const int   entityId = agentIds[i];
            const auto* mask     = mAdmin.mPool.getMask(entityId);
            if (mAdmin.mPool.hasComponent<CPlayer>(mask))
            {
                const auto player = mAdmin.mPool.getComponent<CPlayer>(entityId);
                if (player->respawnTimer > 0)
                {
                    continue;
                }
            }

            const auto         bound    = mAdmin.mPool.getComponent<CBoundCircle>(entityId);
            const auto         position = mAdmin.mPool.getComponent<CPosition>(entityId);
            const sf::Vector2f renderPos(static_cast<float>(position->pos.x), static_cast<float>(position->pos.y));

            const float cullMargin = bound->radius + 55.f;
            if (renderPos.x + cullMargin < camLeft || renderPos.x - cullMargin > camRight ||
                renderPos.y + cullMargin < camTop || renderPos.y - cullMargin > camBottom)
            {
                continue;
            }

            const int  vx             = static_cast<int>(renderPos.x) / spacing;
            const int  vy             = static_cast<int>(renderPos.y) / spacing;
            const bool isOpponentUnit = isOpponent(entityId);
            if (visionGrid.isValidCell(vx, vy) && (visionGrid[vx][vy] == 0 || (isOpponentUnit && visionGrid[vx][vy] == 1)))
            {
                continue;
            }

            drawQueue.push_back(DrawItem3D{renderPos.y, DrawItemKind::Agent, entityId});
        }

        const auto& pickupIds   = mAdmin.mPool.viewIndex<CPickup>();
        const int   pickupCount = static_cast<int>(pickupIds.size());
        for (int i = 0; i < pickupCount; i++)
        {
            const int   pickupId   = pickupIds[i];
            const auto* pickupMask = mAdmin.mPool.getMask(pickupId);
            if (!mAdmin.mPool.hasComponent<CPosition>(pickupMask))
                continue;

            const auto position = mAdmin.mPool.getComponent<CPosition>(pickupId);
            if (position->pos.x + 25.f < camLeft || position->pos.x - 25.f > camRight ||
                position->pos.y + 25.f < camTop || position->pos.y - 25.f > camBottom)
            {
                continue;
            }
            drawQueue.push_back(DrawItem3D{static_cast<float>(position->pos.y), DrawItemKind::Pickup, pickupId});
        }

        const auto& envZones     = mAdmin.mPool.viewComponent<CEnvironmentZone>();
        const int   envZoneCount = static_cast<int>(envZones.size());
        for (int i = 0; i < envZoneCount; i++)
        {
            if (envZones[i].kind == ZoneKind::Bush || envZones[i].kind == ZoneKind::Tree)
            {
                const sf::Vector2f bPos(static_cast<float>(envZones[i].pos.x), static_cast<float>(envZones[i].pos.y));
                const sf::Vector2f bSize(static_cast<float>(envZones[i].size.x), static_cast<float>(envZones[i].size.y));
                if (bPos.x + bSize.x < camLeft || bPos.x > camRight || bPos.y + bSize.y < camTop || bPos.y > camBottom)
                {
                    continue;
                }
                const float sortY = bPos.y + bSize.y * 0.5f;
                drawQueue.push_back(DrawItem3D{sortY, DrawItemKind::EnvironmentBush, i});
            }
        }


        std::ranges::sort(drawQueue, [](const DrawItem3D& a, const DrawItem3D& b) { return a.sortY < b.sortY; });

        for (const auto& item : drawQueue)
        {
            if (item.kind == DrawItemKind::WallOrBase)
            {
                const int   i          = item.indexOrId;
                const int   wallId     = wallIds[i];
                const auto* wallMask   = mAdmin.mPool.getMask(wallId);
                const bool  isSelected = selectedWallId == wallId;
                const bool  isDragging = isSelected && (input().leftMousePressed || input().activeWorldFingerId != -1);
                const bool  isBase     = mAdmin.mPool.hasComponent<CBase>(wallMask);
                CRectDragDrop wallRect = wallRects[i];
                if (isDragging)
                {
                    wallRect.pos.x = input().mouseWorldPos.x - input().dragOffsetX;
                    wallRect.pos.y = input().mouseWorldPos.y - input().dragOffsetY;
                }

                if (isBase)
                {
                    const auto* baseComponent   = &*mAdmin.mPool.getComponent<CBase>(wallId);
                    const auto* healthComponent = mAdmin.mPool.hasComponent<CHealth>(wallMask)
                                                      ? &*mAdmin.mPool.getComponent<CHealth>(wallId)
                                                      : nullptr;
                    ViewStateDetail::drawBase(mClientGame->mGame,
                                              cam,
                                              wallRect,
                                              baseComponent,
                                              healthComponent,
                                              isSelected,
                                              mCurrentFrame,
                                              scene3DTriBatch,
                                              scene3DLineBatch,
                                              &healthBars,
                                              isFriendly(wallId));
                }
                else
                {
                    const auto* wallComponent = mAdmin.mPool.hasComponent<CWall>(wallMask)
                                                    ? &*mAdmin.mPool.getComponent<CWall>(wallId)
                                                    : nullptr;
                    ViewStateDetail::drawWall(mClientGame->mGame,
                                              cam,
                                              wallRect,
                                              wallComponent,
                                              isSelected,
                                              mCurrentFrame,
                                              mRenderRandom,
                                              scene3DTriBatch,
                                              scene3DLineBatch,
                                              mClientGame->gameState().campaign().currentMap);
                }
            }
            else if (item.kind == DrawItemKind::EnvironmentBush)
            {
                const int          i    = item.indexOrId;
                const auto&        zone = envZones[i];
                const sf::Vector2f bPos(static_cast<float>(zone.pos.x), static_cast<float>(zone.pos.y));
                const sf::Vector2f bSize(static_cast<float>(zone.size.x), static_cast<float>(zone.size.y));
                if (zone.kind == ZoneKind::Tree)
                {
                    ViewStateDetail::drawTree3D(scene3DTriBatch,
                                                cam,
                                                bPos,
                                                bSize,
                                                mClientGame->gameState().campaign().currentMap,
                                                mCurrentFrame);
                }
                else
                {
                    ViewStateDetail::drawBush3D(scene3DTriBatch,
                                                cam,
                                                bPos,
                                                bSize,
                                                mClientGame->gameState().campaign().currentMap,
                                                mCurrentFrame);
                }
            }
            else if (item.kind == DrawItemKind::Pickup)
            {
                const int   pickupId  = item.indexOrId;
                const auto  position  = mAdmin.mPool.getComponent<CPosition>(pickupId);
                const auto  pickup    = mAdmin.mPool.getComponent<CPickup>(pickupId);
                const float lifeRatio = std::clamp(pickup->lifeTime / 300.f, 0.f, 1.f);
                const auto  alpha     = static_cast<std::uint8_t>(215 * lifeRatio);

                sf::Color pickupColor = sf::Color::White;
                switch (pickup->kind)
                {
                    case PowerupKind::HealthBoost:
                        pickupColor = sf::Color(50, 255, 80, alpha);
                        break;
                    case PowerupKind::DamageBoost:
                        pickupColor = sf::Color(255, 65, 65, alpha);
                        break;
                    case PowerupKind::SpeedBoost:
                        pickupColor = sf::Color(50, 210, 255, alpha);
                        break;
                    case PowerupKind::AttackRateBoost:
                        pickupColor = sf::Color(255, 240, 50, alpha);
                        break;
                    case PowerupKind::ShieldBoost:
                        pickupColor = sf::Color(210, 70, 255, alpha);
                        break;
                }

                const sf::Vector2f pos(static_cast<float>(position->pos.x), static_cast<float>(position->pos.y));
                const float        bobZ = 9.f + std::sin(mCurrentFrame * 0.12f + pickupId) * 3.8f;
                const float        rot  = mCurrentFrame * 0.08f + pickupId * 0.7f;

                addPrismLUT3D(scene3DTriBatch,
                              pos,
                              7.2f,
                              5.8f,
                              0.f,
                              3.8f,
                              kUnitPolygon6,
                              sf::Color(pickupColor.r, pickupColor.g, pickupColor.b, static_cast<uint8_t>(alpha * 0.65f)));
                addOctahedron3D(scene3DTriBatch, pos, 6.2f, bobZ, bobZ + 8.5f, bobZ + 17.f, rot, pickupColor);
                addOctahedron3D(scene3DTriBatch,
                                pos,
                                3.4f,
                                bobZ + 3.5f,
                                bobZ + 8.5f,
                                bobZ + 13.5f,
                                -rot * 1.3f,
                                sf::Color(255, 255, 255, alpha));
            }
            else if (item.kind == DrawItemKind::Agent)
            {
                const int          entityId = item.indexOrId;
                const auto*        mask     = mAdmin.mPool.getMask(entityId);
                const auto         bound    = mAdmin.mPool.getComponent<CBoundCircle>(entityId);
                const auto         position = mAdmin.mPool.getComponent<CPosition>(entityId);
                const sf::Vector2f renderPos(static_cast<float>(position->pos.x), static_cast<float>(position->pos.y));
                const bool         isPlayerUnit = mAdmin.mPool.hasComponent<CPlayer>(mask);
                const float        unitScale    = isPlayerUnit ? 1.0f : 0.78f;
                const float        r            = bound->radius * unitScale;

                const bool isFriendlyUnit = isFriendly(entityId);
                sf::Color fillColor = isFriendlyUnit ? ViewStateDetail::kFriendlyColor : ViewStateDetail::kOpponentColor;
                sf::Color  outlineCol = sf::Color(40, 40, 40, 150);
                const auto unit       = !isPlayerUnit && mAdmin.mPool.hasComponent<CUnit>(mask)
                                            ? &*mAdmin.mPool.getComponent<CUnit>(entityId)
                                            : nullptr;
                if (unit)
                {
                    fillColor  = ViewStateDetail::getUnitClassColor(unit->unitClass);
                    outlineCol = ViewStateDetail::getStateColor(unit->state);
                }

                if (GameStateDetail::isPointInBush(&mClientGame->gameState(), position->pos))
                {
                    fillColor.a  = 135;
                    outlineCol.a = 150;
                }

                sf::Vector2f dir(0.f, isFriendlyUnit ? -1.f : 1.f);
                float        speed = 0.f;
                if (mAdmin.mPool.hasComponent<CVelocity>(mask))
                {
                    const auto velocity = mAdmin.mPool.getComponent<CVelocity>(entityId);
                    speed               = static_cast<float>(velocity->vel.length());
                    if (speed > 0.05f)
                    {
                        dir = {static_cast<float>(velocity->vel.x) / speed, static_cast<float>(velocity->vel.y) / speed};
                    }
                }
                const sf::Vector2f perp(-dir.y, dir.x);
                const bool         isMoving  = speed > 0.08f;
                const float        walkPhase = mCurrentFrame * 0.34f + entityId * 1.73f;
                const float        bobZ      = isMoving ? std::abs(std::sin(walkPhase)) * 2.2f : 0.f;
                const float        swing     = isMoving ? std::sin(walkPhase) * 3.2f : 0.f;

                float unitTopZ = 44.f;

                if (isPlayerUnit)
                {
                    const int  cId     = getPlayerClientId(entityId);
                    const auto palette = ViewStateDetail::getPlayerPalette(cId);


                    const sf::Color bootCol(215, 230, 245);
                    const sf::Color legArmorCol = palette.innerOutline;
                    const float     legW        = r * 0.30f;
                    const float     legD        = r * 0.24f;
                    const float     legSpacing  = r * 0.38f;

                    addOrientedBox3D(scene3DTriBatch,
                                     renderPos - perp * legSpacing + dir * swing,
                                     dir,
                                     legW,
                                     legD,
                                     0.f,
                                     bobZ + 11.f,
                                     bootCol);
                    addOrientedBox3D(scene3DTriBatch,
                                     renderPos + perp * legSpacing - dir * swing,
                                     dir,
                                     legW,
                                     legD,
                                     0.f,
                                     bobZ + 11.f,
                                     bootCol);

                    addOrientedBox3D(scene3DTriBatch,
                                     renderPos - perp * legSpacing + dir * (swing + legD * 0.45f),
                                     dir,
                                     legW * 0.90f,
                                     1.6f,
                                     bobZ + 6.f,
                                     bobZ + 10.5f,
                                     legArmorCol);
                    addOrientedBox3D(scene3DTriBatch,
                                     renderPos + perp * legSpacing - dir * (swing - legD * 0.45f),
                                     dir,
                                     legW * 0.90f,
                                     1.6f,
                                     bobZ + 6.f,
                                     bobZ + 10.5f,
                                     legArmorCol);

                    addOrientedBox3D(scene3DTriBatch,
                                     renderPos,
                                     dir,
                                     r * 0.65f,
                                     r * 0.42f,
                                     bobZ + 10.5f,
                                     bobZ + 14.f,
                                     sf::Color(195, 210, 225));
                    addOrientedBox3D(scene3DTriBatch,
                                     renderPos + dir * (r * 0.22f),
                                     dir,
                                     r * 0.20f,
                                     1.5f,
                                     bobZ + 11.2f,
                                     bobZ + 13.5f,
                                     palette.outerPulse);

                    addPrismLUT3D(scene3DTriBatch,
                                  renderPos,
                                  r * 0.72f,
                                  r * 0.64f,
                                  bobZ + 13.5f,
                                  bobZ + 20.f,
                                  kUnitPolygon6,
                                  palette.fill);
                    addPrismLUT3D(scene3DTriBatch,
                                  renderPos,
                                  r * 0.66f,
                                  r * 0.58f,
                                  bobZ + 20.f,
                                  bobZ + 25.5f,
                                  kUnitPolygon6,
                                  ViewStateDetail::shadeColor(palette.fill, 1.15f));

                    addOrientedBox3D(scene3DTriBatch,
                                     renderPos + dir * (r * 0.32f),
                                     dir,
                                     r * 0.28f,
                                     1.2f,
                                     bobZ + 17.5f,
                                     bobZ + 22.f,
                                     sf::Color(245, 255, 255));

                    const float cloakSway = isMoving ? std::sin(walkPhase * 0.9f) * 1.8f : 0.f;
                    addOrientedBox3D(scene3DTriBatch,
                                     renderPos - dir * (r * 0.35f) + perp * cloakSway,
                                     dir,
                                     r * 0.50f,
                                     1.5f,
                                     bobZ + 14.f,
                                     bobZ + 24.f,
                                     ViewStateDetail::shadeColor(palette.fill, 0.85f));

                    const sf::Color pauldronCol = palette.innerOutline;
                    addOrientedBox3D(scene3DTriBatch,
                                     renderPos - perp * (r * 0.68f),
                                     dir,
                                     r * 0.34f,
                                     r * 0.24f,
                                     bobZ + 20.5f,
                                     bobZ + 26.5f,
                                     pauldronCol);
                    addOrientedBox3D(scene3DTriBatch,
                                     renderPos + perp * (r * 0.68f),
                                     dir,
                                     r * 0.34f,
                                     r * 0.24f,
                                     bobZ + 20.5f,
                                     bobZ + 26.5f,
                                     pauldronCol);

                    const sf::Vector2f riflePos = renderPos + perp * (r * 0.65f) + dir * (r * 0.25f);
                    addOrientedBox3D(scene3DTriBatch,
                                     riflePos,
                                     dir,
                                     r * 0.62f,
                                     1.8f,
                                     bobZ + 17.5f,
                                     bobZ + 22.5f,
                                     sf::Color(228, 240, 250));
                    addOrientedBox3D(scene3DTriBatch,
                                     riflePos + dir * (r * 0.36f),
                                     dir,
                                     r * 0.16f,
                                     1.4f,
                                     bobZ + 18.5f,
                                     bobZ + 21.5f,
                                     sf::Color(100, 245, 255));

                    addPrismLUT3D(scene3DTriBatch,
                                  renderPos,
                                  r * 0.44f,
                                  r * 0.34f,
                                  bobZ + 25.5f,
                                  bobZ + 33.5f,
                                  kUnitPolygon8,
                                  palette.fill);
                    addOrientedBox3D(scene3DTriBatch,
                                     renderPos,
                                     dir,
                                     r * 0.32f,
                                     1.2f,
                                     bobZ + 32.5f,
                                     bobZ + 35.5f,
                                     palette.outerPulse);
                    addOrientedBox3D(scene3DTriBatch,
                                     renderPos + dir * (r * 0.34f),
                                     dir,
                                     1.6f,
                                     r * 0.36f,
                                     bobZ + 27.5f,
                                     bobZ + 30.5f,
                                     sf::Color(220, 255, 255));

                    unitTopZ = bobZ + 36.f;
                }
                else if (unit->unitClass == UnitClass::Tank)
                {
                    const sf::Color treadCol(45, 50, 58);
                    addOrientedBox3D(scene3DTriBatch, renderPos - perp * (r * 0.80f), dir, r * 1.12f, r * 0.30f, 0.f, 15.f, treadCol);
                    addOrientedBox3D(scene3DTriBatch, renderPos + perp * (r * 0.80f), dir, r * 1.12f, r * 0.30f, 0.f, 15.f, treadCol);

                    addOrientedBox3D(scene3DTriBatch, renderPos, dir, r * 0.98f, r * 0.72f, 5.f, 24.f, fillColor);
                    addOrientedBox3D(scene3DTriBatch, renderPos + dir * (r * 0.96f), dir, r * 0.16f, r * 0.68f, 7.f, 18.f, outlineCol);

                    addPrismLUT3D(scene3DTriBatch,
                                  renderPos,
                                  r * 0.68f,
                                  r * 0.56f,
                                  24.f,
                                  37.f,
                                  kUnitPolygon8,
                                  ViewStateDetail::shadeColor(fillColor, 1.14f));
                    addPrismLUT3D(scene3DTriBatch,
                                  renderPos - dir * (r * 0.16f),
                                  r * 0.30f,
                                  r * 0.26f,
                                  37.f,
                                  42.5f,
                                  kUnitPolygon6,
                                  outlineCol);

                    addOrientedBox3D(scene3DTriBatch,
                                     renderPos + dir * (r * 0.88f),
                                     dir,
                                     r * 0.72f,
                                     r * 0.18f,
                                     28.f,
                                     34.f,
                                     sf::Color(75, 82, 95));
                    addOrientedBox3D(scene3DTriBatch, renderPos + dir * (r * 1.55f), dir, 3.0f, r * 0.26f, 27.f, 35.f, outlineCol);
                    unitTopZ = 45.f;
                }
                else if (unit->unitClass == UnitClass::Archer || unit->unitClass == UnitClass::Shooter)
                {
                    const float hoverZ = 13.f + std::sin(mCurrentFrame * 0.12f + entityId) * 4.0f;

                    addPrismLUT3D(scene3DTriBatch, renderPos, r * 0.52f, r * 0.68f, hoverZ - 6.f, hoverZ, kUnitPolygon6, outlineCol, false);

                    addOrientedBox3D(scene3DTriBatch,
                                     renderPos - dir * (r * 0.1f),
                                     perp,
                                     r * 1.25f,
                                     r * 0.28f,
                                     hoverZ + 3.f,
                                     hoverZ + 10.f,
                                     ViewStateDetail::shadeColor(fillColor, 0.88f));

                    addPrismLUT3D(scene3DTriBatch, renderPos, r * 0.92f, r * 0.70f, hoverZ, hoverZ + 15.f, kUnitPolygon4, fillColor);

                    addOrientedBox3D(scene3DTriBatch,
                                     renderPos - perp * (r * 0.40f) + dir * (r * 0.68f),
                                     dir,
                                     r * 0.50f,
                                     2.0f,
                                     hoverZ + 5.f,
                                     hoverZ + 10.f,
                                     sf::Color(220, 245, 255));
                    addOrientedBox3D(scene3DTriBatch,
                                     renderPos + perp * (r * 0.40f) + dir * (r * 0.68f),
                                     dir,
                                     r * 0.50f,
                                     2.0f,
                                     hoverZ + 5.f,
                                     hoverZ + 10.f,
                                     sf::Color(220, 245, 255));

                    addOctahedron3D(scene3DTriBatch,
                                    renderPos,
                                    r * 0.46f,
                                    hoverZ + 10.f,
                                    hoverZ + 19.f,
                                    hoverZ + 28.f,
                                    mCurrentFrame * 0.10f + entityId,
                                    sf::Color(235, 250, 255));
                    unitTopZ = hoverZ + 31.f;
                }
                else if (unit->unitClass == UnitClass::Berserker)
                {
                    const sf::Color legCol = ViewStateDetail::shadeColor(fillColor, 0.62f);
                    addOrientedBox3D(scene3DTriBatch,
                                     renderPos - perp * (r * 0.42f) + dir * swing,
                                     dir,
                                     r * 0.32f,
                                     r * 0.24f,
                                     0.f,
                                     bobZ + 11.f,
                                     legCol);
                    addOrientedBox3D(scene3DTriBatch,
                                     renderPos + perp * (r * 0.42f) - dir * swing,
                                     dir,
                                     r * 0.32f,
                                     r * 0.24f,
                                     0.f,
                                     bobZ + 11.f,
                                     legCol);

                    addOrientedBox3D(scene3DTriBatch, renderPos, dir, r * 0.98f, r * 0.74f, bobZ + 8.f, bobZ + 24.f, fillColor);
                    addPrismLUT3D(scene3DTriBatch,
                                  renderPos + dir * (r * 0.16f),
                                  r * 0.78f,
                                  r * 0.54f,
                                  bobZ + 24.f,
                                  bobZ + 34.f,
                                  kUnitPolygon6,
                                  ViewStateDetail::shadeColor(fillColor, 1.14f));

                    addOrientedBox3D(scene3DTriBatch,
                                     renderPos - perp * (r * 0.80f),
                                     dir,
                                     r * 0.36f,
                                     r * 0.24f,
                                     bobZ + 22.f,
                                     bobZ + 31.f,
                                     outlineCol);
                    addOrientedBox3D(scene3DTriBatch,
                                     renderPos + perp * (r * 0.80f),
                                     dir,
                                     r * 0.36f,
                                     r * 0.24f,
                                     bobZ + 22.f,
                                     bobZ + 31.f,
                                     outlineCol);

                    const sf::Color bladeCol(255, 215, 215);
                    addOrientedBox3D(scene3DTriBatch,
                                     renderPos - perp * (r * 0.85f) + dir * (r * 0.48f + swing),
                                     dir,
                                     r * 0.74f,
                                     2.4f,
                                     bobZ + 13.f,
                                     bobZ + 22.f,
                                     bladeCol);
                    addOrientedBox3D(scene3DTriBatch,
                                     renderPos + perp * (r * 0.85f) + dir * (r * 0.48f - swing),
                                     dir,
                                     r * 0.74f,
                                     2.4f,
                                     bobZ + 13.f,
                                     bobZ + 22.f,
                                     bladeCol);

                    addOctahedron3D(scene3DTriBatch,
                                    renderPos - dir * (r * 0.15f),
                                    r * 0.40f,
                                    bobZ + 29.f,
                                    bobZ + 37.f,
                                    bobZ + 45.f,
                                    0.f,
                                    sf::Color(255, 50, 50));

                    addOrientedBox3D(scene3DTriBatch,
                                     renderPos + dir * (r * 0.64f),
                                     dir,
                                     2.2f,
                                     r * 0.44f,
                                     bobZ + 26.f,
                                     bobZ + 31.f,
                                     sf::Color(255, 240, 180));
                    unitTopZ = bobZ + 47.f;
                }
                else
                {
                    const sf::Color legCol = ViewStateDetail::shadeColor(fillColor, 0.65f);
                    addOrientedBox3D(scene3DTriBatch,
                                     renderPos - perp * (r * 0.38f) + dir * swing,
                                     dir,
                                     r * 0.30f,
                                     r * 0.24f,
                                     0.f,
                                     bobZ + 11.f,
                                     legCol);
                    addOrientedBox3D(scene3DTriBatch,
                                     renderPos + perp * (r * 0.38f) - dir * swing,
                                     dir,
                                     r * 0.30f,
                                     r * 0.24f,
                                     0.f,
                                     bobZ + 11.f,
                                     legCol);

                    addPrismLUT3D(scene3DTriBatch, renderPos, r * 0.88f, r * 0.80f, bobZ + 9.f, bobZ + 23.f, kUnitPolygon8, fillColor);
                    addPrismLUT3D(scene3DTriBatch,
                                  renderPos,
                                  r * 0.80f,
                                  r * 0.66f,
                                  bobZ + 23.f,
                                  bobZ + 34.f,
                                  kUnitPolygon8,
                                  ViewStateDetail::shadeColor(fillColor, 1.12f));

                    addOrientedBox3D(scene3DTriBatch,
                                     renderPos + perp * (r * 0.72f),
                                     dir,
                                     r * 0.32f,
                                     r * 0.22f,
                                     bobZ + 26.f,
                                     bobZ + 34.f,
                                     outlineCol);

                    addPrismLUT3D(scene3DTriBatch, renderPos, r * 0.56f, r * 0.38f, bobZ + 34.f, bobZ + 43.f, kUnitPolygon8, fillColor);
                    addOrientedBox3D(scene3DTriBatch,
                                     renderPos + dir * (r * 0.46f),
                                     dir,
                                     2.2f,
                                     r * 0.40f,
                                     bobZ + 36.f,
                                     bobZ + 40.5f,
                                     isFriendlyUnit ? sf::Color(230, 255, 255) : sf::Color(255, 210, 190));

                    addOrientedBox3D(scene3DTriBatch,
                                     renderPos + perp * (r * 0.75f) + dir * (r * 0.38f),
                                     dir,
                                     r * 0.54f,
                                     2.2f,
                                     bobZ + 18.f,
                                     bobZ + 24.f,
                                     sf::Color(90, 100, 115));
                    addOrientedBox3D(scene3DTriBatch,
                                     renderPos - perp * (r * 0.76f) + dir * (r * 0.16f),
                                     dir,
                                     r * 0.42f,
                                     2.2f,
                                     bobZ + 14.f,
                                     bobZ + 30.f,
                                     outlineCol);
                    unitTopZ = bobZ + 45.f;
                }

                if (mAdmin.mPool.hasComponent<CHealth>(mask))
                {
                    const auto  health      = mAdmin.mPool.getComponent<CHealth>(entityId);
                    const float healthRatio = health->maxHp > 0
                                                  ? std::clamp(static_cast<float>(health->hp) /
                                                                   static_cast<float>(health->maxHp),
                                                               0.f,
                                                               1.f)
                                                  : 0.f;
                    const float barW        = isPlayerUnit ? 25.f : 19.f;
                    const float barD        = 3.0f;
                    const float barZ        = unitTopZ + 5.f;

                    if (health->armor > 0)
                    {
                        addBox3D(healthBars,
                                 nullptr,
                                 renderPos.x - barW * 0.5f,
                                 renderPos.y - barD * 0.5f,
                                 barZ + 5.5f,
                                 barW,
                                 barD,
                                 3.2f,
                                 sf::Color(255, 205, 50));
                    }

                    addBox3D(healthBars,
                             nullptr,
                             renderPos.x - barW * 0.5f,
                             renderPos.y - barD * 0.5f,
                             barZ,
                             barW,
                             barD,
                             3.4f,
                             sf::Color(28, 34, 44, 215));

                    if (healthRatio > 0.01f)
                    {
                        const sf::Color fillCol = isFriendlyUnit ? sf::Color(80, 195, 255) : sf::Color(255, 100, 100);
                        addBox3D(healthBars,
                                 nullptr,
                                 renderPos.x - barW * 0.5f,
                                 renderPos.y - barD * 0.5f,
                                 barZ + 0.2f,
                                 barW * healthRatio,
                                 barD + 0.3f,
                                 3.5f,
                                 fillCol);
                    }
                }
            }
        }


        if (input().build.placementMode)
        {
#if defined(_WIN32) || defined(WIN32) || defined(__WIN32__)
            const auto worldPos = input().mouseWorldPos;
            if (input().build.palette == PlacementPalette::Unit)
            {
                const auto      unitClass = input().build.selectedUnitClass;
                const float     radius    = static_cast<float>(GameStateDetail::getUnitRadius(unitClass));
                const sf::Color uCol      = ViewStateDetail::getUnitClassColor(unitClass);
                addPrismLUT3D(scene3DTriBatch,
                              worldPos,
                              radius * 1.15f,
                              radius * 1.05f,
                              0.f,
                              4.f,
                              kUnitPolygon8,
                              sf::Color(uCol.r, uCol.g, uCol.b, 180));
                addPrismLUT3D(scene3DTriBatch,
                              worldPos,
                              radius * 0.88f,
                              radius * 0.72f,
                              4.f,
                              38.f,
                              kUnitPolygon8,
                              sf::Color(uCol.r, uCol.g, uCol.b, 110));
            }
            else
            {
                if (input().build.selectedKind == WallKind::DesignatedPoint)
                {
                    addPrismLUT3D(scene3DTriBatch, worldPos, 18.f, 15.5f, 0.f, 6.f, kUnitPolygon8, sf::Color(255, 215, 35, 160));
                    addOctahedron3D(scene3DTriBatch, worldPos, 7.5f, 16.f, 26.f, 36.f, 0.f, sf::Color(255, 240, 130, 200));
                }
                else
                {
                    const auto      size     = GameStateDetail::getBuildWallSize(input().build.selectedKind);
                    const auto      pos      = worldPos - size * 0.5f;
                    const auto      wallData = GameStateDetail::createBuildWallData(input().build.selectedKind);
                    const sf::Color wCol     = ViewStateDetail::getWallColor(&wallData, false);
                    addBox3D(scene3DTriBatch,
                             &scene3DLineBatch,
                             pos.x,
                             pos.y,
                             0.f,
                             size.x,
                             size.y,
                             46.f,
                             sf::Color(wCol.r, wCol.g, wCol.b, 105),
                             sf::Color(255, 255, 255, 210));
                }
            }
#endif
        }
    }
    const auto rTScene1 = std::chrono::high_resolution_clock::now();
    if (debug().showPanel)
        mProfiler.renderTimes["4. Depth Scene (3D Mesh)"] = static_cast<float>(
            std::chrono::duration_cast<std::chrono::microseconds>(rTScene1 - rTScene0).count());

    const auto rTProj0 = std::chrono::high_resolution_clock::now();
    {
        PROFILE_SCOPE("Projectiles 3D");
        constexpr float kProjZ = 18.f;

        auto addBullet3D = [&](sf::Vector2f groundPos, float z, float radius, sf::Color color, bool whiteCore = false)
        {
            addOctahedron3D(bulletsBatched, groundPos, radius * 1.15f, z - radius, z, z + radius, mCurrentFrame * 0.15f, color);
            if (whiteCore)
            {
                addOctahedron3D(bulletsBatched,
                                groundPos,
                                radius * 0.65f,
                                z - radius * 0.6f,
                                z,
                                z + radius * 0.6f,
                                0.f,
                                sf::Color::White);
            }
        };

        const auto& projectiles     = mAdmin.mPool.viewComponent<CProjectile>();
        const auto& projectileIds   = mAdmin.mPool.viewIndex<CProjectile>();
        const int   projectileCount = static_cast<int>(projectiles.size());

        for (int i = 0; i < projectileCount; ++i)
        {
            const int   projectileId = projectileIds[i];
            const auto* projMask     = mAdmin.mPool.getMask(projectileId);

            if (!mAdmin.mPool.hasComponent<CPosition>(projMask))
                continue;

            const auto         position = mAdmin.mPool.getComponent<CPosition>(projectileId);
            const sf::Vector2f renderPos(static_cast<float>(position->pos.x), static_cast<float>(position->pos.y));

            if (renderPos.x + 35.f < camLeft || renderPos.x - 35.f > camRight || renderPos.y + 35.f < camTop ||
                renderPos.y - 35.f > camBottom)
            {
                continue;
            }

            const auto  projectile     = mAdmin.mPool.getComponent<CProjectile>(projectileId);
            const auto* ownerMask      = mAdmin.mPool.getMask(projectile->ownerId);
            const bool  isFriendlyProj = isFriendly(projectile->ownerId);
            const bool  isWallProj     = mAdmin.mPool.hasComponent<CWall>(ownerMask);
            const bool  isPlayerProj   = mAdmin.mPool.hasComponent<CPlayer>(ownerMask);
            const auto* ownerUnit      = mAdmin.mPool.hasComponent<CUnit>(ownerMask)
                                             ? &*mAdmin.mPool.getComponent<CUnit>(projectile->ownerId)
                                             : nullptr;

            sf::Vector2f projXY = renderPos;
            float        projZ  = kProjZ;
            sf::Vector2f aimDir(1.f, 0.f);
            float        maxTailLen = 15.f;

            if (mAdmin.mPool.hasComponent<CVelocity>(projMask))
            {
                const auto vel = mAdmin.mPool.getComponent<CVelocity>(projectileId);
                if (vel->vel.length() > 0.05f)
                {
                    aimDir = vel->vel.normalized();
                }
            }

            if (isWallProj)
            {
                constexpr float kTurretMuzzleDist = 31.f;
                constexpr float kTurretMuzzleZ    = 51.5f;

                sf::Vector2f wallCenter = renderPos;
                if (mAdmin.mPool.hasComponent<CRectDragDrop>(ownerMask))
                {
                    const auto wallRect = mAdmin.mPool.getComponent<CRectDragDrop>(projectile->ownerId);
                    wallCenter          = {static_cast<float>(wallRect->pos.x + wallRect->size.x * f32::Fix(1) / 2),
                                           static_cast<float>(wallRect->pos.y + wallRect->size.y * f32::Fix(1) / 2)};
                }

                float       prog       = 0.5f;
                const auto* targetMask = mAdmin.mPool.getMask(projectile->targetId);
                if (mAdmin.mPool.hasComponent<CPosition>(targetMask))
                {
                    const auto         tPos = mAdmin.mPool.getComponent<CPosition>(projectile->targetId);
                    const sf::Vector2f targetWorld(static_cast<float>(tPos->pos.x), static_cast<float>(tPos->pos.y));
                    const float totalDist = std::hypot(targetWorld.x - wallCenter.x, targetWorld.y - wallCenter.y);
                    const float curDist   = std::hypot(renderPos.x - wallCenter.x, renderPos.y - wallCenter.y);
                    if (totalDist > 1.f)
                    {
                        prog = std::clamp(curDist / totalDist, 0.f, 1.f);
                    }
                }

                projXY                     = renderPos + aimDir * (kTurretMuzzleDist * (1.f - prog));
                projZ                      = kTurretMuzzleZ + (kProjZ - kTurretMuzzleZ) * prog;
                const float distFromMuzzle = std::hypot(projXY.x - (wallCenter.x + aimDir.x * kTurretMuzzleDist),
                                                        projXY.y - (wallCenter.y + aimDir.y * kTurretMuzzleDist));
                maxTailLen                 = std::min(15.f, distFromMuzzle);
            }
            else if (mAdmin.mPool.hasComponent<CPosition>(ownerMask))
            {
                const auto         oPos = mAdmin.mPool.getComponent<CPosition>(projectile->ownerId);
                const sf::Vector2f ownerWorld(static_cast<float>(oPos->pos.x), static_cast<float>(oPos->pos.y));
                float              muzzleDist = 12.f;
                float              muzzleZ    = 22.f;

                if (isPlayerProj)
                {
                    muzzleDist = 11.f;
                    muzzleZ    = 20.f;
                }
                else if (ownerUnit)
                {
                    if (ownerUnit->unitClass == UnitClass::Tank)
                    {
                        muzzleDist = 20.f;
                        muzzleZ    = 31.f;
                    }
                    else
                    {
                        muzzleDist = 13.f;
                        muzzleZ    = 21.f;
                    }
                }

                float       prog       = 0.5f;
                const auto* targetMask = mAdmin.mPool.getMask(projectile->targetId);
                if (mAdmin.mPool.hasComponent<CPosition>(targetMask))
                {
                    const auto         tPos = mAdmin.mPool.getComponent<CPosition>(projectile->targetId);
                    const sf::Vector2f targetWorld(static_cast<float>(tPos->pos.x), static_cast<float>(tPos->pos.y));
                    const float totalDist = std::hypot(targetWorld.x - ownerWorld.x, targetWorld.y - ownerWorld.y);
                    const float curDist   = std::hypot(renderPos.x - ownerWorld.x, renderPos.y - ownerWorld.y);
                    if (totalDist > 1.f)
                    {
                        prog = std::clamp(curDist / totalDist, 0.f, 1.f);
                    }
                }

                projXY = renderPos + aimDir * (muzzleDist * (1.f - prog));
                projZ  = muzzleZ + (kProjZ - muzzleZ) * prog;
            }

            addGroundDisc3D(bulletsBatched, projXY, 0.8f, 2.8f, sf::Color(0, 0, 0, 75), sf::Color(0, 0, 0, 20), 6);

            if (isPlayerProj)
            {
                const int       cId       = getPlayerClientId(projectile->ownerId);
                const auto      palette   = ViewStateDetail::getPlayerPalette(cId);
                const sf::Color outerGlow = palette.projOuter;
                const sf::Color innerGlow = palette.projInner;

                addBullet3D(projXY, projZ, 5.5f, outerGlow, false);
                addBullet3D(projXY, projZ, 3.6f, innerGlow, true);

                if (mAdmin.mPool.hasComponent<CVelocity>(projMask))
                {
                    const auto vel = mAdmin.mPool.getComponent<CVelocity>(projectileId);
                    if (vel->vel.length() > 0.05f)
                    {
                        const sf::Vector2f tailPos = projXY - aimDir * 18.f;
                        addBillboardSegment3D(bulletsBatched,
                                              {projXY.x, projXY.y, projZ},
                                              {tailPos.x, tailPos.y, projZ},
                                              5.f,
                                              sf::Color(innerGlow.r, innerGlow.g, innerGlow.b, 200),
                                              sf::Color(outerGlow.r, outerGlow.g, outerGlow.b, 0));
                    }
                }
                continue;
            }

            const sf::Color color = isFriendlyProj ? sf::Color(0, 220, 255, 190) : sf::Color(255, 140, 0, 190);
            addBullet3D(projXY, projZ, 3.4f, color, mClientGame->mGame->mGraphicsLevel >= 1);

            if (mClientGame->mGame->mGraphicsLevel >= 1 && maxTailLen > 0.5f &&
                mAdmin.mPool.hasComponent<CVelocity>(projMask))
            {
                const auto vel = mAdmin.mPool.getComponent<CVelocity>(projectileId);
                if (vel->vel.length() > 0.05f)
                {
                    const sf::Vector2f tailPos = projXY - aimDir * maxTailLen;
                    const float        tailZ   = isWallProj ? std::min(51.5f, projZ + maxTailLen * 0.22f) : projZ;
                    addBillboardSegment3D(bulletsBatched,
                                          {projXY.x, projXY.y, projZ},
                                          {tailPos.x, tailPos.y, tailZ},
                                          3.8f,
                                          sf::Color(color.r, color.g, color.b, 180),
                                          sf::Color(color.r, color.g, color.b, 0));
                }
            }
        }
    }

    const auto rTProj1 = std::chrono::high_resolution_clock::now();
    if (debug().showPanel)
        mProfiler.renderTimes["5. Projectiles 3D"] = static_cast<float>(
            std::chrono::duration_cast<std::chrono::microseconds>(rTProj1 - rTProj0).count());

    const auto rTVfx0 = std::chrono::high_resolution_clock::now();
    {
        PROFILE_SCOPE("Visual Effects 3D");
        auto addHealCross3D = [&](sf::Vector2f groundPos, float worldZ, float armLen, float armThick, sf::Color color)
        {
            addBox3D(healBatched,
                     nullptr,
                     groundPos.x - armLen,
                     groundPos.y - armThick * 0.5f,
                     worldZ - armThick * 0.5f,
                     armLen * 2.f,
                     armThick,
                     armThick,
                     color);
            addBox3D(healBatched,
                     nullptr,
                     groundPos.x - armThick * 0.5f,
                     groundPos.y - armThick * 0.5f,
                     worldZ - armLen,
                     armThick,
                     armThick,
                     armLen * 2.f,
                     color);
        };

        auto addStarFlare3D = [&](sf::VertexArray& va, sf::Vector2f groundPos, float worldZ, float size, sf::Color color)
        {
            addOctahedron3D(va,
                            groundPos,
                            size * 0.55f,
                            worldZ - size * 0.65f,
                            worldZ,
                            worldZ + size * 0.65f,
                            mCurrentFrame * 0.2f,
                            color);
        };

        const auto& visionGrid    = mClientGame->gameState().nav().getVisionGrid(localTeamId());
        const int   visionSpacing = mClientGame->gameState().nav().visionSpacing;
        const bool  hasVisionGrid = visionSpacing > 0 && visionGrid.width() > 0 && visionGrid.height() > 0;

        auto isPosExplored = [&](const sf::Vector2f& pos) -> bool
        {
            if (!hasVisionGrid)
                return true;
            if (pos.x < 0.f || pos.y < 0.f)
                return false;
            const int cx = static_cast<int>(pos.x) / visionSpacing;
            const int cy = static_cast<int>(pos.y) / visionSpacing;
            return visionGrid.isValidCell(cx, cy) && visionGrid[cx][cy] != 0;
        };

        for (const int effectId : mAdmin.mPool.viewIndex<CVisualEffect>())
        {
            const auto         effect   = mAdmin.mPool.getComponent<CVisualEffect>(effectId);
            const sf::Vector2f startPos = effect->startPos;
            const sf::Vector2f endPos   = effect->endPos;
            float              minX     = startPos.x;
            float              maxX     = startPos.x;
            float              minY     = startPos.y;
            float              maxY     = startPos.y;

            if (effect->kind == EffectKind::ElectricBeam || effect->kind == EffectKind::FireBeam ||
                effect->kind == EffectKind::LaserBeam)
            {
                minX = std::min(minX, endPos.x) - 25.f;
                maxX = std::max(maxX, endPos.x) + 25.f;
                minY = std::min(minY, endPos.y) - 25.f;
                maxY = std::max(maxY, endPos.y) + 25.f;
            }
            else if (effect->kind == EffectKind::Heal)
            {
                if (endPos != sf::Vector2f{0.f, 0.f})
                {
                    minX = std::min(minX, endPos.x) - 35.f;
                    maxX = std::max(maxX, endPos.x) + 35.f;
                    minY = std::min(minY, endPos.y) - 35.f;
                    maxY = std::max(maxY, endPos.y) + 35.f;
                }
                else
                {
                    minX -= 35.f;
                    maxX += 35.f;
                    minY -= 60.f;
                    maxY += 35.f;
                }
            }
            else
            {
                float maxR = 85.f;
                if (effect->kind == EffectKind::CollectPulse)
                    maxR = 45.f;
                else if (effect->kind == EffectKind::Shockwave)
                    maxR = 65.f;
                else if (effect->kind == EffectKind::SlashArc)
                    maxR = 140.f;
                else if (effect->kind == EffectKind::RadarScan)
                    maxR = 310.f;

                minX -= maxR;
                maxX += maxR;
                minY -= maxR;
                maxY += maxR;
            }

            if (maxX < camLeft || minX > camRight || maxY < camTop || minY > camBottom)
            {
                continue;
            }

            if (effect->kind == EffectKind::ElectricBeam || effect->kind == EffectKind::FireBeam ||
                effect->kind == EffectKind::LaserBeam)
            {
                if (!isPosExplored(effect->startPos) && !isPosExplored(effect->endPos) &&
                    !isPosExplored((effect->startPos + effect->endPos) * 0.5f))
                {
                    continue;
                }
            }
            else if (effect->kind == EffectKind::Heal)
            {
                if (!isPosExplored(effect->startPos) &&
                    (effect->endPos == sf::Vector2f{0.f, 0.f} || !isPosExplored(effect->endPos)))
                {
                    continue;
                }
            }
            else if (effect->kind != EffectKind::ClickIndicator)
            {
                if (!isPosExplored(effect->startPos))
                {
                    float checkR = 0.f;
                    if (effect->kind == EffectKind::DeathExplosion || effect->kind == EffectKind::Shockwave)
                        checkR = 40.f;

                    if (checkR == 0.f || (!isPosExplored(effect->startPos + sf::Vector2f(checkR, 0.f)) &&
                                          !isPosExplored(effect->startPos - sf::Vector2f(checkR, 0.f)) &&
                                          !isPosExplored(effect->startPos + sf::Vector2f(0.f, checkR)) &&
                                          !isPosExplored(effect->startPos - sf::Vector2f(0.f, checkR))))
                    {
                        continue;
                    }
                }
            }

            const float alpha = static_cast<float>(effect->lifeTime) / static_cast<float>(effect->initialLifeTime);

            if (effect->kind == EffectKind::ElectricBeam || effect->kind == EffectKind::FireBeam ||
                effect->kind == EffectKind::LaserBeam)
            {
                sf::Vector2f direction = effect->endPos - effect->startPos;
                const float  length    = std::hypot(direction.x, direction.y);
                if (length > 0.01f)
                {
                    direction /= length;
                    const sf::Vector2f normal(-direction.y, direction.x);

                    sf::Vector2f emitStartXY = effect->startPos;
                    float        startZ      = 44.f;
                    const float  endZ        = 16.f;
                    bool         isFromBase  = false;

                    if (effect->kind == EffectKind::ElectricBeam)
                    {
                        for (const int baseId : mAdmin.mPool.viewIndex<CBase>())
                        {
                            const auto* bMask = mAdmin.mPool.getMask(baseId);
                            if (mAdmin.mPool.hasComponent<CRectDragDrop>(bMask))
                            {
                                const auto bRect = mAdmin.mPool.getComponent<CRectDragDrop>(baseId);
                                const sf::Vector2f bCenter(static_cast<float>(bRect->pos.x + bRect->size.x * f32::Fix(1) / 2),
                                                           static_cast<float>(bRect->pos.y + bRect->size.y * f32::Fix(1) / 2));
                                if (std::hypot(bCenter.x - effect->startPos.x, bCenter.y - effect->startPos.y) < 8.f)
                                {
                                    isFromBase             = true;
                                    const bool  isMainBase = static_cast<float>(bRect->size.x) > 120.f;
                                    const float tier2Z     = isMainBase ? 74.f : 56.f;
                                    startZ                 = tier2Z + 44.f + std::sin(mCurrentFrame * 0.07f) * 5.5f;
                                    break;
                                }
                            }
                        }

                        if (!isFromBase)
                        {
                            emitStartXY = effect->startPos + direction * 22.f;
                            startZ      = 55.5f;
                        }
                    }
                    else if (effect->kind == EffectKind::FireBeam)
                    {
                        emitStartXY = effect->startPos + direction * 14.f;
                        startZ      = 74.f;
                    }
                    else
                    {
                        startZ = 51.5f;
                    }

                    sf::Color outerColor;
                    sf::Color innerColor;
                    float     thickness = 4.f;

                    if (effect->kind == EffectKind::ElectricBeam)
                    {
                        outerColor = sf::Color(100, 200, 255, static_cast<std::uint8_t>(255 * alpha));
                        innerColor = sf::Color(255, 255, 255, static_cast<std::uint8_t>(255 * alpha));
                        thickness  = 4.2f;
                    }
                    else if (effect->kind == EffectKind::FireBeam)
                    {
                        outerColor = sf::Color(255, 100, 20, static_cast<std::uint8_t>(210 * alpha));
                        innerColor = sf::Color(255, 215, 60, static_cast<std::uint8_t>(255 * alpha));
                        thickness  = 6.5f;
                    }
                    else
                    {
                        outerColor = sf::Color(255, 50, 50, static_cast<std::uint8_t>(210 * alpha));
                        innerColor = sf::Color(255, 255, 210, static_cast<std::uint8_t>(255 * alpha));
                        thickness  = 3.4f;
                    }

                    addBillboardSegment3D(beamsBatched,
                                          {emitStartXY.x, emitStartXY.y, 1.f},
                                          {effect->endPos.x, effect->endPos.y, 1.f},
                                          thickness * 1.4f,
                                          sf::Color(outerColor.r,
                                                    outerColor.g,
                                                    outerColor.b,
                                                    static_cast<uint8_t>(outerColor.a * 0.25f)),
                                          sf::Color(outerColor.r,
                                                    outerColor.g,
                                                    outerColor.b,
                                                    static_cast<uint8_t>(outerColor.a * 0.25f)));

                    if (effect->kind == EffectKind::ElectricBeam && mClientGame->mGame->mGraphicsLevel >= 1)
                    {
                        if (!isFromBase)
                        {
                            const float        apexZ = 90.f + std::sin(mCurrentFrame * 0.2f) * 3.5f;
                            const sf::Vector3f pApex{effect->startPos.x, effect->startPos.y, apexZ};
                            const sf::Vector3f pProng{emitStartXY.x, emitStartXY.y, startZ};
                            addBillboardSegment3D(beamsBatched, pApex, pProng, thickness * 1.15f, outerColor, outerColor);
                            addBillboardSegment3D(beamsBatched, pApex, pProng, thickness * 0.45f, innerColor, innerColor);
                        }

                        sf::Vector3f nodes[5];
                        nodes[0] = {emitStartXY.x, emitStartXY.y, startZ};
                        nodes[4] = {effect->endPos.x, effect->endPos.y, endZ};
                        for (int j = 1; j < 4; ++j)
                        {
                            const float ratio = j * 0.25f;
                            const float offXY = static_cast<float>(mRenderRandom.randIntRange(-12, 12));
                            const float offZ  = static_cast<float>(mRenderRandom.randIntRange(-8, 8));
                            const sf::Vector2f gPt = emitStartXY + (effect->endPos - emitStartXY) * ratio + normal * offXY;
                            const float zPt = startZ + (endZ - startZ) * ratio + offZ;
                            nodes[j]        = {gPt.x, gPt.y, zPt};
                        }

                        for (int j = 0; j < 4; ++j)
                        {
                            addBillboardSegment3D(beamsBatched, nodes[j], nodes[j + 1], thickness * 1.4f, outerColor, outerColor);
                            addBillboardSegment3D(beamsBatched, nodes[j], nodes[j + 1], thickness * 0.5f, innerColor, innerColor);
                        }

                        if (mClientGame->mGame->mGraphicsLevel == 2)
                        {
                            for (int j = 1; j < 3; ++j)
                            {
                                const float        sideSign = (j % 2 == 0) ? 1.f : -1.f;
                                const sf::Vector2f bEndXY   = sf::Vector2f(nodes[j].x, nodes[j].y) + direction * 32.f +
                                                              normal * (18.f * sideSign);
                                const sf::Vector3f bEnd3D{bEndXY.x, bEndXY.y, nodes[j].z - 6.f};
                                const sf::Color    bCol(outerColor.r,
                                                        outerColor.g,
                                                        outerColor.b,
                                                        static_cast<uint8_t>(outerColor.a * 0.65f));
                                addBillboardSegment3D(beamsBatched,
                                                      nodes[j],
                                                      bEnd3D,
                                                      thickness * 0.6f,
                                                      bCol,
                                                      sf::Color(bCol.r, bCol.g, bCol.b, 0));
                            }
                            addStarFlare3D(beamsBatched, emitStartXY, startZ, 14.f * alpha, sf::Color::White);
                            addStarFlare3D(beamsBatched, effect->endPos, endZ, 18.f * alpha, sf::Color::White);
                        }
                    }
                    else if (effect->kind == EffectKind::FireBeam && mClientGame->mGame->mGraphicsLevel >= 1)
                    {
                        const float time  = mCurrentFrame * 0.1f;
                        const int   count = (mClientGame->mGame->mGraphicsLevel == 2) ? 6 : 4;
                        for (int j = 0; j < count; ++j)
                        {
                            const float ratio  = static_cast<float>(j) / static_cast<float>(count - 1);
                            const float wiggle = std::sin(time * 14.f + j * 1.6f) * 5.f;
                            const sf::Vector2f pos = emitStartXY + (effect->endPos - emitStartXY) * ratio + normal * wiggle;
                            const float     zPos = startZ + (endZ - startZ) * ratio;
                            const float     r    = 5.f + ratio * 12.f;
                            const sf::Color fCol(255,
                                                 static_cast<uint8_t>(85 + ratio * 135),
                                                 20,
                                                 static_cast<uint8_t>(180 * alpha * (1.f - ratio * 0.25f)));
                            addOctahedron3D(beamsBatched, pos, r, zPos - r * 0.7f, zPos, zPos + r * 0.9f, time + j, fCol);

                            const sf::Color hotCore(255, 255, 175, static_cast<uint8_t>(235 * alpha * (1.f - ratio)));
                            addOctahedron3D(beamsBatched, pos, r * 0.48f, zPos - r * 0.35f, zPos, zPos + r * 0.45f, -time, hotCore);
                        }
                    }
                    else
                    {
                        const sf::Vector3f pStart{emitStartXY.x, emitStartXY.y, startZ};
                        const sf::Vector3f pEnd{effect->endPos.x, effect->endPos.y, endZ};
                        addBillboardSegment3D(beamsBatched,
                                              pStart,
                                              pEnd,
                                              thickness * 2.2f,
                                              sf::Color(outerColor.r, outerColor.g, outerColor.b, static_cast<uint8_t>(90 * alpha)),
                                              sf::Color(outerColor.r,
                                                        outerColor.g,
                                                        outerColor.b,
                                                        static_cast<uint8_t>(90 * alpha)));
                        addBillboardSegment3D(beamsBatched, pStart, pEnd, thickness * 1.2f, outerColor, outerColor);
                        addBillboardSegment3D(beamsBatched, pStart, pEnd, thickness * 0.45f, innerColor, innerColor);

                        if (mClientGame->mGame->mGraphicsLevel == 2)
                        {
                            addStarFlare3D(beamsBatched, emitStartXY, startZ, 18.f * alpha, sf::Color::White);
                            addStarFlare3D(beamsBatched, effect->endPos, endZ, 18.f * alpha, sf::Color::White);
                        }
                    }
                }
            }
            else if (effect->kind == EffectKind::DeathExplosion || effect->kind == EffectKind::CollectPulse ||
                     effect->kind == EffectKind::Shockwave || effect->kind == EffectKind::RadarScan)
            {
                float maxRadius = 30.f;
                if (effect->kind == EffectKind::CollectPulse)
                    maxRadius = 40.f;
                else if (effect->kind == EffectKind::Shockwave)
                    maxRadius = 60.f;
                else if (effect->kind == EffectKind::RadarScan)
                    maxRadius = 300.f;
                else if (effect->kind == EffectKind::DeathExplosion)
                    maxRadius = 80.f;

                const float radius = maxRadius * (1.f - alpha);

                if (effect->kind == EffectKind::DeathExplosion)
                {
                    const bool      isFriendly     = (effect->teamId == localTeamId());
                    const sf::Color explosionColor = isFriendly ? sf::Color(50, 255, 100) : sf::Color(255, 65, 50);

                    addGroundRing3D(ringsBatched,
                                    effect->startPos,
                                    2.f,
                                    radius,
                                    3.5f * alpha,
                                    sf::Color(explosionColor.r,
                                              explosionColor.g,
                                              explosionColor.b,
                                              static_cast<uint8_t>(220 * alpha)),
                                    24);

                    if (mClientGame->mGame->mGraphicsLevel >= 1)
                    {
                        const float domeR = radius * 0.42f;
                        const float domeZ = 6.f + (1.f - alpha) * 18.f;
                        addOctahedron3D(ringsBatched,
                                        effect->startPos,
                                        domeR,
                                        2.f,
                                        domeZ,
                                        domeZ + domeR * 1.2f,
                                        alpha * 3.f,
                                        sf::Color(255, 255, 255, static_cast<uint8_t>(235 * alpha)));
                    }

                    if (mClientGame->mGame->mGraphicsLevel == 2)
                    {
                        constexpr int shardCount = 8;
                        const float   travel     = maxRadius * 1.15f * (1.f - alpha);
                        const float   parabolicZ = 4.f + 46.f * 4.f * alpha * (1.f - alpha);
                        for (int j = 0; j < shardCount; ++j)
                        {
                            const float ang = j * 2.f * std::numbers::pi_v<float> / shardCount + (alpha * 0.6f);
                            const sf::Vector2f sPos = effect->startPos + sf::Vector2f(std::cos(ang), std::sin(ang)) * travel;
                            const float     size = 3.8f * alpha;
                            const sf::Color sCol(explosionColor.r,
                                                 explosionColor.g,
                                                 explosionColor.b,
                                                 static_cast<uint8_t>(255 * alpha));
                            addOctahedron3D(shards, sPos, size, parabolicZ - size, parabolicZ, parabolicZ + size, ang, sCol);
                        }
                    }
                }
                else if (effect->kind == EffectKind::CollectPulse)
                {
                    addGroundRing3D(ringsBatched,
                                    effect->startPos,
                                    2.f,
                                    radius,
                                    2.6f * alpha,
                                    sf::Color(50, 255, 100, static_cast<uint8_t>(205 * alpha)),
                                    20);

                    if (mClientGame->mGame->mGraphicsLevel == 2)
                    {
                        constexpr int starsCount = 6;
                        const float   travel     = maxRadius * 0.85f * (1.f - alpha);
                        const float   starZ      = 8.f + (1.f - alpha) * 32.f;
                        for (int j = 0; j < starsCount; ++j)
                        {
                            const float ang = j * 2.f * std::numbers::pi_v<float> / starsCount - (alpha * 1.5f);
                            const sf::Vector2f sPos = effect->startPos + sf::Vector2f(std::cos(ang), std::sin(ang)) * travel;
                            addStarFlare3D(ringsBatched, sPos, starZ, 9.f * alpha, sf::Color::White);
                        }
                    }
                }
                else if (effect->kind == EffectKind::Shockwave || effect->kind == EffectKind::RadarScan)
                {
                }
            }
            else if (effect->kind == EffectKind::Heal)
            {
                const sf::Vector2f unitPos    = effect->startPos;
                const float        crossZ     = 42.f + (1.f - alpha) * 26.f;
                const float        crossSize  = 8.f * (0.65f + 0.35f * alpha);
                const float        crossThick = 2.4f;

                if (mClientGame->mGame->mGraphicsLevel >= 2)
                {
                    addHealCross3D(unitPos,
                                   crossZ,
                                   crossSize + 1.2f,
                                   crossThick + 1.6f,
                                   sf::Color(30, 200, 90, static_cast<uint8_t>(95 * alpha)));
                }
                addHealCross3D(unitPos, crossZ, crossSize, crossThick, sf::Color(60, 255, 120, static_cast<uint8_t>(230 * alpha)));
                if (mClientGame->mGame->mGraphicsLevel >= 1)
                {
                    addHealCross3D(unitPos,
                                   crossZ,
                                   crossSize * 0.6f,
                                   crossThick * 0.5f,
                                   sf::Color(240, 255, 240, static_cast<uint8_t>(250 * alpha)));
                }

                const float auraR1 = 10.f + (1.f - alpha) * 16.f;
                addGroundRing3D(healBatched,
                                unitPos,
                                1.5f,
                                auraR1,
                                1.8f,
                                sf::Color(60, 255, 120, static_cast<uint8_t>(140 * alpha)),
                                16);
                if (mClientGame->mGame->mGraphicsLevel >= 1)
                {
                    const float auraR2 = 6.f + (1.f - alpha) * 9.f;
                    addGroundRing3D(healBatched,
                                    unitPos,
                                    2.f,
                                    auraR2,
                                    1.2f,
                                    sf::Color(140, 255, 180, static_cast<uint8_t>(95 * alpha)),
                                    12);
                }

                if (effect->endPos != sf::Vector2f{0.f, 0.f})
                {
                    const sf::Vector2f wallPos = effect->endPos;
                    const sf::Vector2f diff    = unitPos - wallPos;
                    const float        dist    = std::hypot(diff.x, diff.y);
                    if (dist > 2.f)
                    {
                        const sf::Vector2f u = diff / dist;
                        const sf::Vector2f n(-u.y, u.x);

                        const int       N         = (mClientGame->mGame->mGraphicsLevel == 2) ? 20 : 10;
                        const float     frameTime = static_cast<float>(mCurrentFrame);
                        constexpr float kWallZ    = 81.f;
                        constexpr float kUnitZ    = 20.f;

                        sf::Vector3f prev1{wallPos.x, wallPos.y, kWallZ};
                        sf::Vector3f prev2 = prev1;

                        for (int i = 1; i <= N; ++i)
                        {
                            const float        t        = static_cast<float>(i) / static_cast<float>(N);
                            const sf::Vector2f pBase    = wallPos + diff * t;
                            const float        envelope = std::sin(t * std::numbers::pi_v<float>);
                            const float        archZ    = kWallZ + (kUnitZ - kWallZ) * t + envelope * 26.f;

                            const float        phase1 = t * (3.5f * std::numbers::pi_v<float>)-frameTime * 0.15f;
                            const float        wave1  = std::sin(phase1) * (8.5f * envelope);
                            const sf::Vector2f xy1    = pBase + n * wave1;
                            const sf::Vector3f cur1{xy1.x, xy1.y, archZ + std::cos(phase1) * (4.f * envelope)};

                            addBillboardSegment3D(healBatched,
                                                  prev1,
                                                  cur1,
                                                  2.8f,
                                                  sf::Color(40, 240, 110, static_cast<uint8_t>(85 * alpha)),
                                                  sf::Color(40, 240, 110, static_cast<uint8_t>(85 * alpha)));
                            addBillboardSegment3D(healBatched,
                                                  prev1,
                                                  cur1,
                                                  1.3f,
                                                  sf::Color(195, 255, 220, static_cast<uint8_t>(225 * alpha)),
                                                  sf::Color(195, 255, 220, static_cast<uint8_t>(225 * alpha)));
                            prev1 = cur1;

                            if (mClientGame->mGame->mGraphicsLevel == 2)
                            {
                                const float        phase2 = phase1 + std::numbers::pi_v<float>;
                                const float        wave2  = std::sin(phase2) * (5.5f * envelope);
                                const sf::Vector2f xy2    = pBase + n * wave2;
                                const sf::Vector3f cur2{xy2.x, xy2.y, archZ + std::cos(phase2) * (3.f * envelope)};
                                addBillboardSegment3D(healBatched,
                                                      prev2,
                                                      cur2,
                                                      1.0f,
                                                      sf::Color(70, 255, 140, static_cast<uint8_t>(115 * alpha)),
                                                      sf::Color(70, 255, 140, static_cast<uint8_t>(115 * alpha)));
                                prev2 = cur2;
                            }
                        }

                        const int moteCount = (mClientGame->mGame->mGraphicsLevel == 2) ? 3 : 1;
                        for (int m = 0; m < moteCount; ++m)
                        {
                            const float moteProgress = std::fmod(frameTime * 0.032f + static_cast<float>(m) * 0.333f, 1.0f);
                            const sf::Vector2f moteBase = wallPos + diff * moteProgress;
                            const float        moteEnv  = std::sin(moteProgress * std::numbers::pi_v<float>);
                            const float        moteZ    = kWallZ + (kUnitZ - kWallZ) * moteProgress + moteEnv * 26.f;
                            const float motePhase = moteProgress * (3.5f * std::numbers::pi_v<float>)-frameTime * 0.15f;
                            const sf::Vector2f moteXY = moteBase + n * (std::sin(motePhase) * (8.5f * moteEnv));

                            addHealCross3D(moteXY,
                                           moteZ,
                                           2.6f,
                                           1.2f,
                                           sf::Color(230, 255, 240, static_cast<uint8_t>(240 * alpha * moteEnv)));
                        }
                    }
                }
            }
            else if (effect->kind == EffectKind::ClickIndicator)
            {
                if (effect->teamId != localTeamId() && !isPosExplored(effect->startPos))
                {
                    continue;
                }
                const int     style = static_cast<int>(effect->endPos.x);
                sf::Color     col   = (style == 3) ? sf::Color(255, 120, 80) : sf::Color(60, 255, 120);
                const uint8_t a     = static_cast<uint8_t>(std::clamp(255.f * alpha, 0.f, 255.f));
                col.a               = a;

                const float maxRadius     = 24.f;
                const float currentRadius = maxRadius * (1.15f - 0.35f * (1.f - alpha));
                addGroundRing3D(ringsBatched, effect->startPos, 1.5f, currentRadius, 2.5f * alpha, col, 18);

                const float tickLen   = 7.f * alpha;
                const float tickThick = 2.f * alpha;
                const float dist      = currentRadius + 3.f;
                addBillboardSegment3D(ringsBatched,
                                      {effect->startPos.x, effect->startPos.y - dist - tickLen, 1.8f},
                                      {effect->startPos.x, effect->startPos.y - dist, 1.8f},
                                      tickThick,
                                      col,
                                      col);
                addBillboardSegment3D(ringsBatched,
                                      {effect->startPos.x, effect->startPos.y + dist, 1.8f},
                                      {effect->startPos.x, effect->startPos.y + dist + tickLen, 1.8f},
                                      tickThick,
                                      col,
                                      col);
                addBillboardSegment3D(ringsBatched,
                                      {effect->startPos.x - dist - tickLen, effect->startPos.y, 1.8f},
                                      {effect->startPos.x - dist, effect->startPos.y, 1.8f},
                                      tickThick,
                                      col,
                                      col);
                addBillboardSegment3D(ringsBatched,
                                      {effect->startPos.x + dist, effect->startPos.y, 1.8f},
                                      {effect->startPos.x + dist + tickLen, effect->startPos.y, 1.8f},
                                      tickThick,
                                      col,
                                      col);

                const float beaconZ = 4.f + alpha * 10.f;
                addOctahedron3D(sparks,
                                effect->startPos,
                                4.2f * alpha,
                                beaconZ - 4.f * alpha,
                                beaconZ,
                                beaconZ + 6.f * alpha,
                                (1.f - alpha) * 3.f,
                                sf::Color(255, 255, 255, static_cast<uint8_t>(240 * alpha)));
            }
            else if (effect->kind == EffectKind::SlashArc)
            {
                const sf::Vector2f center   = effect->startPos;
                const sf::Vector2f dirVec   = effect->endPos - effect->startPos;
                const float        dirLen   = std::hypot(dirVec.x, dirVec.y);
                const float        scale    = std::clamp(dirLen / 120.f, 0.4f, 1.0f);
                const float        baseAng  = std::atan2(dirVec.y, dirVec.x);
                const float        progress = 1.f - alpha;

                constexpr float arcSpan  = 2.44f;
                const float     halfSpan = arcSpan * 0.5f;

                const float rInner = (28.f + progress * 22.f) * scale;
                const float rOuter = (82.f + progress * 38.f) * scale;
                const float slashZ = 8.f + progress * 8.f;

                const bool      isFriendly = (effect->teamId == localTeamId());
                const sf::Color coreColor  = isFriendly ? sf::Color(80, 220, 255, static_cast<uint8_t>(255 * alpha))
                                                        : sf::Color(255, 130, 50, static_cast<uint8_t>(255 * alpha));
                const sf::Color edgeColor  = isFriendly ? sf::Color(230, 255, 255, static_cast<uint8_t>(220 * alpha))
                                                        : sf::Color(255, 240, 180, static_cast<uint8_t>(220 * alpha));
                const sf::Color fadeColor  = sf::Color(coreColor.r, coreColor.g, coreColor.b, 0);

                constexpr int kArcSegs = 16;
                for (int s = 0; s < kArcSegs; ++s)
                {
                    const float t0 = static_cast<float>(s) / static_cast<float>(kArcSegs);
                    const float t1 = static_cast<float>(s + 1) / static_cast<float>(kArcSegs);

                    const float edgeFactor0 = 1.f - std::abs(t0 * 2.f - 1.f);
                    const float edgeFactor1 = 1.f - std::abs(t1 * 2.f - 1.f);

                    const float a0 = baseAng - halfSpan + t0 * arcSpan;
                    const float a1 = baseAng - halfSpan + t1 * arcSpan;

                    const sf::Vector2f pIn0  = center + sf::Vector2f(std::cos(a0), std::sin(a0)) *
                                                            (rInner + (1.f - edgeFactor0) * 12.f);
                    const sf::Vector2f pOut0 = center + sf::Vector2f(std::cos(a0), std::sin(a0)) *
                                                            (rOuter * (0.75f + 0.25f * edgeFactor0));
                    const sf::Vector2f pIn1  = center + sf::Vector2f(std::cos(a1), std::sin(a1)) *
                                                            (rInner + (1.f - edgeFactor1) * 12.f);
                    const sf::Vector2f pOut1 = center + sf::Vector2f(std::cos(a1), std::sin(a1)) *
                                                            (rOuter * (0.75f + 0.25f * edgeFactor1));

                    const sf::Color c0 = (edgeFactor0 > 0.45f) ? edgeColor : coreColor;
                    const sf::Color c1 = (edgeFactor1 > 0.45f) ? edgeColor : coreColor;

                    vfxBatched.append(sf::Vertex(cam.project(pIn0, slashZ), fadeColor));
                    vfxBatched.append(sf::Vertex(cam.project(pOut0, slashZ + 2.f), c0));
                    vfxBatched.append(sf::Vertex(cam.project(pOut1, slashZ + 2.f), c1));

                    vfxBatched.append(sf::Vertex(cam.project(pOut1, slashZ + 2.f), c1));
                    vfxBatched.append(sf::Vertex(cam.project(pIn1, slashZ), fadeColor));
                    vfxBatched.append(sf::Vertex(cam.project(pIn0, slashZ), fadeColor));
                }
            }
        }
    }

    const auto rTVfx1 = std::chrono::high_resolution_clock::now();
    if (debug().showPanel)
        mProfiler.renderTimes["6. Beams & VFX 3D"] = static_cast<float>(
            std::chrono::duration_cast<std::chrono::microseconds>(rTVfx1 - rTVfx0).count());

    const auto rTGpu0 = std::chrono::high_resolution_clock::now();
    {
        PROFILE_SCOPE("Draw Batched 3D Meshes (GPU)");
        if (groundRangeBatch.getVertexCount() > 0)
            window().draw(groundRangeBatch);

        if (scene3DTriBatch.getVertexCount() > 0)
            window().draw(scene3DTriBatch);
        if (scene3DLineBatch.getVertexCount() > 0)
            window().draw(scene3DLineBatch);

        if (vfxBatched.getVertexCount() > 0)
            window().draw(vfxBatched);

        if (healthBars.getVertexCount() > 0)
            window().draw(healthBars);
    }

    const auto rTGpu1 = std::chrono::high_resolution_clock::now();
    if (debug().showPanel)
        mProfiler.renderTimes["7. GPU Mesh Flush"] = static_cast<float>(
            std::chrono::duration_cast<std::chrono::microseconds>(rTGpu1 - rTGpu0).count());

    const auto rTSel0 = std::chrono::high_resolution_clock::now();
    {
        PROFILE_SCOPE("Selection 3D & World Debug");
        ViewStateDetail::drawWorldDebug(this, &this->mClientGame->gameState());

        static sf::VertexArray selection3DTri(sf::PrimitiveType::Triangles);
        static sf::VertexArray selection3DLines(sf::PrimitiveType::Lines);
        selection3DTri.clear();
        selection3DLines.clear();

        if (input().isSelecting)
        {
            const float x0 = std::min(input().selectionStart.x, input().selectionEnd.x);
            const float y0 = std::min(input().selectionStart.y, input().selectionEnd.y);
            const float x1 = std::max(input().selectionStart.x, input().selectionEnd.x);
            const float y1 = std::max(input().selectionStart.y, input().selectionEnd.y);

            addBox3D(selection3DTri,
                     &selection3DLines,
                     x0,
                     y0,
                     1.f,
                     std::max(1.f, x1 - x0),
                     std::max(1.f, y1 - y0),
                     16.f,
                     sf::Color(80, 195, 255, 42),
                     sf::Color(110, 235, 255, 225));
        }

        for (int id : mAdmin.mPool.viewIndex<CUnit>())
        {
            const auto unit = mAdmin.mPool.getComponent<CUnit>(id);
            if (!unit->isSelected || !isFriendly(id))
            {
                continue;
            }
            const auto* mask = mAdmin.mPool.getMask(id);
            if (mAdmin.mPool.hasComponent<CPosition>(mask) && mAdmin.mPool.hasComponent<CBoundCircle>(mask))
            {
                const auto         circle   = mAdmin.mPool.getComponent<CBoundCircle>(id);
                const auto         position = mAdmin.mPool.getComponent<CPosition>(id);
                const sf::Vector2f renderPos(static_cast<float>(position->pos.x), static_cast<float>(position->pos.y));

                const float r = circle->radius + 15.f;
                if (renderPos.x + r < camLeft || renderPos.x - r > camRight || renderPos.y + r < camTop ||
                    renderPos.y - r > camBottom)
                {
                    continue;
                }

                const float selR = circle->radius + 9.f;
                addPrismLUT3D(selection3DTri,
                              renderPos,
                              selR,
                              selR * 0.94f,
                              1.f,
                              4.5f,
                              kUnitPolygon8,
                              sf::Color(90, 215, 255, 165),
                              false);

                for (int c = 0; c < 4; ++c)
                {
                    const float        ang = mCurrentFrame * 0.05f + c * 0.5f * std::numbers::pi_v<float>;
                    const sf::Vector2f bPt = renderPos + sf::Vector2f(std::cos(ang), std::sin(ang)) * selR;
                    addOctahedron3D(selection3DTri, bPt, 3.2f, 5.f, 9.5f, 14.f, mCurrentFrame * 0.12f, sf::Color(140, 240, 255, 235));
                }
            }
        }

        if (selection3DTri.getVertexCount() > 0)
            window().draw(selection3DTri);
        if (selection3DLines.getVertexCount() > 0)
            window().draw(selection3DLines);
    }
    const auto rTSel1 = std::chrono::high_resolution_clock::now();
    if (debug().showPanel)
        mProfiler.renderTimes["8. Selection & Debug"] = static_cast<float>(
            std::chrono::duration_cast<std::chrono::microseconds>(rTSel1 - rTSel0).count());

    int localFriendlyCount = 0;
    int localOpponentCount = 0;
    for (int uid : unitIds)
    {
        if (isFriendly(uid))
            localFriendlyCount++;
        else
            localOpponentCount++;
    }
    mProfiler.friendlyCount = localFriendlyCount;
    mProfiler.opponentCount = localOpponentCount;
    mProfiler.projCount     = static_cast<int>(mAdmin.mPool.viewIndex<CProjectile>().size());
    mProfiler.vfxCount      = static_cast<int>(mAdmin.mPool.viewIndex<CVisualEffect>().size());
}

void ViewState::doAction(const Action& action)
{
    const auto& gameState = mClientGame->gameState();

    switch (action.type)
    {
        case Action::Type::Back:
            mClientGame->mGame->popScene();
            break;
        case Action::Type::ExitToMenu:
            mClientGame->mGame->changeScene("Menu");
            break;
        case Action::Type::Debug:
            debug().showPanel = !debug().showPanel;
            break;
        case Action::Type::Select:
            input().inputMode = InputMode::Select;
            break;
        case Action::Type::DragDrop:
            input().inputMode = InputMode::DragDrop;
            break;
        case Action::Type::ToggleInputMode:
            input().inputMode = (input().inputMode == InputMode::DragDrop) ? InputMode::Select : InputMode::DragDrop;
            break;
        case Action::Type::Speed:
            if (std::holds_alternative<float>(action.data))
            {
                float newScale = std::get<float>(action.data);
                send(Command(CommandType::SetGameSpeed, CmdSetGameSpeed{.timeScale = newScale}));
            }
            break;
        case Action::Type::TogglePlacementPalette:
            input().build.palette       = input().build.palette == PlacementPalette::Wall ? PlacementPalette::Unit
                                                                                          : PlacementPalette::Wall;
            input().build.placementMode = false;
            break;
        case Action::Type::SelectWallKind:
            if (std::holds_alternative<WallKind>(action.data))
            {
                const WallKind kind = std::get<WallKind>(action.data);
                const bool wasSelected = input().build.placementMode && input().build.palette == PlacementPalette::Wall &&
                                         input().build.selectedKind == kind;
                input().build.palette       = PlacementPalette::Wall;
                input().build.selectedKind  = kind;
                input().build.placementMode = !wasSelected;
            }
            break;
        case Action::Type::SelectUnitClass:
            if (std::holds_alternative<UnitClass>(action.data))
            {
                const UnitClass unitClass = std::get<UnitClass>(action.data);
                const bool wasSelected = input().build.placementMode && input().build.palette == PlacementPalette::Unit &&
                                         input().build.selectedUnitClass == unitClass;
                input().build.palette           = PlacementPalette::Unit;
                input().build.selectedUnitClass = unitClass;
                input().build.placementMode     = !wasSelected;
            }
            break;
        case Action::Type::EventKeyPressed:
        {
            if (std::holds_alternative<KeyPressedData>(action.data))
            {
                const auto& data = std::get<KeyPressedData>(action.data);
                input().keysPressed.insert(data.code);

                if (data.code == sf::Keyboard::Key::Space)
                {
                    send(Command(CommandType::PlayerAttack, CmdPlayerAttack{.playerId = mPlayerEntityId}));
                }
                else if (data.code == sf::Keyboard::Key::Q)
                {
                    send(Command(CommandType::PlayerSkill, CmdPlayerSkill{.playerId = mPlayerEntityId, .skillId = 1}));
                }
                else if (data.code == sf::Keyboard::Key::E)
                {
                    send(Command(CommandType::PlayerSkill, CmdPlayerSkill{.playerId = mPlayerEntityId, .skillId = 2}));
                }
                else if (data.code == sf::Keyboard::Key::F || data.code == sf::Keyboard::Key::R)
                {
                    send(Command(CommandType::PlayerSkill, CmdPlayerSkill{.playerId = mPlayerEntityId, .skillId = 3}));
                }
                else if (data.code == sf::Keyboard::Key::Escape)
                {
                    window().close();
                }
                else if (data.code == sf::Keyboard::Key::F3 || data.code == sf::Keyboard::Key::P)
                {
                    debug().showPanel = !debug().showPanel;
                }
            }
            break;
        }
        case Action::Type::EventKeyReleased:
        {
            if (std::holds_alternative<KeyReleasedData>(action.data))
            {
                const auto& data = std::get<KeyReleasedData>(action.data);
                input().keysPressed.erase(data.code);
            }
            break;
        }
        case Action::Type::EventTouchBegan:
        {
            if (std::holds_alternative<TouchBeganData>(action.data))
            {
                const auto& data = std::get<TouchBeganData>(action.data);
                if (data.finger == 0)
                    input().touch0Pos = data.position;
                else if (data.finger == 1)
                    input().touch1Pos = data.position;

                if (input().activeWorldFingerId == -1)
                {
                    input().activeWorldFingerId = data.finger;
                    input().touchStartPos       = data.position;

                    sf::Vector2i pixelPos = data.position;

                    sf::Vector2f uiPos    = window().mapPixelToCoords(pixelPos, uiView());
                    float        scale    = 1170.f / 768.f;
                    float        minimapW = 280.f * scale + 20.f * scale;
                    const float  safeMapW = std::max(1.f, mapWidth());
                    float        minimapH = minimapW * (mapHeight() / safeMapW) + 20.f * scale;

                    if (uiPos.x <= minimapW && uiPos.y <= minimapH)
                    {
                        break;
                    }

                    if (input().inputMode == InputMode::Select)
                    {
                        if (uiPos.y >= 1170.f - 210.f)
                        {
                            break;
                        }
                    }

                    sf::Vector2f worldPos = screenToWorldGround(pixelPos);

                    if (input().build.placementMode)
                    {
                        if (input().build.palette == PlacementPalette::Unit)
                            send(Command(CommandType::PlaceUnit,
                                         CmdPlaceUnit{.playerId  = mPlayerEntityId,
                                                      .unitClass = input().build.selectedUnitClass,
                                                      .worldX    = worldPos.x,
                                                      .worldY    = worldPos.y}));
                        else
                            send(Command(CommandType::PlaceWall,
                                         CmdPlaceWall{.playerId = mPlayerEntityId,
                                                      .kind     = input().build.selectedKind,
                                                      .worldX   = worldPos.x,
                                                      .worldY   = worldPos.y}));
                        input().build.placementMode = false;
                        break;
                    }

                    input().leftMousePressed     = true;
                    input().leftMouseJustPressed = true;
                    input().prevMousePixelPos    = pixelPos;
                    input().mousePixelPos        = pixelPos;
                    input().mouseWorldPos        = worldPos;
                    input().isCameraDragging     = false;
                    input().cameraDragDelta      = {0.f, 0.f};
                }
            }
            break;
        }
        case Action::Type::EventTouchEnded:
        {
            if (std::holds_alternative<TouchEndedData>(action.data))
            {
                const auto& data = std::get<TouchEndedData>(action.data);
                input().lastPinchDist.reset();
                if (data.finger == 0)
                    input().touch0Pos.reset();
                else if (data.finger == 1)
                    input().touch1Pos.reset();

                if (data.finger == input().activeWorldFingerId)
                {
                    sf::Vector2f worldPos = screenToWorldGround(data.position);

                    input().leftMousePressed      = false;
                    input().leftMouseJustReleased = true;
                    input().mousePixelPos         = data.position;
                    input().mouseWorldPos         = worldPos;

                    input().activeWorldFingerId = -1;
                    input().lastPanTouch.reset();
                    input().touchStartPos.reset();

                    input().cameraDragDelta  = {0.f, 0.f};
                    input().isCameraDragging = false;
                }
            }
            break;
        }
        case Action::Type::EventTouchMoved:
        {
            if (std::holds_alternative<TouchMovedData>(action.data))
            {
                const auto& data = std::get<TouchMovedData>(action.data);

                if (data.finger == 0)
                    input().touch0Pos = data.position;
                else if (data.finger == 1)
                    input().touch1Pos = data.position;

                if (input().touch0Pos.has_value() && input().touch1Pos.has_value())
                {
                    auto  pos0        = sf::Vector2f(input().touch0Pos.value());
                    auto  pos1        = sf::Vector2f(input().touch1Pos.value());
                    float currentDist = std::hypot(pos0.x - pos1.x, pos0.y - pos1.y);

                    if (input().lastPinchDist.has_value())
                    {
                        float delta = currentDist - input().lastPinchDist.value();
                        if (std::abs(delta) > 0.5f)
                        {
                            input().pinchZoomDelta = -delta * 0.001f;
                        }
                    }
                    input().lastPinchDist = currentDist;
                    input().lastPanTouch.reset();
                    break;
                }

                if (data.finger == input().activeWorldFingerId)
                {
                    sf::Vector2i currentTouch = data.position;

                    if (input().inputMode == InputMode::DragDrop)
                    {
                        if (input().dragSelectedId == -1)
                        {
                            if (input().touchStartPos.has_value())
                            {
                                float moveDist = std::hypot((float)(currentTouch.x - input().touchStartPos->x),
                                                            (float)(currentTouch.y - input().touchStartPos->y));
                                if (input().isCameraDragging || moveDist >= 10.f)
                                {
                                    if (!input().isCameraDragging)
                                    {
                                        input().prevMousePixelPos = currentTouch;
                                        input().isCameraDragging  = true;
                                    }
                                }
                            }
                        }
                    }

                    sf::Vector2f worldPos     = screenToWorldGround(currentTouch);
                    input().prevMousePixelPos = input().mousePixelPos;
                    input().mousePixelPos     = currentTouch;
                    input().mouseWorldPos     = worldPos;
                }
            }
            break;
        }

        case Action::Type::EventMouseButtonPressed:
        {
            if (std::holds_alternative<MouseButtonPressedData>(action.data))
            {
                const auto&  data         = std::get<MouseButtonPressedData>(action.data);
                sf::Vector2i pixelPos     = data.position;
                bool         isRightClick = (data.button == sf::Mouse::Button::Right);

                sf::Vector2f uiPos    = window().mapPixelToCoords(pixelPos, uiView());
                float        scale    = 1170.f / 768.f;
                float        minimapW = 280.f * scale + 20.f * scale;
                const float  safeMapW = std::max(1.f, mapWidth());
                float        minimapH = minimapW * (mapHeight() / safeMapW) + 20.f * scale;

                if (!isRightClick && uiPos.x <= minimapW && uiPos.y <= minimapH)
                {
                    break;
                }

                if (!isRightClick && input().inputMode == InputMode::Select)
                {
                    if (uiPos.y >= 1170.f - 210.f)
                    {
                        break;
                    }
                }

                sf::Vector2f worldPos = screenToWorldGround(pixelPos);

                if (isRightClick && input().build.placementMode)
                {
                    input().build.placementMode = false;
                    break;
                }

                if (input().build.placementMode && !isRightClick)
                {
                    if (input().build.palette == PlacementPalette::Unit)
                        send(Command(CommandType::PlaceUnit,
                                     CmdPlaceUnit{.playerId  = mPlayerEntityId,
                                                  .unitClass = input().build.selectedUnitClass,
                                                  .worldX    = worldPos.x,
                                                  .worldY    = worldPos.y}));
                    else
                        send(Command(CommandType::PlaceWall,
                                     CmdPlaceWall{.playerId = mPlayerEntityId,
                                                  .kind     = input().build.selectedKind,
                                                  .worldX   = worldPos.x,
                                                  .worldY   = worldPos.y}));
                    input().build.placementMode = false;
                    break;
                }

                if (isRightClick)
                {
                    input().rightMousePressed     = true;
                    input().rightMouseJustPressed = true;
                    input().prevMousePixelPos     = pixelPos;
                    input().mousePixelPos         = pixelPos;
                    input().mouseWorldPos         = worldPos;
                    input().isCameraDragging      = true;
                    input().cameraDragDelta       = {0.f, 0.f};
                }
                else
                {
                    input().leftMousePressed     = true;
                    input().leftMouseJustPressed = true;
                    input().mousePixelPos        = pixelPos;
                    input().mouseWorldPos        = worldPos;
                }
            }
            break;
        }
        case Action::Type::EventMouseButtonReleased:
        {
            if (std::holds_alternative<MouseButtonReleasedData>(action.data))
            {
                const auto&  data         = std::get<MouseButtonReleasedData>(action.data);
                sf::Vector2f worldPos     = screenToWorldGround(data.position);
                bool         isRightClick = (data.button == sf::Mouse::Button::Right);

                if (isRightClick)
                {
                    input().rightMousePressed      = false;
                    input().rightMouseJustReleased = true;
                    input().mousePixelPos          = data.position;
                    input().mouseWorldPos          = worldPos;
                    input().isCameraDragging       = false;
                    input().cameraDragDelta        = {0.f, 0.f};
                }
                else
                {
                    input().leftMousePressed      = false;
                    input().leftMouseJustReleased = true;
                    input().mousePixelPos         = data.position;
                    input().mouseWorldPos         = worldPos;
                }
            }
            break;
        }
        case Action::Type::EventMouseMoved:
        {
            if (std::holds_alternative<MouseMovedData>(action.data))
            {
                const auto&  data     = std::get<MouseMovedData>(action.data);
                sf::Vector2i pixelPos = data.position;
                sf::Vector2f worldPos = screenToWorldGround(pixelPos);

                input().prevMousePixelPos = input().mousePixelPos;
                input().mousePixelPos     = pixelPos;
                input().mouseWorldPos     = worldPos;
            }
            break;
        }
        case Action::Type::EventMouseWheelScrolled:
        {
            if (std::holds_alternative<MouseWheelScrolledData>(action.data))
            {
                const auto& data  = std::get<MouseWheelScrolledData>(action.data);
                input().zoomDelta = -data.delta * 0.1f;
            }
            break;
        }
        case Action::Type::JoystickMove:
        {
            if (std::holds_alternative<sf::Vector2f>(action.data))
            {
                sf::Vector2f dir = std::get<sf::Vector2f>(action.data);
                float        len = std::hypot(dir.x, dir.y);
                if (len > 5.f)
                {
                    dir /= len;
                }
                else
                {
                    dir = {0.f, 0.f};
                }
                input().joystickDir = dir;
            }
            break;
        }
        case Action::Type::JoystickRelease:
        {
            input().joystickDir = {0.f, 0.f};
            mLastSentMoveDir    = {0.f, 0.f};
            send(Command(CommandType::MovePlayer, CmdMovePlayer{.playerId = mPlayerEntityId, .dirX = 0.f, .dirY = 0.f}));
            break;
        }
        case Action::Type::Attack:
        {
            send(Command(CommandType::PlayerAttack, CmdPlayerAttack{.playerId = mPlayerEntityId}));
            break;
        }
        case Action::Type::Skill1:
        {
            send(Command(CommandType::PlayerSkill, CmdPlayerSkill{.playerId = mPlayerEntityId, .skillId = 1}));
            break;
        }
        case Action::Type::Skill2:
        {
            send(Command(CommandType::PlayerSkill, CmdPlayerSkill{.playerId = mPlayerEntityId, .skillId = 2}));
            break;
        }
        case Action::Type::Skill3:
        {
            send(Command(CommandType::PlayerSkill, CmdPlayerSkill{.playerId = mPlayerEntityId, .skillId = 3}));
            break;
        }
        case Action::Type::CombatMode:
        {
            input().combatModeState = CombatStance::Attack;
            send(Command(CommandType::CombatMode,
                         CmdCombatMode{.playerId = mPlayerEntityId, .state = CombatStance::Attack}));
            break;
        }
        case Action::Type::DesignatedMode:
        {
            input().combatModeState = CombatStance::Tactical;
            send(Command(CommandType::CombatMode,
                         CmdCombatMode{.playerId = mPlayerEntityId, .state = CombatStance::Tactical}));
            break;
        }
        case Action::Type::UnitInfo:
        {
            GUICombat* guiCombat = nullptr;
            Space*     child     = mClientGame->firstChild;
            while (child)
            {
                guiCombat = dynamic_cast<GUICombat*>(child);
                if (guiCombat)
                    break;
                child = child->next;
            }
            if (guiCombat)
            {
                guiCombat->mShowUnitInfoPanel = !guiCombat->mShowUnitInfoPanel;
                if (guiCombat->mShowUnitInfoPanel)
                {
                    guiCombat->mShowHud = false;
                    debug().showPanel   = false;
                }
            }
            break;
        }
        case Action::Type::Hud:
        {
            GUICombat* guiCombat = nullptr;
            Space*     child     = mClientGame->firstChild;
            while (child)
            {
                guiCombat = dynamic_cast<GUICombat*>(child);
                if (guiCombat)
                    break;
                child = child->next;
            }
            if (guiCombat)
            {
                guiCombat->mShowHud = !guiCombat->mShowHud;
                if (guiCombat->mShowHud)
                {
                    guiCombat->mShowUnitInfoPanel = false;
                    debug().showPanel             = false;
                }
            }
            break;
        }
        case Action::Type::MoveCamera:
        {
            if (std::holds_alternative<sf::Vector2f>(action.data))
            {
                sf::Vector2f targetPos = std::get<sf::Vector2f>(action.data);
                auto         size      = worldView().getSize();
                float        minX      = size.x / 2.f;
                float        minY      = size.y / 2.f;
                float        maxX      = std::max(minX, static_cast<float>(gameState.mapWidth()) - size.x / 2.f);
                float        maxY      = std::max(minY, static_cast<float>(gameState.mapHeight()) - size.y / 2.f);
                targetPos.x            = std::clamp(targetPos.x, minX, maxX);
                targetPos.y            = std::clamp(targetPos.y, minY, maxY);
                worldView().setCenter(targetPos);
            }
            break;
        }
        default:
            mClientGame->Scene::doAction(action);
            break;
    }
}

void ViewState::handleEvents(const std::vector<GameEvent>& events)
{
    for (const auto& event : events)
    {
        std::visit(
            [this](const auto& e)
            {
                using T = std::decay_t<decltype(e)>;
                if constexpr (std::is_same_v<T, EventSound>)
                {
                    playSound(e);
                }
            },
            event);
    }
}

void ViewState::playSound(const EventSound& sound)
{
    std::optional<sf::Vector2f> listenerPos;
    const auto&                 gameState  = mClientGame->gameState();
    int                         pid        = mPlayerEntityId;
    const auto*                 playerMask = gameState.admin().mPool.getMask(pid);
    if (pid != -1 && gameState.admin().mPool.hasComponent<CPosition>(playerMask))
    {
        listenerPos = gameState.admin().mPool.getComponent<CPosition>(pid)->pos;
    }
    else
    {
        listenerPos = worldView().getCenter();
    }

    sf::Vector2f  center = worldView().getCenter();
    sf::Vector2f  size   = worldView().getSize();
    sf::FloatRect viewBounds(center - size * 0.5f, size);

    std::optional<sf::Vector2f> wp;
    if (sound.worldPos)
        wp = static_cast<sf::Vector2f>(*sound.worldPos);
    std::optional<float> ci;
    if (sound.customImportance)
        ci = static_cast<float>(*sound.customImportance);

    mClientGame->mGame->mAssets.playSound(sound.soundName, wp, ci, listenerPos, viewBounds);
}
