#include "GameStateData.hpp"

UnitTypeConfig& CampaignState::getUnitConfig(UnitClass uc)
{
    return unitConfigs[static_cast<size_t>(uc)];
}

const UnitTypeConfig& CampaignState::getUnitConfig(UnitClass uc) const
{
    return unitConfigs[static_cast<size_t>(uc)];
}

NavData::NavData()
{
    mFieldDesignated = std::make_unique<VectorField>(mGrid);
}

const Grid<char>& NavData::getVisionGrid(int teamId) const
{
    if (teamId >= 0 && teamId < static_cast<int>(mTeamVisionGrid.size()))
    {
        if (mTeamVisionGrid[teamId].width() > 0 && mTeamVisionGrid[teamId].height() > 0)
        {
            return mTeamVisionGrid[teamId];
        }
    }
    return mVisionGrid;
}

VectorField& NavData::getDesignatedField(int teamId)
{
    if (teamId <= 0)
        return *mFieldDesignated;
    if (teamId >= static_cast<int>(mTeamDesignatedFields.size()))
    {
        mTeamDesignatedFields.resize(teamId + 1);
    }
    if (!mTeamDesignatedFields[teamId])
    {
        mTeamDesignatedFields[teamId] = std::make_unique<VectorField>(mGrid);
    }
    return *mTeamDesignatedFields[teamId];
}

const VectorField& NavData::getDesignatedField(int teamId) const
{
    return const_cast<NavData*>(this)->getDesignatedField(teamId);
}

VectorField& NavData::getTeamField(int teamId)
{
    if (teamId < 0)
        teamId = 0;
    if (teamId >= static_cast<int>(mTeamFlowFields.size()))
    {
        mTeamFlowFields.resize(teamId + 1);
    }
    if (!mTeamFlowFields[teamId])
    {
        mTeamFlowFields[teamId] = std::make_unique<VectorField>(mGrid);
    }
    return *mTeamFlowFields[teamId];
}

const VectorField& NavData::getTeamField(int teamId) const
{
    return const_cast<NavData*>(this)->getTeamField(teamId);
}
