#include "ScriptMgr.h"
#include "Player.h"
#include "World.h"
#include "Config.h"
#include "Map.h"
#include "Log.h"
#include <vector>
#include <string>
#include <sstream>
#include <algorithm>

namespace FlyMountsEverywhere
{
    static bool sEnabled = true;
    static bool sBlockBGs = true;
    static bool sBlockInstances = true;
    static uint8 sMinLevel = 20;
    static bool sDebug = false;
    static std::vector<uint32> sAllowedMounts;

    static bool AllowedHere(Player* player)
    {
        if (!sEnabled || !player || !player->IsInWorld())
            return false;

        if (player->getLevel() < sMinLevel)
            return false;

        Map* map = player->GetMap();
        if (!map)
            return false;

        if (sBlockBGs && map->IsBattlegroundOrArena())
            return false;

        if (sBlockInstances && (map->IsDungeon() || map->IsRaid()))
            return false;

        return true;
    }

    static bool IsAllowedMount(Player* player)
    {
        if (!player->IsMounted())
            return false;

        uint32 mountSpellId = player->GetMountSpellId();
        if (!mountSpellId)
            return false;

        if (sAllowedMounts.empty())
            return player->HasAuraType(SPELL_AURA_MOD_INCREASE_MOUNTED_FLIGHT_SPEED);

        if (std::find(sAllowedMounts.begin(), sAllowedMounts.end(), mountSpellId) != sAllowedMounts.end())
            return player->HasAuraType(SPELL_AURA_MOD_INCREASE_MOUNTED_FLIGHT_SPEED);

        return false;
    }

    static void UpdateFlight(Player* player)
    {
        if (!player)
            return;

        if (AllowedHere(player) && IsAllowedMount(player))
        {
            if (!player->CanFly())
            {
                player->SetCanFly(true);
                if (sDebug)
                    LOG_INFO("module", "[FlyingMountsEverywhere] Flight ENABLED for {} (spell {})",
                             player->GetName(), player->GetMountSpellId());
            }
            return;
        }

        if (player->CanFly())
        {
            player->SetCanFly(false);
            if (sDebug)
                LOG_INFO("module", "[FlyingMountsEverywhere] Flight DISABLED for {}",
                         player->GetName());
        }
    }

    static void ParseAllowedMounts(std::string const& csv)
    {
        sAllowedMounts.clear();
        std::stringstream ss(csv);
        std::string item;
        while (std::getline(ss, item, ','))
        {
            try
            {
                uint32 id = static_cast<uint32>(std::stoul(item));
                sAllowedMounts.push_back(id);
            }
            catch (...) { }
        }
    }

    class WorldScript_FlyingMountsEverywhere : public WorldScript
    {
    public:
        WorldScript_FlyingMountsEverywhere() : WorldScript("WorldScript_FlyingMountsEverywhere") { }

        void OnAfterConfigLoad(bool /*reload*/) override
        {
            sEnabled        = sConfigMgr->GetOption<bool>("mod_flying_mounts_everywhere.Enabled", true);
            sBlockBGs       = sConfigMgr->GetOption<bool>("mod_flying_mounts_everywhere.BlockInBattlegrounds", true);
            sBlockInstances = sConfigMgr->GetOption<bool>("mod_flying_mounts_everywhere.BlockInInstances", true);
            sMinLevel       = static_cast<uint8>(sConfigMgr->GetOption<int32>("mod_flying_mounts_everywhere.MinLevel", 20));
            sDebug          = sConfigMgr->GetOption<bool>("mod_flying_mounts_everywhere.Debug", false);

            std::string allowed = sConfigMgr->GetOption<std::string>("mod_flying_mounts_everywhere.AllowedMountSpells", "");
            ParseAllowedMounts(allowed);
        }
    };

    class PlayerScript_FlyingMountsEverywhere : public PlayerScript
    {
    public:
        PlayerScript_FlyingMountsEverywhere() : PlayerScript("PlayerScript_FlyingMountsEverywhere") { }

        void OnLogin(Player* player) override { UpdateFlight(player); }
        void OnMapChanged(Player* player) override { UpdateFlight(player); }
        void OnUpdateZone(Player* player, uint32, uint32) override { UpdateFlight(player); }
        void OnMount(Player* player, uint32) override { UpdateFlight(player); }
        void OnDismount(Player* player) override { UpdateFlight(player); }
        void OnUpdate(Player* player, uint32) override { UpdateFlight(player); }
    };
}

void Addmod_flying_mounts_everywhereScripts()
{
    new FlyMountsEverywhere::WorldScript_FlyingMountsEverywhere();
    new FlyMountsEverywhere::PlayerScript_FlyingMountsEverywhere();
}
