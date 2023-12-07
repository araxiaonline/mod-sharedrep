#include "Chat.h"
#include "DatabaseEnv.h"
#include "Log.h"
#include "Player.h"
#include "ReputationMgr.h"
#include "ScriptMgr.h"


class SharedRepPlayerScript : public PlayerScript
{
    public:
        SharedRepPlayerScript() : PlayerScript("SharedRepPlayerScript") { }


        void OnLogin(Player* player) override
        {

            ChatHandler(player->GetSession()).SendSysMessage("This server is running the ShareRep module.");

            LOG_INFO("sharedrep", "OnLogin");
            uint32 accountId = player->GetSession()->GetAccountId();
            ObjectGuid playerGuid = player->GetGUID();
            std::string hordeInClause = "(2, 5, 6, 8, 10)";
            std::string allianceInClause = "(1, 3, 4, 7, 11)";

            uint32 faction_aldor = 932;
            uint32 faction_scryer = 934;
            uint32 faction_sporeggar = 970;
            uint32 faction_theashenverdict = 1119;

            std::string factionClause = hordeInClause;


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

            float standingAldor = 0;
            float standingScryer = 0;

            do
            {
                Field* row = result->Fetch();

                uint32 repGuid = row[0].Get<uint32>();
                uint32 repFaction = row[1].Get<uint32>();
                float repStanding = row[2].Get<float>();

                if (repGuid == playerGuid.GetRawValue()) continue;

                if (repFaction == faction_aldor)
                {
                    standingAldor = repStanding;
                    continue;
                }

                if (repFaction == faction_scryer)
                {
                    standingScryer = repStanding;
                    continue;
                }

                FactionEntry const* factionEntry = sFactionStore.LookupEntry(repFaction);

                if (!factionEntry)
                {
                    LOG_DEBUG("sharedrep", "Faction not found: " + std::to_string(repFaction));
                    continue;
                }

                float currentStanding = player->GetReputationMgr().GetReputation(factionEntry);
                if (repStanding <= currentStanding)
                {
                    LOG_DEBUG("sharedrep",
                        "Standing not higher for faction(" + std::to_string(repFaction) + ")");
                    continue;
                }

                player->GetReputationMgr().SetOneFactionReputation(factionEntry, float(repStanding), false);
                player->GetReputationMgr().SendState(player->GetReputationMgr().GetState(factionEntry));
                LOG_DEBUG("sharedrep",
                    "Faction(" + std::to_string(repFaction) + ") reputation set: "
                    + std::to_string(repStanding)
                );

            } while (result->NextRow());

        }
};

void AddSharedRepScripts()
{
    new SharedRepPlayerScript();
};
