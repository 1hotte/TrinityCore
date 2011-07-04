/*
 * Copyright (C) 2008-2011 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2005-2009 MaNGOS <http://getmangos.com/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "BattlegroundMap.h"
#include "BattlegroundWS.h"
#include "Creature.h"
#include "GameObject.h"
#include "Language.h"
#include "Object.h"
#include "ObjectMgr.h"
#include "BattlegroundMgr.h"
#include "Player.h"
#include "World.h"
#include "WorldPacket.h"

// these variables aren't used outside of this file, so declare them only here
enum BG_WSG_Rewards
{
    BG_WSG_WIN = 0,
    BG_WSG_FLAG_CAP,
    BG_WSG_MAP_COMPLETE,
    BG_WSG_REWARD_NUM
};

uint32 BG_WSG_Honor[BG_HONOR_MODE_NUM][BG_WSG_REWARD_NUM] = {
    {20, 40, 40}, // normal honor
    {60, 40, 80}  // holiday
};

uint32 BG_WSG_Reputation[BG_HONOR_MODE_NUM][BG_WSG_REWARD_NUM] = {
    {0, 35, 0}, // normal honor
    {0, 45, 0}  // holiday
};

void BattlegroundWS::InstallBattleground()
{
    _flagKeepers[BG_TEAM_ALLIANCE]     = 0;
    _flagKeepers[BG_TEAM_HORDE]        = 0;
    m_DroppedFlagGUID[BG_TEAM_ALLIANCE] = 0;
    m_DroppedFlagGUID[BG_TEAM_HORDE]    = 0;
    _flagState[BG_TEAM_ALLIANCE]       = BG_WS_FLAG_STATE_ON_BASE;
    _flagState[BG_TEAM_HORDE]          = BG_WS_FLAG_STATE_ON_BASE;
    TeamScores[BG_TEAM_ALLIANCE]      = 0;
    TeamScores[BG_TEAM_HORDE]         = 0;
    bool isBGWeekend = sBattlegroundMgr->IsBGWeekend(GetTypeID());
    m_ReputationCapture = (isBGWeekend) ? 45 : 35;
    m_HonorWinKills = (isBGWeekend) ? 3 : 1;
    m_HonorEndKills = (isBGWeekend) ? 4 : 2;
    // For WorldState
    _minutesElapsed                    = 0;
    _lastFlagCaptureTime               = 0;
}

void BattlegroundWS::InitializeObjects()
{
    ObjectGUIDsByType.resize(BG_WS_OBJECT_MAX + BG_WS_CREATURES_MAX);

    // flags
    AddGameObject(BG_WS_OBJECT_A_FLAG, BG_OBJECT_A_FLAG_WS_ENTRY, 1540.423f, 1481.325f, 351.8284f, 3.089233f, 0, 0, 0.9996573f, 0.02617699f, BG_WS_FLAG_RESPAWN_TIME/1000);
    AddGameObject(BG_WS_OBJECT_H_FLAG, BG_OBJECT_H_FLAG_WS_ENTRY, 916.0226f, 1434.405f, 345.413f, 0.01745329f, 0, 0, 0.008726535f, 0.9999619f, BG_WS_FLAG_RESPAWN_TIME/1000);
    // buffs
    AddGameObject(BG_WS_OBJECT_SPEEDBUFF_1, BG_OBJECTID_SPEEDBUFF_ENTRY, 1449.93f, 1470.71f, 342.6346f, -1.64061f, 0, 0, 0.7313537f, -0.6819983f, BUFF_RESPAWN_TIME);
    AddGameObject(BG_WS_OBJECT_SPEEDBUFF_2, BG_OBJECTID_SPEEDBUFF_ENTRY, 1005.171f, 1447.946f, 335.9032f, 1.64061f, 0, 0, 0.7313537f, 0.6819984f, BUFF_RESPAWN_TIME);
    AddGameObject(BG_WS_OBJECT_REGENBUFF_1, BG_OBJECTID_REGENBUFF_ENTRY, 1317.506f, 1550.851f, 313.2344f, -0.2617996f, 0, 0, 0.1305263f, -0.9914448f, BUFF_RESPAWN_TIME);
    AddGameObject(BG_WS_OBJECT_REGENBUFF_2, BG_OBJECTID_REGENBUFF_ENTRY, 1110.451f, 1353.656f, 316.5181f, -0.6806787f, 0, 0, 0.333807f, -0.9426414f, BUFF_RESPAWN_TIME);
    AddGameObject(BG_WS_OBJECT_BERSERKBUFF_1, BG_OBJECTID_BERSERKERBUFF_ENTRY, 1320.09f, 1378.79f, 314.7532f, 1.186824f, 0, 0, 0.5591929f, 0.8290376f, BUFF_RESPAWN_TIME);
    AddGameObject(BG_WS_OBJECT_BERSERKBUFF_2, BG_OBJECTID_BERSERKERBUFF_ENTRY, 1139.688f, 1560.288f, 306.8432f, -2.443461f, 0, 0, 0.9396926f, -0.3420201f, BUFF_RESPAWN_TIME);
    // alliance gates
    AddGameObject(BG_WS_OBJECT_DOOR_A_1, BG_OBJECT_DOOR_A_1_WS_ENTRY, 1503.335f, 1493.466f, 352.1888f, 3.115414f, 0, 0, 0.9999143f, 0.01308903f, RESPAWN_IMMEDIATELY);
    AddGameObject(BG_WS_OBJECT_DOOR_A_2, BG_OBJECT_DOOR_A_2_WS_ENTRY, 1492.478f, 1457.912f, 342.9689f, 3.115414f, 0, 0, 0.9999143f, 0.01308903f, RESPAWN_IMMEDIATELY);
    AddGameObject(BG_WS_OBJECT_DOOR_A_3, BG_OBJECT_DOOR_A_3_WS_ENTRY, 1468.503f, 1494.357f, 351.8618f, 3.115414f, 0, 0, 0.9999143f, 0.01308903f, RESPAWN_IMMEDIATELY);
    AddGameObject(BG_WS_OBJECT_DOOR_A_4, BG_OBJECT_DOOR_A_4_WS_ENTRY, 1471.555f, 1458.778f, 362.6332f, 3.115414f, 0, 0, 0.9999143f, 0.01308903f, RESPAWN_IMMEDIATELY);
    AddGameObject(BG_WS_OBJECT_DOOR_A_5, BG_OBJECT_DOOR_A_5_WS_ENTRY, 1492.347f, 1458.34f, 342.3712f, -0.03490669f, 0, 0, 0.01745246f, -0.9998477f, RESPAWN_IMMEDIATELY);
    AddGameObject(BG_WS_OBJECT_DOOR_A_6, BG_OBJECT_DOOR_A_6_WS_ENTRY, 1503.466f, 1493.367f, 351.7352f, -0.03490669f, 0, 0, 0.01745246f, -0.9998477f, RESPAWN_IMMEDIATELY);
    // horde gates
    AddGameObject(BG_WS_OBJECT_DOOR_H_1, BG_OBJECT_DOOR_H_1_WS_ENTRY, 949.1663f, 1423.772f, 345.6241f, -0.5756807f, -0.01673368f, -0.004956111f, -0.2839723f, 0.9586737f, RESPAWN_IMMEDIATELY);
    AddGameObject(BG_WS_OBJECT_DOOR_H_2, BG_OBJECT_DOOR_H_2_WS_ENTRY, 953.0507f, 1459.842f, 340.6526f, -1.99662f, -0.1971825f, 0.1575096f, -0.8239487f, 0.5073641f, RESPAWN_IMMEDIATELY);
    AddGameObject(BG_WS_OBJECT_DOOR_H_3, BG_OBJECT_DOOR_H_3_WS_ENTRY, 949.9523f, 1422.751f, 344.9273f, 0.0f, 0, 0, 0, 1, RESPAWN_IMMEDIATELY);
    AddGameObject(BG_WS_OBJECT_DOOR_H_4, BG_OBJECT_DOOR_H_4_WS_ENTRY, 950.7952f, 1459.583f, 342.1523f, 0.05235988f, 0, 0, 0.02617695f, 0.9996573f, RESPAWN_IMMEDIATELY);

    WorldSafeLocsEntry const *sg = sWorldSafeLocsStore.LookupEntry(WS_GRAVEYARD_MAIN_ALLIANCE);
    if (sg)
        AddSpiritGuide(WS_SPIRIT_MAIN_ALLIANCE, sg->x, sg->y, sg->z, 3.124139f, BG_TEAM_ALLIANCE);

    sg = sWorldSafeLocsStore.LookupEntry(WS_GRAVEYARD_MAIN_HORDE);
    if (sg)
        AddSpiritGuide(WS_SPIRIT_MAIN_HORDE, sg->x, sg->y, sg->z, 3.193953f, BG_TEAM_HORDE);

    for (uint32 i = BG_WS_OBJECT_DOOR_A_1; i <= BG_WS_OBJECT_DOOR_H_4; ++i)
    {
        DoorClose(i);
        SpawnGameObject(i, RESPAWN_IMMEDIATELY);
    }
    for (uint32 i = BG_WS_OBJECT_A_FLAG; i <= BG_WS_OBJECT_BERSERKBUFF_2; ++i)
        SpawnGameObject(i, RESPAWN_ONE_DAY);

    UpdateWorldState(BG_WS_STATE_TIMER_ACTIVE, 1);
    UpdateWorldState(BG_WS_STATE_TIMER, 25);
}

void BattlegroundWS::StartBattleground()
{
    BattlegroundMap::StartBattleground();
    EndTimer = 25 * MINUTE * IN_MILLISECONDS;
    _timerUpdateWorldState = 1 * MINUTE * IN_MILLISECONDS;

    for (uint32 i = BG_WS_OBJECT_DOOR_A_1; i <= BG_WS_OBJECT_DOOR_A_4; ++i)
        DoorOpen(i);

    for (uint32 i = BG_WS_OBJECT_DOOR_H_1; i <= BG_WS_OBJECT_DOOR_H_2; ++i)
        DoorOpen(i);

    SpawnGameObject(BG_WS_OBJECT_DOOR_A_5, RESPAWN_ONE_DAY);
    SpawnGameObject(BG_WS_OBJECT_DOOR_A_6, RESPAWN_ONE_DAY);
    SpawnGameObject(BG_WS_OBJECT_DOOR_H_3, RESPAWN_ONE_DAY);
    SpawnGameObject(BG_WS_OBJECT_DOOR_H_4, RESPAWN_ONE_DAY);

    for (uint32 i = BG_WS_OBJECT_A_FLAG; i <= BG_WS_OBJECT_BERSERKBUFF_2; ++i)
        SpawnGameObject(i, RESPAWN_IMMEDIATELY);

    // players joining later are not egible
    StartTimedAchievement(ACHIEVEMENT_TIMED_TYPE_EVENT, WS_EVENT_START_BATTLE);
}

void BattlegroundWS::EndBattleground(uint32 winner)
{
    //win reward
    RewardHonorToTeam(GetBonusHonorFromKill(m_HonorWinKills), winner);

    //complete map_end rewards (even if no team wins)
    RewardHonorToTeam(GetBonusHonorFromKill(m_HonorEndKills), BG_TEAM_ALLIANCE);
    RewardHonorToTeam(GetBonusHonorFromKill(m_HonorEndKills), BG_TEAM_HORDE);

    BattlegroundMap::EndBattleground(winner);
}

void BattlegroundWS::ProcessInProgress(uint32 const& diff)
{
    BattlegroundMap::ProcessInProgress(diff);

    if (_timerUpdateWorldState <= diff)
    {
        ++_minutesElapsed;
        UpdateWorldState(BG_WS_STATE_TIMER, 25 - _minutesElapsed);
    }
    else
        _timerUpdateWorldState -= diff;


    if (_flagState[BG_TEAM_ALLIANCE] == BG_WS_FLAG_STATE_WAIT_RESPAWN)
    {
        _flagsTimer[BG_TEAM_ALLIANCE] -= diff;

        if (_flagsTimer[BG_TEAM_ALLIANCE] < 0)
        {
            _flagsTimer[BG_TEAM_ALLIANCE] = 0;
            RespawnFlag(BG_TEAM_ALLIANCE, true);
        }
    }

    if (_flagState[BG_TEAM_ALLIANCE] == BG_WS_FLAG_STATE_ON_GROUND)
    {
        _flagsDropTimer[BG_TEAM_ALLIANCE] -= diff;

        if (_flagsDropTimer[BG_TEAM_ALLIANCE] < 0)
        {
            _flagsDropTimer[BG_TEAM_ALLIANCE] = 0;
            RespawnFlagAfterDrop(BG_TEAM_ALLIANCE);
            _bothFlagsKept = false;
        }
    }

    if (_flagState[BG_TEAM_HORDE] == BG_WS_FLAG_STATE_WAIT_RESPAWN)
    {
        _flagsTimer[BG_TEAM_HORDE] -= diff;

        if (_flagsTimer[BG_TEAM_HORDE] < 0)
        {
            _flagsTimer[BG_TEAM_HORDE] = 0;
            RespawnFlag(BG_TEAM_HORDE, true);
        }
    }

    if (_flagState[BG_TEAM_HORDE] == BG_WS_FLAG_STATE_ON_GROUND)
    {
        _flagsDropTimer[BG_TEAM_HORDE] -= diff;

        if (_flagsDropTimer[BG_TEAM_HORDE] < 0)
        {
            _flagsDropTimer[BG_TEAM_HORDE] = 0;
            RespawnFlagAfterDrop(BG_TEAM_HORDE);
            _bothFlagsKept = false;
        }
    }

    if (_bothFlagsKept)
    {
        _flagSpellForceTimer += diff;
        if (_flagDebuffState == 0 && _flagSpellForceTimer >= 600000)  //10 minutes
        {
            if (Player* plr = sObjectMgr->GetPlayer(_flagKeepers[0]))
                plr->CastSpell(plr, WS_SPELL_FOCUSED_ASSAULT, true);
            if (Player* plr = sObjectMgr->GetPlayer(_flagKeepers[1]))
                plr->CastSpell(plr, WS_SPELL_FOCUSED_ASSAULT, true);

            _flagDebuffState = 1;
        }
        else if (_flagDebuffState == 1 && _flagSpellForceTimer >= 900000) //15 minutes
        {
            if (Player* plr = sObjectMgr->GetPlayer(_flagKeepers[0]))
            {
                plr->RemoveAurasDueToSpell(WS_SPELL_FOCUSED_ASSAULT);
                plr->CastSpell(plr, WS_SPELL_BRUTAL_ASSAULT, true);
            }
            if (Player* plr = sObjectMgr->GetPlayer(_flagKeepers[1]))
            {
                plr->RemoveAurasDueToSpell(WS_SPELL_FOCUSED_ASSAULT);
                plr->CastSpell(plr, WS_SPELL_BRUTAL_ASSAULT, true);
            }
            _flagDebuffState = 2;
        }
    }
    else
    {
        _flagSpellForceTimer = 0; //reset timer.
        _flagDebuffState = 0;
    }
}

void BattlegroundWS::OnPlayerJoin(Player *plr)
{
    BattlegroundMap::OnPlayerJoin(plr);

    //create score and add it to map, default values are set in constructor
    BattlegroundWGScore* sc = new BattlegroundWGScore;

    PlayerScores[plr->GetGUIDLow()] = sc;
}

void BattlegroundWS::OnPlayerExit(Player* player)
{
    BattlegroundMap::OnPlayerExit(player);

    // sometimes flag aura not removed :(
    if (IsAllianceFlagPickedup() && _flagKeepers[BG_TEAM_ALLIANCE] == player->GetGUID())
    {
        if (!player)
        {
            sLog->outError("BattlegroundWS: Removing offline player who has the FLAG!!");
            SetAllianceFlagPicker(0);
            RespawnFlag(BG_TEAM_ALLIANCE, false);
        }
        else
            EventPlayerDroppedFlag(player);
    }

    if (IsHordeFlagPickedup() && _flagKeepers[BG_TEAM_HORDE] == player->GetGUID())
    {
        if (!player)
        {
            sLog->outError("BattlegroundWS: Removing offline player who has the FLAG!!");
            SetHordeFlagPicker(0);
            RespawnFlag(BG_TEAM_HORDE, false);
        }
        else
            EventPlayerDroppedFlag(player);
    }

    player->RemoveAurasDueToSpell(BG_WS_SPELL_FOCUSED_ASSAULT);
    player->RemoveAurasDueToSpell(BG_WS_SPELL_BRUTAL_ASSAULT);
}

void BattlegroundWS::OnPlayerKill(Player* player, Player* killer)
{
    if (GetStatus() != STATUS_IN_PROGRESS)
        return;

    EventPlayerDroppedFlag(player);

    BattlegroundMap::HandleKillPlayer(player, killer);
}

void BattlegroundWS::OnTimeoutReached()
{
    if (GetTeamScore(BG_TEAM_ALLIANCE) == 0)
    {
        if (GetTeamScore(TEAM_HORDE) == 0)          // No one scored - result is tie
            EndBattleground(WINNER_NONE);
        else                                        // Horde has more points and thus wins
            EndBattleground(TEAM_HORDE);
    }

    else if (GetTeamScore(TEAM_HORDE) == 0)
        EndBattleground(BG_TEAM_ALLIANCE);             // Alliance has > 0, Horde has 0, alliance wins

    else if (GetTeamScore(TEAM_HORDE) == GetTeamScore(BG_TEAM_ALLIANCE)) // Team score equal, winner is team that scored the last flag
        EndBattleground(_lastFlagCaptureTime);

    else if (GetTeamScore(TEAM_HORDE) > GetTeamScore(BG_TEAM_ALLIANCE))  // Last but not least, check who has the higher score
        EndBattleground(TEAM_HORDE);
    else
        EndBattleground(BG_TEAM_ALLIANCE);
}

void BattlegroundWS::RespawnFlag(uint32 Team, bool captured)
{
    if (Team == BG_TEAM_ALLIANCE)
    {
        sLog->outDebug(LOG_FILTER_BATTLEGROUND, "Respawn Alliance flag");
        _flagState[BG_TEAM_ALLIANCE] = BG_WS_FLAG_STATE_ON_BASE;
    }
    else
    {
        sLog->outDebug(LOG_FILTER_BATTLEGROUND, "Respawn Horde flag");
        _flagState[BG_TEAM_HORDE] = BG_WS_FLAG_STATE_ON_BASE;
    }

    if (captured)
    {
        //when map_update will be allowed for battlegrounds this code will be useless
        SpawnGameObject(BG_WS_OBJECT_H_FLAG, RESPAWN_IMMEDIATELY);
        SpawnGameObject(BG_WS_OBJECT_A_FLAG, RESPAWN_IMMEDIATELY);
        SendMessageToAll(LANG_BG_WS_F_PLACED, CHAT_MSG_BG_SYSTEM_NEUTRAL);
        PlaySoundToAll(BG_WS_SOUND_FLAGS_RESPAWNED);        // flag respawned sound...
    }
    _bothFlagsKept = false;
}

void BattlegroundWS::RespawnFlagAfterDrop(uint32 team)
{
    if (GetStatus() != STATUS_IN_PROGRESS)
        return;

    RespawnFlag(team, false);
    if (team == BG_TEAM_ALLIANCE)
    {
        SpawnGameObject(BG_WS_OBJECT_A_FLAG, RESPAWN_IMMEDIATELY);
        SendMessageToAll(LANG_BG_WS_ALLIANCE_FLAG_RESPAWNED, CHAT_MSG_BG_SYSTEM_NEUTRAL);
    }
    else
    {
        SpawnGameObject(BG_WS_OBJECT_H_FLAG, RESPAWN_IMMEDIATELY);
        SendMessageToAll(LANG_BG_WS_HORDE_FLAG_RESPAWNED, CHAT_MSG_BG_SYSTEM_NEUTRAL);
    }

    PlaySoundToAll(BG_WS_SOUND_FLAGS_RESPAWNED);

    GameObject *obj = GetBgMap()->GetGameObject(GetDroppedFlagGUID(team));
    if (obj)
        obj->Delete();
    else
        sLog->outError("unknown droped flag bg, guid: %u", GUID_LOPART(GetDroppedFlagGUID(team)));

    SetDroppedFlagGUID(0, team);
    _bothFlagsKept = false;
}

void BattlegroundWS::EventPlayerCapturedFlag(Player *source)
{
    if (GetStatus() != STATUS_IN_PROGRESS)
        return;

    uint32 winner = 0;

    source->RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_ENTER_PVP_COMBAT);
    if (source->GetTeam() == BG_TEAM_ALLIANCE)
    {
        if (!this->IsHordeFlagPickedup())
            return;
        SetHordeFlagPicker(0);                              // must be before aura remove to prevent 2 events (drop+capture) at the same time
                                                            // horde flag in base (but not respawned yet)
        _flagState[BG_TEAM_HORDE] = BG_WS_FLAG_STATE_WAIT_RESPAWN;
                                                            // Drop Horde Flag from Player
        source->RemoveAurasDueToSpell(BG_WS_SPELL_WARSONG_FLAG);
        if (_flagDebuffState == 1)
          source->RemoveAurasDueToSpell(WS_SPELL_FOCUSED_ASSAULT);
        if (_flagDebuffState == 2)
          source->RemoveAurasDueToSpell(WS_SPELL_BRUTAL_ASSAULT);
        if (GetTeamScore(BG_TEAM_ALLIANCE) < BG_WS_MAX_TEAM_SCORE)
            AddPoint(BG_TEAM_ALLIANCE, 1);
        PlaySoundToAll(BG_WS_SOUND_FLAG_CAPTURED_ALLIANCE);
        RewardReputationToTeam(890, m_ReputationCapture, BG_TEAM_ALLIANCE);
    }
    else
    {
        if (!this->IsAllianceFlagPickedup())
            return;
        SetAllianceFlagPicker(0);                           // must be before aura remove to prevent 2 events (drop+capture) at the same time
                                                            // alliance flag in base (but not respawned yet)
        _flagState[BG_TEAM_ALLIANCE] = BG_WS_FLAG_STATE_WAIT_RESPAWN;
                                                            // Drop Alliance Flag from Player
        source->RemoveAurasDueToSpell(BG_WS_SPELL_SILVERWING_FLAG);
        if (_flagDebuffState == 1)
          source->RemoveAurasDueToSpell(WS_SPELL_FOCUSED_ASSAULT);
        if (_flagDebuffState == 2)
          source->RemoveAurasDueToSpell(WS_SPELL_BRUTAL_ASSAULT);
        if (GetTeamScore(BG_TEAM_HORDE) < BG_WS_MAX_TEAM_SCORE)
            AddPoint(BG_TEAM_HORDE, 1);
        PlaySoundToAll(BG_WS_SOUND_FLAG_CAPTURED_HORDE);
        RewardReputationToTeam(889, m_ReputationCapture, BG_TEAM_HORDE);
    }
    //for flag capture is reward 2 honorable kills
    RewardHonorToTeam(GetBonusHonorFromKill(2), source->GetTeam());

    SpawnGameObject(BG_WS_OBJECT_H_FLAG, BG_WS_FLAG_RESPAWN_TIME);
    SpawnGameObject(BG_WS_OBJECT_A_FLAG, BG_WS_FLAG_RESPAWN_TIME);

    if (source->GetTeam() == BG_TEAM_ALLIANCE)
        SendMessageToAll(LANG_BG_WS_CAPTURED_HF, CHAT_MSG_BG_SYSTEM_ALLIANCE, source);
    else
        SendMessageToAll(LANG_BG_WS_CAPTURED_AF, CHAT_MSG_BG_SYSTEM_HORDE, source);

    UpdateFlagState(source->GetTeam(), 1);                  // flag state none
    UpdateTeamScore(source->GetTeam());
    // only flag capture should be updated
    UpdatePlayerScore(source, SCORE_FLAG_CAPTURES, 1);      // +1 flag captures

    // update last flag capture to be used if teamscore is equal
    SetLastFlagCapture(source->GetTeam());

    if (GetTeamScore(BG_TEAM_ALLIANCE) == BG_WS_MAX_TEAM_SCORE)
        winner = BG_TEAM_ALLIANCE;

    if (GetTeamScore(BG_TEAM_HORDE) == BG_WS_MAX_TEAM_SCORE)
        winner = BG_TEAM_HORDE;

    if (winner)
    {
        UpdateWorldState(BG_WS_FLAG_UNK_ALLIANCE, 0);
        UpdateWorldState(BG_WS_FLAG_UNK_HORDE, 0);
        UpdateWorldState(BG_WS_FLAG_STATE_ALLIANCE, 1);
        UpdateWorldState(BG_WS_FLAG_STATE_HORDE, 1);
        UpdateWorldState(BG_WS_STATE_TIMER_ACTIVE, 0);

        RewardHonorToTeam(BG_WSG_Honor[m_HonorMode][BG_WSG_WIN], winner);
        EndBattleground(winner);
    }
    else
    {
        _flagsTimer[source->GetBGTeam()] = BG_WS_FLAG_RESPAWN_TIME;
    }
}

void BattlegroundWS::EventPlayerDroppedFlag(Player* source)
{
    if (GetStatus() != STATUS_IN_PROGRESS)
    {
        // if not running, do not cast things at the dropper player (prevent spawning the "dropped" flag), neither send unnecessary messages
        // just take off the aura
        if (source->GetBGTeam() == BG_TEAM_ALLIANCE)
        {
            if (!IsHordeFlagPickedup())
                return;
            if (GetHordeFlagPickerGUID() == source->GetGUID())
            {
                SetHordeFlagPicker(0);
                source->RemoveAurasDueToSpell(BG_WS_SPELL_WARSONG_FLAG);
            }
        }
        else
        {
            if (!IsAllianceFlagPickedup())
                return;

            if (GetAllianceFlagPickerGUID() == source->GetGUID())
            {
                SetAllianceFlagPicker(0);
                source->RemoveAurasDueToSpell(BG_WS_SPELL_SILVERWING_FLAG);
            }
        }
        return;
    }

    bool set = false;

    if (source->GetBGTeam() == BG_TEAM_ALLIANCE)
    {
        if (!IsHordeFlagPickedup())
            return;

        if (GetHordeFlagPickerGUID() == source->GetGUID())
        {
            SetHordeFlagPicker(0);
            source->RemoveAurasDueToSpell(BG_WS_SPELL_WARSONG_FLAG);
            if (_flagDebuffState == 1)
              source->RemoveAurasDueToSpell(WS_SPELL_FOCUSED_ASSAULT);
            if (_flagDebuffState == 2)
              source->RemoveAurasDueToSpell(WS_SPELL_BRUTAL_ASSAULT);
            _flagState[BG_TEAM_HORDE] = BG_WS_FLAG_STATE_ON_GROUND;
            source->CastSpell(source, BG_WS_SPELL_WARSONG_FLAG_DROPPED, true);
            set = true;
        }
    }
    else
    {
        if (!IsAllianceFlagPickedup())
            return;

        if (GetAllianceFlagPickerGUID() == source->GetGUID())
        {
            SetAllianceFlagPicker(0);
            source->RemoveAurasDueToSpell(BG_WS_SPELL_SILVERWING_FLAG);
            if (_flagDebuffState == 1)
              source->RemoveAurasDueToSpell(WS_SPELL_FOCUSED_ASSAULT);
            if (_flagDebuffState == 2)
              source->RemoveAurasDueToSpell(WS_SPELL_BRUTAL_ASSAULT);
            _flagState[BG_TEAM_ALLIANCE] = BG_WS_FLAG_STATE_ON_GROUND;
            source->CastSpell(source, BG_WS_SPELL_SILVERWING_FLAG_DROPPED, true);
            set = true;
        }
    }

    if (set)
    {
        source->CastSpell(source, SPELL_RECENTLY_DROPPED_FLAG, true);
        UpdateFlagState(source->GetTeam(), 1);

        if (source->GetBGTeam() == BG_TEAM_ALLIANCE)
        {
            SendMessageToAll(LANG_BG_WS_DROPPED_HF, CHAT_MSG_BG_SYSTEM_HORDE, source);
            UpdateWorldState(BG_WS_FLAG_UNK_HORDE, uint32(-1));
        }
        else
        {
            SendMessageToAll(LANG_BG_WS_DROPPED_AF, CHAT_MSG_BG_SYSTEM_ALLIANCE, source);
            UpdateWorldState(BG_WS_FLAG_UNK_ALLIANCE, uint32(-1));
        }

        _flagsDropTimer[source->GetBGTeam()] = BG_WS_FLAG_DROP_TIME;
    }
}

void BattlegroundWS::EventPlayerClickedOnFlag(Player* souce, GameObject* targetObj)
{
    if (GetStatus() != STATUS_IN_PROGRESS)
        return;

    int32 message_id = 0;
    ChatMsg type = CHAT_MSG_BG_SYSTEM_NEUTRAL;

    //alliance flag picked up from base
    if (souce->GetBGTeam() == TEAM_HORDE && GetFlagState(BG_TEAM_ALLIANCE) == BG_WS_FLAG_STATE_ON_BASE
        && ObjectGUIDsByType[BG_WS_OBJECT_A_FLAG] == targetObj->GetGUID())
    {
        message_id = LANG_BG_WS_PICKEDUP_AF;
        type = CHAT_MSG_BG_SYSTEM_HORDE;
        PlaySoundToAll(BG_WS_SOUND_ALLIANCE_FLAG_PICKED_UP);
        SpawnGameObject(BG_WS_OBJECT_A_FLAG, RESPAWN_ONE_DAY);
        SetAllianceFlagPicker(souce->GetGUID());
        _flagState[BG_TEAM_ALLIANCE] = BG_WS_FLAG_STATE_ON_PLAYER;
        //update world state to show correct flag carrier
        UpdateFlagState(BG_TEAM_HORDE, BG_WS_FLAG_STATE_ON_PLAYER);
        UpdateWorldState(BG_WS_FLAG_UNK_ALLIANCE, 1);
        souce->CastSpell(souce, BG_WS_SPELL_SILVERWING_FLAG, true);
        souce->GetAchievementMgr().StartTimedAchievement(ACHIEVEMENT_TIMED_TYPE_SPELL_TARGET, BG_WS_SPELL_SILVERWING_FLAG_PICKED);
        if (_flagState[1] == BG_WS_FLAG_STATE_ON_PLAYER)
          _bothFlagsKept = true;
    }

    //horde flag picked up from base
    if (souce->GetTeam() == BG_TEAM_ALLIANCE && this->GetFlagState(BG_TEAM_HORDE) == BG_WS_FLAG_STATE_ON_BASE
        && ObjectGUIDsByType[BG_WS_OBJECT_H_FLAG] == targetObj->GetGUID())
    {
        message_id = LANG_BG_WS_PICKEDUP_HF;
        type = CHAT_MSG_BG_SYSTEM_ALLIANCE;
        PlaySoundToAll(BG_WS_SOUND_HORDE_FLAG_PICKED_UP);
        SpawnGameObject(BG_WS_OBJECT_H_FLAG, RESPAWN_ONE_DAY);
        SetHordeFlagPicker(souce->GetGUID());
        _flagState[BG_TEAM_HORDE] = BG_WS_FLAG_STATE_ON_PLAYER;
        //update world state to show correct flag carrier
        UpdateFlagState(BG_TEAM_ALLIANCE, BG_WS_FLAG_STATE_ON_PLAYER);
        UpdateWorldState(BG_WS_FLAG_UNK_HORDE, 1);
        souce->CastSpell(souce, BG_WS_SPELL_WARSONG_FLAG, true);
        souce->GetAchievementMgr().StartTimedAchievement(ACHIEVEMENT_TIMED_TYPE_SPELL_TARGET, BG_WS_SPELL_WARSONG_FLAG_PICKED);
        if (_flagState[0] == BG_WS_FLAG_STATE_ON_PLAYER)
          _bothFlagsKept = true;
    }

    //Alliance flag on ground(not in base) (returned or picked up again from ground!)
    if (GetFlagState(BG_TEAM_ALLIANCE) == BG_WS_FLAG_STATE_ON_GROUND && souce->IsWithinDistInMap(targetObj, 10))
    {
        if (souce->GetBGTeam() == BG_TEAM_ALLIANCE)
        {
            message_id = LANG_BG_WS_RETURNED_AF;
            type = CHAT_MSG_BG_SYSTEM_ALLIANCE;
            UpdateFlagState(BG_TEAM_HORDE, BG_WS_FLAG_STATE_WAIT_RESPAWN);
            RespawnFlag(BG_TEAM_ALLIANCE, false);
            SpawnGameObject(BG_WS_OBJECT_A_FLAG, RESPAWN_IMMEDIATELY);
            PlaySoundToAll(BG_WS_SOUND_FLAG_RETURNED);
            UpdatePlayerScore(souce, SCORE_FLAG_RETURNS, 1);
            _bothFlagsKept = false;
        }
        else
        {
            message_id = LANG_BG_WS_PICKEDUP_AF;
            type = CHAT_MSG_BG_SYSTEM_HORDE;
            PlaySoundToAll(BG_WS_SOUND_ALLIANCE_FLAG_PICKED_UP);
            SpawnGameObject(BG_WS_OBJECT_A_FLAG, RESPAWN_ONE_DAY);
            SetAllianceFlagPicker(souce->GetGUID());
            souce->CastSpell(souce, BG_WS_SPELL_SILVERWING_FLAG, true);
            _flagState[BG_TEAM_ALLIANCE] = BG_WS_FLAG_STATE_ON_PLAYER;
            UpdateFlagState(BG_TEAM_HORDE, BG_WS_FLAG_STATE_ON_PLAYER);
            if (_flagDebuffState == 1)
              souce->CastSpell(souce, WS_SPELL_FOCUSED_ASSAULT, true);
            if (_flagDebuffState == 2)
              souce->CastSpell(souce, WS_SPELL_BRUTAL_ASSAULT, true);
            UpdateWorldState(BG_WS_FLAG_UNK_ALLIANCE, 1);
        }
        //called in HandleGameObjectUseOpcode:
        //target_obj->Delete();
    }

    //Horde flag on ground(not in base) (returned or picked up again)
    if (GetFlagState(TEAM_HORDE) == BG_WS_FLAG_STATE_ON_GROUND && souce->IsWithinDistInMap(targetObj, 10))
    {
        if (souce->GetBGTeam() == TEAM_HORDE)
        {
            message_id = LANG_BG_WS_RETURNED_HF;
            type = CHAT_MSG_BG_SYSTEM_HORDE;
            UpdateFlagState(BG_TEAM_ALLIANCE, BG_WS_FLAG_STATE_WAIT_RESPAWN);
            RespawnFlag(BG_TEAM_HORDE, false);
            SpawnGameObject(BG_WS_OBJECT_H_FLAG, RESPAWN_IMMEDIATELY);
            PlaySoundToAll(BG_WS_SOUND_FLAG_RETURNED);
            UpdatePlayerScore(souce, SCORE_FLAG_RETURNS, 1);
            _bothFlagsKept = false;
        }
        else
        {
            message_id = LANG_BG_WS_PICKEDUP_HF;
            type = CHAT_MSG_BG_SYSTEM_ALLIANCE;
            PlaySoundToAll(BG_WS_SOUND_HORDE_FLAG_PICKED_UP);
            SpawnGameObject(BG_WS_OBJECT_H_FLAG, RESPAWN_ONE_DAY);
            SetHordeFlagPicker(souce->GetGUID());
            souce->CastSpell(souce, BG_WS_SPELL_WARSONG_FLAG, true);
            _flagState[BG_TEAM_HORDE] = BG_WS_FLAG_STATE_ON_PLAYER;
            UpdateFlagState(BG_TEAM_ALLIANCE, BG_WS_FLAG_STATE_ON_PLAYER);
            if (_flagDebuffState == 1)
              souce->CastSpell(souce, WS_SPELL_FOCUSED_ASSAULT, true);
            if (_flagDebuffState == 2)
              souce->CastSpell(souce, WS_SPELL_BRUTAL_ASSAULT, true);
            UpdateWorldState(BG_WS_FLAG_UNK_HORDE, 1);
        }
        //called in HandleGameObjectUseOpcode:
        //target_obj->Delete();
    }

    if (!message_id)
        return;

    SendMessageToAll(message_id, type, souce);
    souce->RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_ENTER_PVP_COMBAT);
}

void BattlegroundWS::UpdateFlagState(uint32 team, uint32 value)
{
    if (team == BG_TEAM_ALLIANCE)
        UpdateWorldState(BG_WS_FLAG_STATE_ALLIANCE, value);
    else
        UpdateWorldState(BG_WS_FLAG_STATE_HORDE, value);
}

void BattlegroundWS::UpdateTeamScore(uint32 team)
{
    if (team == BG_TEAM_ALLIANCE)
        UpdateWorldState(BG_WS_FLAG_CAPTURES_ALLIANCE, GetTeamScore(team));
    else
        UpdateWorldState(BG_WS_FLAG_CAPTURES_HORDE, GetTeamScore(team));
}

void BattlegroundWS::HandleAreaTrigger(Player *Source, uint32 Trigger)
{
    // this is wrong way to implement these things. On official it done by gameobject spell cast.
    if (GetStatus() != STATUS_IN_PROGRESS)
        return;

    //uint32 SpellId = 0;
    //uint64 buff_guid = 0;
    switch(Trigger)
    {
        case 3686:                                          // Alliance elixir of speed spawn. Trigger not working, because located inside other areatrigger, can be replaced by IsWithinDist(object, dist) in Battleground::Update().
            //buff_guid = m_BgObjects[BG_WS_OBJECT_SPEEDBUFF_1];
            break;
        case 3687:                                          // Horde elixir of speed spawn. Trigger not working, because located inside other areatrigger, can be replaced by IsWithinDist(object, dist) in Battleground::Update().
            //buff_guid = m_BgObjects[BG_WS_OBJECT_SPEEDBUFF_2];
            break;
        case 3706:                                          // Alliance elixir of regeneration spawn
            //buff_guid = m_BgObjects[BG_WS_OBJECT_REGENBUFF_1];
            break;
        case 3708:                                          // Horde elixir of regeneration spawn
            //buff_guid = m_BgObjects[BG_WS_OBJECT_REGENBUFF_2];
            break;
        case 3707:                                          // Alliance elixir of berserk spawn
            //buff_guid = m_BgObjects[BG_WS_OBJECT_BERSERKBUFF_1];
            break;
        case 3709:                                          // Horde elixir of berserk spawn
            //buff_guid = m_BgObjects[BG_WS_OBJECT_BERSERKBUFF_2];
            break;
        case 3646:                                          // Alliance Flag spawn
            if (_flagState[BG_TEAM_HORDE] && !_flagState[BG_TEAM_ALLIANCE])
                if (GetHordeFlagPickerGUID() == Source->GetGUID())
                    EventPlayerCapturedFlag(Source);
            break;
        case 3647:                                          // Horde Flag spawn
            if (_flagState[BG_TEAM_ALLIANCE] && !_flagState[BG_TEAM_HORDE])
                if (GetAllianceFlagPickerGUID() == Source->GetGUID())
                    EventPlayerCapturedFlag(Source);
            break;
        case 3649:                                          // unk1
        case 3688:                                          // unk2
        case 4628:                                          // unk3
        case 4629:                                          // unk4
            break;
        default:
            sLog->outError("WARNING: Unhandled AreaTrigger in Battleground: %u", Trigger);
            Source->GetSession()->SendAreaTriggerMessage("Warning: Unhandled AreaTrigger in Battleground: %u", Trigger);
            break;
    }

    //if (buff_guid)
    //    HandleTriggerBuff(buff_guid, Source);
}

void BattlegroundWS::UpdatePlayerScore(Player *Source, uint32 type, uint32 value, bool doAddHonor)
{
    BattlegroundScoreMap::iterator itr = PlayerScores.find(Source->GetGUIDLow());
    if (itr == PlayerScores.end())                         // player not found
        return;

    switch (type)
    {
        case SCORE_FLAG_CAPTURES:                           // flags captured
            ((BattlegroundWGScore*)itr->second)->FlagCaptures += value;
            Source->GetAchievementMgr().UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_BG_OBJECTIVE_CAPTURE, WS_OBJECTIVE_CAPTURE_FLAG);
            break;
        case SCORE_FLAG_RETURNS:                            // flags returned
            ((BattlegroundWGScore*)itr->second)->FlagReturns += value;
            Source->GetAchievementMgr().UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_BG_OBJECTIVE_CAPTURE, WS_OBJECTIVE_RETURN_FLAG);
            break;
        default:
            BattlegroundMap::UpdatePlayerScore(Source, type, value, doAddHonor);
            break;
    }
}

WorldSafeLocsEntry const* BattlegroundWS::GetClosestGraveYard(Player* player)
{
    //if status in progress, it returns main graveyards with spiritguides
    //else it will return the graveyard in the flagroom - this is especially good
    //if a player dies in preparation phase - then the player can't cheat
    //and teleport to the graveyard outside the flagroom
    //and start running around, while the doors are still closed
    if (player->GetTeam() == BG_TEAM_ALLIANCE)
    {
        if (GetStatus() == STATUS_IN_PROGRESS)
            return sWorldSafeLocsStore.LookupEntry(WS_GRAVEYARD_MAIN_ALLIANCE);
        else
            return sWorldSafeLocsStore.LookupEntry(WS_GRAVEYARD_FLAGROOM_ALLIANCE);
    }
    else
    {
        if (GetStatus() == STATUS_IN_PROGRESS)
            return sWorldSafeLocsStore.LookupEntry(WS_GRAVEYARD_MAIN_HORDE);
        else
            return sWorldSafeLocsStore.LookupEntry(WS_GRAVEYARD_FLAGROOM_HORDE);
    }
}

void BattlegroundWS::FillInitialWorldStates(WorldPacket& data)
{
    data << uint32(BG_WS_FLAG_CAPTURES_ALLIANCE) << uint32(GetTeamScore(BG_TEAM_ALLIANCE));
    data << uint32(BG_WS_FLAG_CAPTURES_HORDE) << uint32(GetTeamScore(BG_TEAM_HORDE));

    if (_flagState[BG_TEAM_ALLIANCE] == BG_WS_FLAG_STATE_ON_GROUND)
        data << uint32(BG_WS_FLAG_UNK_ALLIANCE) << uint32(-1);
    else if (_flagState[BG_TEAM_ALLIANCE] == BG_WS_FLAG_STATE_ON_PLAYER)
        data << uint32(BG_WS_FLAG_UNK_ALLIANCE) << uint32(1);
    else
        data << uint32(BG_WS_FLAG_UNK_ALLIANCE) << uint32(0);

    if (_flagState[BG_TEAM_HORDE] == BG_WS_FLAG_STATE_ON_GROUND)
        data << uint32(BG_WS_FLAG_UNK_HORDE) << uint32(-1);
    else if (_flagState[BG_TEAM_HORDE] == BG_WS_FLAG_STATE_ON_PLAYER)
        data << uint32(BG_WS_FLAG_UNK_HORDE) << uint32(1);
    else
        data << uint32(BG_WS_FLAG_UNK_HORDE) << uint32(0);

    data << uint32(BG_WS_FLAG_CAPTURES_MAX) << uint32(BG_WS_MAX_TEAM_SCORE);

    if (GetStatus() == STATUS_IN_PROGRESS)
    {
        data << uint32(BG_WS_STATE_TIMER_ACTIVE) << uint32(1);
        data << uint32(BG_WS_STATE_TIMER) << uint32(25-_minutesElapsed);
    }
    else
        data << uint32(BG_WS_STATE_TIMER_ACTIVE) << uint32(0);

    if (_flagState[BG_TEAM_HORDE] == BG_WS_FLAG_STATE_ON_PLAYER)
        data << uint32(BG_WS_FLAG_STATE_ALLIANCE) << uint32(2);
    else
        data << uint32(BG_WS_FLAG_STATE_ALLIANCE) << uint32(1);

    if (_flagState[BG_TEAM_ALLIANCE] == BG_WS_FLAG_STATE_ON_PLAYER)
        data << uint32(BG_WS_FLAG_STATE_HORDE) << uint32(2);
    else
        data << uint32(BG_WS_FLAG_STATE_HORDE) << uint32(1);

}

void BattlegroundWS::InitializeTextIds()
{
    PreparationPhaseTextIds[BG_STARTING_EVENT_FIRST]  = LANG_BG_WS_START_TWO_MINUTES;
    PreparationPhaseTextIds[BG_STARTING_EVENT_SECOND] = LANG_BG_WS_START_ONE_MINUTE;
    PreparationPhaseTextIds[BG_STARTING_EVENT_THIRD]  = LANG_BG_WS_START_HALF_MINUTE;
    PreparationPhaseTextIds[BG_STARTING_EVENT_FOURTH] = LANG_BG_WS_HAS_BEGUN;
}

