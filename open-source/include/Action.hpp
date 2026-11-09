#ifndef ACTION_HPP
#define ACTION_HPP

#include "Components.hpp"

#include <SFML/Window/Event.hpp>

#include <SFML/System/Vector2.hpp>

#include <string>
#include <variant>

struct KeyPressedData
{
    sf::Keyboard::Key code;
};

struct KeyReleasedData
{
    sf::Keyboard::Key code;
};

struct TouchBeganData
{
    int          finger;
    sf::Vector2i position;
};

struct TouchEndedData
{
    int          finger;
    sf::Vector2i position;
};

struct TouchMovedData
{
    int          finger;
    sf::Vector2i position;
};

struct MouseButtonPressedData
{
    sf::Mouse::Button button;
    sf::Vector2i      position;
};

struct MouseButtonReleasedData
{
    sf::Mouse::Button button;
    sf::Vector2i      position;
};

struct MouseMovedData
{
    sf::Vector2i position;
};

struct MouseWheelScrolledData
{
    float delta;
};

struct Action
{
    enum class Type
    {
        SelectWallKind,
        SelectUnitClass,
        TogglePlacementPalette,

        Back,
        Debug,
        CombatMode,
        DesignatedMode,
        Select,
        DragDrop,
        ToggleInputMode,
        Speed,
        UnitInfo,
        Hud,

        JoystickMove,
        JoystickRelease,
        Attack,
        Skill1,
        Skill2,
        Skill3,

        ExitToMenu,

        Level1,
        Level2,
        Level3,
        Easy,
        Normal,
        Hard,
        SinglePlayer,
        Start,
        Multiplayer,
        Settings,
        Instruction,
        Replay,
        Exit,

        MasterVolumeMinus,
        MasterVolumePlus,
        MusicVolumeMinus,
        MusicVolumePlus,
        SfxVolumeMinus,
        SfxVolumePlus,
        GraphicsMinus,
        GraphicsPlus,
        GraphicsQuality,

        Vietnamese,
        English,

        MoveCamera,

        EventKeyPressed,
        EventKeyReleased,
        EventTouchBegan,
        EventTouchEnded,
        EventTouchMoved,
        EventMouseButtonPressed,
        EventMouseButtonReleased,
        EventMouseMoved,
        EventMouseWheelScrolled
    };

    Type type;
    int  frame = 0;
    std::variant<std::monostate,
                 WallKind,
                 UnitClass,
                 sf::Vector2f,
                 int,
                 float,
                 KeyPressedData,
                 KeyReleasedData,
                 TouchBeganData,
                 TouchEndedData,
                 TouchMovedData,
                 MouseButtonPressedData,
                 MouseButtonReleasedData,
                 MouseMovedData,
                 MouseWheelScrolledData>
        data;

    Action(Type t) : type(t), data(std::monostate{})
    {
    }

    template <typename T>
    Action(Type t, T value) : type(t), data(value)
    {
    }

    [[nodiscard]] std::string toString() const;
};

#endif
