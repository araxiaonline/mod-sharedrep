#include "Chat.h"
#include "DatabaseEnv.h"
#include "Log.h"
#include "Player.h"
#include "ReputationMgr.h"
#include "ScriptMgr.h"

// sporeggar 45499
// sons of hodir 84999

class SharedRepPlayerScript : public PlayerScript
{
    public:
        SharedRepPlayerScript() : PlayerScript("SharedRepPlayerScript") { }


        void OnLogin(Player* player) override
        {
            Msg(player, "This server is running the |cff4CFF00ShareRep |rmodule.");

            uint32 accountId = player->GetSession()->GetAccountId();
            ObjectGuid playerGuid = player->GetGUID();
            std::string hordeInClause = "(2, 5, 6, 8, 10)";
            std::string allianceInClause = "(1, 3, 4, 7, 11)";
            std::string factionClause = hordeInClause;

            uint32 factionAldor = 932;
            uint32 factionScryer = 934;
            uint32 factionSporeggar = 970;
            uint32 factionSonsofhodir = 1119;

            uint32 playerStandingAldor = 0;
            uint32 playerStandingScryer = 0;
            uint32 accountStandingAldor = 0;
            uint32 accountStandingScryer = 0;

            std::string playerDetails = " for " + player->GetName() + "(" + std::to_string(playerGuid.GetRawValue()) + ")";

            FactionEntry const* factionEntryAldor = sFactionStore.LookupEntry(factionAldor);
            if (!factionEntryAldor)
            {
                LogDebug("Faction not found: " + std::to_string(factionAldor) + playerDetails);
            } else {
                playerStandingAldor = player->GetReputationMgr().GetReputation(factionEntryAldor);
            }

            FactionEntry const* factionEntryScryer = sFactionStore.LookupEntry(factionScryer);
            if (!factionEntryScryer)
            {
                LogDebug("Faction not found: " + std::to_string(factionScryer) + playerDetails);
            } else {
                playerStandingScryer = player->GetReputationMgr().GetReputation(factionEntryScryer);
            }

            if (player->GetTeamId() == TEAM_ALLIANCE)
            {
                factionClause = allianceInClause;
            }

            std::string query = R"(
                SELECT
                    guid
                    , faction
                    , MAX(standing)
                FROM acore_characters.character_reputation
                WHERE standing != 0
                AND GUID IN (
                    SELECT
                        guid
                    FROM characters
                    WHERE
                        account = {}
                        AND guid != {}
                        AND race IN {}
                )
                GROUP BY faction;
            )";

            QueryResult result = CharacterDatabase.Query(
                query,
                accountId,
                playerGuid.GetRawValue(),
                factionClause
            );

            if (!result)
            {
                LOG_DEBUG("sharedrep",
                    "Failed to query reputation for player " + player->GetName()
                );
                return;
            }

            do
            {
                Field* row = result->Fetch();

                // this is the specific rep info that is max for the current account.
                uint32 repGuid = row[0].Get<uint32>();
                uint32 repFaction = row[1].Get<uint32>();
                uint32 repStanding = row[2].Get<uint32>();

                if (repGuid == playerGuid.GetRawValue()) continue;

                if (repFaction == factionAldor)
                {
                    accountStandingAldor = repStanding;
                    continue;
                }

                if (repFaction == factionScryer)
                {
                    accountStandingScryer = repStanding;
                    continue;
                }

                FactionEntry const* factionEntry = sFactionStore.LookupEntry(repFaction);

                if (!factionEntry)
                {
                    LogDebug("Faction not found: " + std::to_string(repFaction) + playerDetails);
                    continue;
                }

                std::string factionName = std::string(factionEntry->name[LOCALE_enUS]);

                float currentStanding = player->GetReputationMgr().GetReputation(factionEntry);
                if (repStanding <= currentStanding)
                {
                    LogDebug("Standing not higher for "
                        + factionName  + " (" + std::to_string(repFaction) + ")"
                        + " " + std::to_string(currentStanding) + " >= " + std::to_string(repStanding)
                        + playerDetails);
                    continue;
                }

                // These factions seem to cap lower than most of the others. Not sure why
                // but this keeps things somewhat safe regardless of wonky db values.
                if (repFaction == factionSporeggar || repFaction == factionSonsofhodir)
                {
                    if (repStanding > 42999)
                    {
                        repStanding = 42999;
                    }
                } else {
                    if (repStanding > 45999)
                    {
                        repStanding = 45999;
                    }
                }

                player->GetReputationMgr().SetOneFactionReputation(factionEntry, float(repStanding), false);
                player->GetReputationMgr().SendState(player->GetReputationMgr().GetState(factionEntry));
                LogDebug("Reputation increased for "
                    + factionName
                    + " from " + std::to_string(currentStanding)
                    + " to " + std::to_string(repStanding) + playerDetails);

                Msg(player, "Reputation increased for "
                    + factionName
                    + " to " + std::to_string(repStanding));

            } while (result->NextRow());

        }

        void Msg(Player* player, std::string msg)
        {
            ChatHandler(player->GetSession()).SendSysMessage(msg);
        }

        void LogDebug(std::string msg)
        {
            LOG_DEBUG("sharedrep", msg);
        }
};

void AddSharedRepScripts()
{
    new SharedRepPlayerScript();
};
