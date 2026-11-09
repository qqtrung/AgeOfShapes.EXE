#ifndef SCENEMULTIPLAYER_HPP
#define SCENEMULTIPLAYER_HPP

#include "Action.hpp"
#include "Packet.hpp"
#include "Scene.hpp"

#include <string>
#include <vector>

struct SceneMultiplayer : Scene
{
    int                                mCurrentRoomId  = -1;
    bool                               mIsHost         = false;
    bool                               mIsReady        = false;
    int                                mRoomLevel      = 1;
    int                                mRoomDifficulty = 1;
    int                                mRoomGameMode   = 0;
    std::string                        mPlayerName;
    std::vector<RoomSlotSync>          mCurrentRoomSlots;
    std::vector<PacketLobbyClientInfo> mOnlineClients;
    std::vector<PacketLobbyRoomInfo>   mOnlineRooms;

    bool        mHasPendingInvite    = false;
    int         mPendingInviteRoomId = -1;
    int         mPendingInviteHostId = -1;
    std::string mPendingInviteHostName;
    std::string mLobbyNotice          = "In lobby...";
    bool        mStartingCombat       = false;
    bool        mServerDisconnected   = false;
    bool        mHasSentConnectPacket = false;

    SceneMultiplayer(Game* game);
    ~SceneMultiplayer() override;

    bool input(const sf::Event& event) override;
    void update() override;
    void render() override;
    void network() override;
    void doAction(const Action& action) override;

private:
    void drawUI();
};

#endif
