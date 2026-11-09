#ifndef GUICONTROL_HPP
#define GUICONTROL_HPP

#include "Space.hpp"

#include <SFML/Graphics.hpp>

struct GUIControl : Space
{
    struct Joystick : Space
    {
        sf::Vector2f mCenter;
        float        mRadius = 100.f;
        sf::Vector2f mKnobPos;
        float        mKnobRadius = 45.f;
        bool         mIsActive   = false;
        int          mFingerId   = -1;

        sf::CircleShape mBaseShape;
        sf::CircleShape mKnobShape;
        bool            mInitialized = false;

        ~Joystick() override;

        void update() override;
        bool input(const sf::Event& event) override;
        void render() override;
    };

    struct SkillButton : Space
    {
        enum class SkillType
        {
            Attack,
            Skill1,
            Skill2,
            Skill3
        };

        SkillType    mType;
        sf::Vector2f mCenter;
        float        mRadius   = 55.f;
        bool         mPressed  = false;
        int          mFingerId = -1;

        sf::CircleShape mBtnShape;
        sf::CircleShape mOverlayShape;
        sf::Text        mText;
        sf::Text        mCDText;
        const char*     mLabel       = "";
        bool            mInitialized = false;

        SkillButton(SkillType type, sf::Font& font, const char* label);
        ~SkillButton() override;

        void update() override;
        bool input(const sf::Event& event) override;
        void render() override;
    };

    sf::Font& mFont;

    sf::CircleShape    mRangeCircleShape;
    sf::RectangleShape mRespawnOverlayShape;
    sf::Text           mRespawnText;

    explicit GUIControl(sf::Font& font);

    bool input(const sf::Event&) override;
    void update() override;
    void render() override;
};

#endif
