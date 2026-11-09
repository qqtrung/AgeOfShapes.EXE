#include "GuiControl.hpp"

#include "ClientGame.hpp"
#include "FloatFixed.hpp"
#include "Game.hpp"
#include "Profiler.hpp"
#include "TextUtil.hpp"
#include "ViewState.hpp"

#include <cmath>


GUIControl::Joystick::~Joystick()
{
    logout("~Joystick()");
}

void GUIControl::Joystick::update()
{
    PROFILE_FUNCTION();
}

bool GUIControl::Joystick::input(const sf::Event& event)
{
    auto* guiControl = dynamic_cast<GUIControl*>(parent);
    if (!guiControl)
    {
        return false;
    }

    auto* client = dynamic_cast<ClientGame*>(guiControl->parent);
    if (!client || client->isGameOver())
    {
        return false;
    }

    sf::Vector2f windowSize = {2532.f, 1170.f};
    float        scale      = 1170.f / 768.f;

    mRadius     = 125.f * scale;
    mKnobRadius = 62.f * scale;
    mCenter     = {230.f * scale, windowSize.y - 230.f * scale};

    if (const auto* mousePressed = event.getIf<sf::Event::MouseButtonPressed>())
    {
        if (mousePressed->button == sf::Mouse::Button::Left)
        {
            sf::Vector2f clickPos = client->window().mapPixelToCoords(mousePressed->position, client->mViewState->uiView());

            sf::Vector2f joyDiff = clickPos - mCenter;
            if (std::hypot(joyDiff.x, joyDiff.y) <= mRadius)
            {
                mIsActive  = true;
                float dist = std::hypot(joyDiff.x, joyDiff.y);
                if (dist > mRadius)
                {
                    mKnobPos = mCenter + (joyDiff / dist) * mRadius;
                }
                else
                {
                    mKnobPos = clickPos;
                }
                Action action(Action::Type::JoystickMove, mKnobPos - mCenter);
                pushAction(action);
                return true;
            }
        }
    }

    if (const auto* mouseMoved = event.getIf<sf::Event::MouseMoved>())
    {
        if (mIsActive)
        {
            sf::Vector2f mousePos = client->window().mapPixelToCoords(mouseMoved->position, client->mViewState->uiView());
            sf::Vector2f joyDiff = mousePos - mCenter;
            float        dist    = std::hypot(joyDiff.x, joyDiff.y);
            if (dist > mRadius)
            {
                mKnobPos = mCenter + (joyDiff / dist) * mRadius;
            }
            else
            {
                mKnobPos = mousePos;
            }
            Action action(Action::Type::JoystickMove, mKnobPos - mCenter);
            pushAction(action);
            return true;
        }
    }

    if (const auto* mouseReleased = event.getIf<sf::Event::MouseButtonReleased>())
    {
        if (mouseReleased->button == sf::Mouse::Button::Left)
        {
            if (mIsActive)
            {
                mIsActive = false;
                mKnobPos  = mCenter;
                Action action(Action::Type::JoystickRelease);
                pushAction(action);
                return true;
            }
        }
    }

    if (const auto* touchBegan = event.getIf<sf::Event::TouchBegan>())
    {
        sf::Vector2f touchPos = client->window().mapPixelToCoords({(int)touchBegan->position.x,
                                                                   (int)touchBegan->position.y},
                                                                  client->mViewState->uiView());

        sf::Vector2f joyDiff = touchPos - mCenter;
        if (std::hypot(joyDiff.x, joyDiff.y) <= mRadius && !mIsActive)
        {
            mIsActive  = true;
            mFingerId  = touchBegan->finger;
            float dist = std::hypot(joyDiff.x, joyDiff.y);
            if (dist > mRadius)
            {
                mKnobPos = mCenter + (joyDiff / dist) * mRadius;
            }
            else
            {
                mKnobPos = touchPos;
            }
            Action action(Action::Type::JoystickMove, mKnobPos - mCenter);
            pushAction(action);
            return true;
        }
    }

    if (const auto* touchMoved = event.getIf<sf::Event::TouchMoved>())
    {
        sf::Vector2f touchPos = client->window().mapPixelToCoords({(int)touchMoved->position.x,
                                                                   (int)touchMoved->position.y},
                                                                  client->mViewState->uiView());

        if (mIsActive && touchMoved->finger == mFingerId)
        {
            sf::Vector2f joyDiff = touchPos - mCenter;
            float        dist    = std::hypot(joyDiff.x, joyDiff.y);
            if (dist > mRadius)
            {
                mKnobPos = mCenter + (joyDiff / dist) * mRadius;
            }
            else
            {
                mKnobPos = touchPos;
            }
            Action action(Action::Type::JoystickMove, mKnobPos - mCenter);
            pushAction(action);
            return true;
        }
    }

    if (const auto* touchEnded = event.getIf<sf::Event::TouchEnded>())
    {
        if (mIsActive && touchEnded->finger == mFingerId)
        {
            mIsActive = false;
            mFingerId = -1;
            mKnobPos  = mCenter;
            Action action(Action::Type::JoystickRelease);
            pushAction(action);
            return true;
        }
    }

    return false;
}

void GUIControl::Joystick::render()
{
    PROFILE_FUNCTION();
    auto* guiControl = dynamic_cast<GUIControl*>(parent);
    if (!guiControl)
    {
        return;
    }

    auto* client = dynamic_cast<ClientGame*>(guiControl->parent);
    if (!client)
    {
        return;
    }

    auto& window = client->window();
    window.setView(client->mViewState->uiView());

    sf::Vector2f windowSize = {2532.f, 1170.f};
    float        scale      = 1170.f / 768.f;

    if (!mInitialized)
    {
        mRadius     = 125.f * scale;
        mKnobRadius = 62.f * scale;
        mCenter     = {230.f * scale, windowSize.y - 230.f * scale};

        mBaseShape.setRadius(mRadius);
        mBaseShape.setOrigin({mRadius, mRadius});
        mBaseShape.setPosition(mCenter);
        mBaseShape.setFillColor(sf::Color(255, 255, 255, 25));
        mBaseShape.setOutlineThickness(3.f * scale);
        mBaseShape.setOutlineColor(sf::Color(0, 255, 240, 100));

        mKnobShape.setRadius(mKnobRadius);
        mKnobShape.setOrigin({mKnobRadius, mKnobRadius});
        mKnobShape.setFillColor(sf::Color(0, 255, 240, 150));
        mKnobShape.setOutlineThickness(2.f * scale);
        mKnobShape.setOutlineColor(sf::Color::White);

        mInitialized = true;
    }

    if (!mIsActive)
    {
        mKnobPos = mCenter;
    }

    window.draw(mBaseShape);
    mKnobShape.setPosition(mKnobPos);
    window.draw(mKnobShape);
}


GUIControl::SkillButton::SkillButton(SkillType type, sf::Font& font, const char* label) :
    mType(type),
    mText(font, label, TextUtil::kBaseCharSize),
    mCDText(font, "", TextUtil::kBaseCharSize),
    mLabel(label)
{
    mText.setFillColor(sf::Color::White);
    mCDText.setFillColor(sf::Color::White);
}

GUIControl::SkillButton::~SkillButton()
{
    logout("~SkillButton()");
}

void GUIControl::SkillButton::update()
{
    PROFILE_FUNCTION();
}

bool GUIControl::SkillButton::input(const sf::Event& event)
{
    auto* guiControl = dynamic_cast<GUIControl*>(parent);
    if (!guiControl)
    {
        return false;
    }

    auto* client = dynamic_cast<ClientGame*>(guiControl->parent);
    if (!client || client->isGameOver())
    {
        return false;
    }

    sf::Vector2f windowSize = {2532.f, 1170.f};
    float        scale      = 1170.f / 768.f;

    if (mType == SkillType::Attack)
    {
        mRadius = 80.f * scale;
        mCenter = {windowSize.x - 160.f * scale, windowSize.y - 160.f * scale};
    }
    else if (mType == SkillType::Skill1)
    {
        mRadius = 62.f * scale;
        mCenter = {windowSize.x - 330.f * scale, windowSize.y - 160.f * scale};
    }
    else if (mType == SkillType::Skill2)
    {
        mRadius = 62.f * scale;
        mCenter = {windowSize.x - 160.f * scale, windowSize.y - 330.f * scale};
    }
    else if (mType == SkillType::Skill3)
    {
        mRadius = 62.f * scale;
        mCenter = {windowSize.x - 300.f * scale, windowSize.y - 300.f * scale};
    }

    if (const auto* mousePressed = event.getIf<sf::Event::MouseButtonPressed>())
    {
        if (mousePressed->button == sf::Mouse::Button::Left)
        {
            sf::Vector2f clickPos = client->window().mapPixelToCoords(mousePressed->position, client->mViewState->uiView());

            sf::Vector2f diff = clickPos - mCenter;
            if (std::hypot(diff.x, diff.y) <= mRadius)
            {
                mPressed = true;
                Action action(Action::Type::Attack);
                if (mType == SkillType::Skill1)
                {
                    action = Action(Action::Type::Skill1);
                }
                else if (mType == SkillType::Skill2)
                {
                    action = Action(Action::Type::Skill2);
                }
                else if (mType == SkillType::Skill3)
                {
                    action = Action(Action::Type::Skill3);
                }
                pushAction(action);
                return true;
            }
        }
    }

    if (const auto* mouseReleased = event.getIf<sf::Event::MouseButtonReleased>())
    {
        if (mouseReleased->button == sf::Mouse::Button::Left)
        {
            if (mPressed)
            {
                mPressed = false;
                return true;
            }
        }
    }

    if (const auto* touchBegan = event.getIf<sf::Event::TouchBegan>())
    {
        sf::Vector2f touchPos = client->window().mapPixelToCoords({(int)touchBegan->position.x,
                                                                   (int)touchBegan->position.y},
                                                                  client->mViewState->uiView());

        sf::Vector2f diff = touchPos - mCenter;
        if (std::hypot(diff.x, diff.y) <= mRadius)
        {
            mPressed  = true;
            mFingerId = touchBegan->finger;
            Action action(Action::Type::Attack);
            if (mType == SkillType::Skill1)
            {
                action = Action(Action::Type::Skill1);
            }
            else if (mType == SkillType::Skill2)
            {
                action = Action(Action::Type::Skill2);
            }
            else if (mType == SkillType::Skill3)
            {
                action = Action(Action::Type::Skill3);
            }
            pushAction(action);
            return true;
        }
    }

    if (const auto* touchEnded = event.getIf<sf::Event::TouchEnded>())
    {
        if (mPressed && touchEnded->finger == mFingerId)
        {
            mPressed  = false;
            mFingerId = -1;
            return true;
        }
    }

    return false;
}

void GUIControl::SkillButton::render()
{
    PROFILE_FUNCTION();
    auto* guiControl = dynamic_cast<GUIControl*>(parent);
    if (!guiControl)
    {
        return;
    }

    auto* client = dynamic_cast<ClientGame*>(guiControl->parent);
    if (!client)
    {
        return;
    }

    auto& window = client->window();
    window.setView(client->mViewState->uiView());

    sf::Vector2f windowSize = {2532.f, 1170.f};
    float        scale      = 1170.f / 768.f;

    if (!mInitialized)
    {
        if (mType == SkillType::Attack)
        {
            mRadius = 80.f * scale;
            mCenter = {windowSize.x - 160.f * scale, windowSize.y - 160.f * scale};
            mBtnShape.setFillColor(sf::Color(120, 130, 140, 180));
            mBtnShape.setOutlineColor(sf::Color(255, 255, 255, 150));
        }
        else if (mType == SkillType::Skill1)
        {
            mRadius = 62.f * scale;
            mCenter = {windowSize.x - 330.f * scale, windowSize.y - 160.f * scale};
            mBtnShape.setFillColor(sf::Color(0, 120, 255, 130));
            mBtnShape.setOutlineColor(sf::Color(0, 200, 255, 200));
        }
        else if (mType == SkillType::Skill2)
        {
            mRadius = 62.f * scale;
            mCenter = {windowSize.x - 160.f * scale, windowSize.y - 330.f * scale};
            mBtnShape.setFillColor(sf::Color(255, 80, 0, 130));
            mBtnShape.setOutlineColor(sf::Color(255, 150, 0, 200));
        }
        else if (mType == SkillType::Skill3)
        {
            mRadius = 62.f * scale;
            mCenter = {windowSize.x - 300.f * scale, windowSize.y - 300.f * scale};
            mBtnShape.setFillColor(sf::Color(230, 40, 120, 130));
            mBtnShape.setOutlineColor(sf::Color(255, 100, 180, 200));
        }

        mBtnShape.setRadius(mRadius);
        mBtnShape.setOrigin({mRadius, mRadius});
        mBtnShape.setPosition(mCenter);
        mBtnShape.setOutlineThickness((mType == SkillType::Attack) ? 3.f * scale : 2.5f * scale);

        mOverlayShape.setRadius(mRadius);
        mOverlayShape.setOrigin({mRadius, mRadius});
        mOverlayShape.setPosition(mCenter);
        mOverlayShape.setFillColor(sf::Color(0, 0, 0, (mType == SkillType::Attack) ? 120 : 180));

        const float textSize = (mType == SkillType::Attack) ? 17.f * scale : 13.5f * scale;
        TextUtil::setSize(mText, textSize);
        sf::FloatRect textBounds = mText.getLocalBounds();
        mText.setOrigin({textBounds.position.x + textBounds.size.x / 2.f, textBounds.position.y + textBounds.size.y / 2.f});
        mText.setPosition(mCenter - sf::Vector2f(0.f, 2.f * scale));

        mInitialized = true;
    }

    window.draw(mBtnShape);
    window.draw(mText);

    float       cooldownTimer = 0.f;
    const int   pid           = client->mViewState->playerEntityId();
    const auto* mask          = client->gameState().mAdmin.mPool.getMask(pid);
    if (pid != -1 && client->gameState().mAdmin.mPool.hasComponent<CPlayer>(mask))
    {
        const auto playerComp = client->gameState().mAdmin.mPool.getComponent<CPlayer>(pid);
        if (mType == SkillType::Attack)
        {
            cooldownTimer = static_cast<float>(playerComp->attackTimer);
        }
        else if (mType == SkillType::Skill1)
        {
            cooldownTimer = static_cast<float>(playerComp->skill1Timer);
        }
        else if (mType == SkillType::Skill2)
        {
            cooldownTimer = static_cast<float>(playerComp->skill2Timer);
        }
        else if (mType == SkillType::Skill3)
        {
            cooldownTimer = static_cast<float>(playerComp->skill3Timer);
        }
    }

    if (cooldownTimer > 0)
    {
        window.draw(mOverlayShape);

        if (mType != SkillType::Attack)
        {
            char buf[16];
            std::snprintf(buf, sizeof(buf), "%.1fs", cooldownTimer / 60.f);
            mCDText.setString(buf);
            TextUtil::setSize(mCDText, 16.f * scale);
            sf::FloatRect cdTextBounds = mCDText.getLocalBounds();
            mCDText.setOrigin({cdTextBounds.position.x + cdTextBounds.size.x / 2.f,
                               cdTextBounds.position.y + cdTextBounds.size.y / 2.f});
            mCDText.setPosition(mCenter);
            window.draw(mCDText);
        }
    }
}


GUIControl::GUIControl(sf::Font& font) : mFont(font), mRespawnText(font, "", TextUtil::kBaseCharSize)
{
    addChild<Joystick>();
    addChild<SkillButton>(SkillButton::SkillType::Attack, font, "ATTACK");
    addChild<SkillButton>(SkillButton::SkillType::Skill1, font, "BULLET");
    addChild<SkillButton>(SkillButton::SkillType::Skill2, font, "FIRE");
    addChild<SkillButton>(SkillButton::SkillType::Skill3, font, "SLASH");

    mRespawnText.setFillColor(sf::Color(255, 255, 255));
    mRespawnText.setOutlineColor(sf::Color::Black);
    mRespawnText.setOutlineThickness(3.f);
    TextUtil::setSize(mRespawnText, 48.f);
}

bool GUIControl::input(const sf::Event&)
{
    return false;
}

void GUIControl::update()
{
    PROFILE_FUNCTION();
}

void GUIControl::render()
{
    PROFILE_FUNCTION();
    auto* client = dynamic_cast<ClientGame*>(parent);
    if (!client)
    {
        return;
    }

    auto&    window  = client->window();
    sf::View oldView = window.getView();

    sf::Vector2f playerWorldPos;
    bool         hasPlayer   = false;
    float        attackRange = 200.f;
    float        sniperRange = 300.f;
    float        fireRange   = 150.f;

    const auto& pool  = client->gameState().mAdmin.mPool;
    int         myPid = client->mViewState->playerEntityId();
    if (myPid != -1 && pool.has(myPid))
    {
        const auto* mask = pool.getMask(myPid);
        if (pool.hasComponent<CPlayer>(mask) && pool.hasComponent<CPosition>(mask))
        {
            const auto posComp = pool.getComponent<CPosition>(myPid);
            playerWorldPos     = sf::Vector2f(static_cast<float>(posComp->pos.x), static_cast<float>(posComp->pos.y));
            hasPlayer          = true;
        }
    }
    else
    {
        for (int id : pool.viewIndex<CPlayer>())
        {
            const auto* mask = pool.getMask(id);
            if (pool.hasComponent<CPosition>(mask))
            {
                const auto posComp = pool.getComponent<CPosition>(id);
                playerWorldPos = sf::Vector2f(static_cast<float>(posComp->pos.x), static_cast<float>(posComp->pos.y));
                hasPlayer      = true;
            }
            break;
        }
    }

    SkillButton::SkillType pressedSkill = SkillButton::SkillType::Attack;
    bool                   anyPressed   = false;

    auto* child = firstChild;
    while (child)
    {
        auto* skillBtn = dynamic_cast<SkillButton*>(child);
        if (skillBtn && skillBtn->mPressed)
        {
            pressedSkill = skillBtn->mType;
            anyPressed   = true;
            break;
        }
        child = child->next;
    }

    if (hasPlayer && anyPressed)
    {
        window.setView(client->mViewState->worldView());
        auto& cam = client->mViewState->camera3D();
        cam.setup(client->mViewState->worldView());

        float     range        = attackRange;
        sf::Color outlineColor = sf::Color(0, 255, 120, 210);

        if (pressedSkill == SkillButton::SkillType::Skill1)
        {
            range        = sniperRange;
            outlineColor = sf::Color(0, 200, 255, 210);
        }
        else if (pressedSkill == SkillButton::SkillType::Skill2)
        {
            range        = fireRange;
            outlineColor = sf::Color(255, 120, 0, 210);
        }
        else if (pressedSkill == SkillButton::SkillType::Skill3)
        {
            range        = 120.f;
            outlineColor = sf::Color(255, 50, 150, 210);
        }

        static sf::VertexArray rangeRing3D(sf::PrimitiveType::Triangles);
        rangeRing3D.clear();
        constexpr int   kSegs     = 48;
        constexpr float kThick    = 2.2f;
        const float     rInner    = std::max(0.f, range - kThick * 0.5f);
        const float     rOuter    = range + kThick * 0.5f;
        const float     stepAngle = 2.f * 3.14159265f / static_cast<float>(kSegs);

        for (int i = 0; i < kSegs; ++i)
        {
            const float a1 = static_cast<float>(i) * stepAngle;
            const float a2 = static_cast<float>(i + 1) * stepAngle;
            const float c1 = std::cos(a1), s1 = std::sin(a1);
            const float c2 = std::cos(a2), s2 = std::sin(a2);

            const sf::Vector2f i1 = cam.project(playerWorldPos.x + c1 * rInner, playerWorldPos.y + s1 * rInner, 2.f);
            const sf::Vector2f o1 = cam.project(playerWorldPos.x + c1 * rOuter, playerWorldPos.y + s1 * rOuter, 2.f);
            const sf::Vector2f i2 = cam.project(playerWorldPos.x + c2 * rInner, playerWorldPos.y + s2 * rInner, 2.f);
            const sf::Vector2f o2 = cam.project(playerWorldPos.x + c2 * rOuter, playerWorldPos.y + s2 * rOuter, 2.f);

            rangeRing3D.append(sf::Vertex(i1, outlineColor));
            rangeRing3D.append(sf::Vertex(o1, outlineColor));
            rangeRing3D.append(sf::Vertex(o2, outlineColor));
            rangeRing3D.append(sf::Vertex(o2, outlineColor));
            rangeRing3D.append(sf::Vertex(i2, outlineColor));
            rangeRing3D.append(sf::Vertex(i1, outlineColor));
        }
        window.draw(rangeRing3D);
    }

    window.setView(client->mViewState->uiView());

    const auto* myPidMask = client->gameState().admin().mPool.getMask(myPid);
    if (myPid != -1 && client->gameState().admin().mPool.has(myPid) &&
        client->gameState().admin().mPool.hasComponent<CPlayer>(myPidMask))
    {
        auto playerComp = client->gameState().admin().mPool.getComponent<CPlayer>(myPid);
        if (playerComp->respawnTimer > 0)
        {
            sf::Vector2f windowSize = {2532.f, 1170.f};
            mRespawnOverlayShape.setSize(windowSize);
            mRespawnOverlayShape.setFillColor(sf::Color(0, 0, 0, 50));
            window.draw(mRespawnOverlayShape);

            float       seconds = std::ceil(playerComp->respawnTimer / 60.f);
            std::string textStr = "RESPAWNING IN " + std::to_string((int)seconds) + "...";
            mRespawnText.setString(textStr);

            sf::FloatRect textBounds = mRespawnText.getLocalBounds();
            mRespawnText.setOrigin(
                {textBounds.position.x + textBounds.size.x / 2.f, textBounds.position.y + textBounds.size.y / 2.f});
            mRespawnText.setPosition(windowSize / 2.f);

            window.draw(mRespawnText);
        }
    }

    window.setView(oldView);
}
