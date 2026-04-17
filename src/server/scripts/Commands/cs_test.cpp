/*
 * This file is part of the DestinyCore Project. See AUTHORS file for Copyright information
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

#include "ScriptMgr.h"
#include "Chat.h"
#include "AreaTrigger.h"
#include "Vehicle.h"
#include "EventObject.h"

class test_commandscript : public CommandScript
{
public:
    test_commandscript() : CommandScript("test_commandscript") { }

    std::vector<ChatCommand> GetCommands() const override
    {
        static std::vector<ChatCommand> TestTable =
        {
            { "summon",             SEC_GAMEMASTER,         true,  &HandleTestSummon,                   ""},
            { "",                   SEC_GAMEMASTER,         true,  &HandleTestSummon,                   ""}
        };

        static std::vector<ChatCommand> CommandTable =
        {
             { "test",                  SEC_ADMINISTRATOR,  true,   nullptr,                        "", TestTable },
             { "sizeof",                SEC_GAMEMASTER,     false,  &HandleSizeofCommand,           ""}
        };

        return CommandTable;
    }

    static bool HandleTestSummon(ChatHandler* handler, char const* args)
    {
        if (!*args)
            return false;

        char* countStr = strtok((char*)args, " ");
        uint32 count = (uint32)atoi(countStr);
        uint32 summoned = 0;

        for (uint32 i = 0; i <= count; ++i)
            if (handler->GetSession()->GetPlayer()->SummonCreature(44548, 16226.2f, 16257.0f, 13.20f, 1.65f))
                summoned++;

        handler->PSendSysMessage("TestSummon count %u summoned %u", count, summoned);
        return true;
    }

    static bool HandleSizeofCommand(ChatHandler* handler, char const* args)
    {
        Player* _player = handler->GetSession()->GetPlayer();
        _player->PrintPlayerSize();

        if (Unit* target = handler->getSelectedUnit())
            if (Creature* creature = target->ToCreature())
                creature->PrintCreatureSize(_player);

        handler->PSendSysMessage("Object size %u", sizeof(Object));
        handler->PSendSysMessage("WorldObject size %u", sizeof(WorldObject));
        handler->PSendSysMessage("Unit size %u", sizeof(Unit));
        handler->PSendSysMessage("Player size %u", sizeof(Player));
        handler->PSendSysMessage("Pet size %u", sizeof(Pet));
        handler->PSendSysMessage("Creature size %u", sizeof(Creature));
        handler->PSendSysMessage("Corpse size %u", sizeof(Corpse));
        handler->PSendSysMessage("GameObject size %u", sizeof(GameObject));
        handler->PSendSysMessage("DynamicObject size %u", sizeof(DynamicObject));
        handler->PSendSysMessage("Transport size %u", sizeof(Transport));
        handler->PSendSysMessage("AreaTrigger size %u", sizeof(AreaTrigger));
        handler->PSendSysMessage("Conversation size %u", sizeof(Conversation));
        handler->PSendSysMessage("EventObject size %u", sizeof(EventObject));
        handler->PSendSysMessage("LootItem size %u", sizeof(LootItem));
        handler->PSendSysMessage("Loot size %u", sizeof(Loot));
        handler->PSendSysMessage("Vehicle size %u", sizeof(Vehicle));
        handler->PSendSysMessage("Item size %u", sizeof(Item));
        handler->PSendSysMessage("Vector for Unit size %u", sizeof(std::vector<Unit*>));
        handler->PSendSysMessage("Vector for Unit size %u", sizeof(std::vector<uint32>));
        std::vector<Unit*> testList;
        testList.resize(1000);
        handler->PSendSysMessage("Vector for Unit size %u", sizeof(testList));

        return true;
    }
};

void AddSC_test_commandscript()
{
    new test_commandscript();
}
