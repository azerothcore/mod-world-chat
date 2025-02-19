/*
<--------------------------------------------------------------------------->
- Developer(s): WiiZZy
- Complete: 100%
- ScriptName: 'World chat'
- Comment: Fully tested
<--------------------------------------------------------------------------->
*/

#include "ScriptMgr.h"
#include "Log.h"
#include "Player.h"
#include "Channel.h"
#include "Chat.h"
#include "Common.h"
#include "World.h"
#include "WorldSession.h"
#include "WorldSessionMgr.h"
#include "Config.h"
#include <unordered_map>

#if AC_COMPILER == AC_COMPILER_GNU
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

/* VERSION */
float ver = 2.0f;

/* Colors */
std::string WORLD_CHAT_ALLIANCE_BLUE = "|cff3399FF";
std::string WORLD_CHAT_HORDE_RED = "|cffCC0000";
std::string WORLD_CHAT_WHITE = "|cffFFFFFF";
std::string WORLD_CHAT_GREEN = "|cff00CC00";
std::string WORLD_CHAT_RED = "|cffFF0000";
std::string WORLD_CHAT_BLUE = "|cff6666FF";
std::string WORLD_CHAT_BLACK = "|cff000000";
std::string WORLD_CHAT_GREY = "|cff808080";

/* Class Colors */
std::string world_chat_ClassColor[11] =
{
    "|cffC79C6E", // WARRIOR
    "|cffF58CBA", // PALADIN
    "|cffABD473", // HUNTER
    "|cffFFF569", // ROGUE
    "|cffFFFFFF", // PRIEST
    "|cffC41F3B", // DEATHKNIGHT
    "|cff0070DE", // SHAMAN
    "|cff40C7EB", // MAGE
    "|cff8787ED", // WARLOCK
    "", // ADDED IN MOP FOR MONK - NOT USED
    "|cffFF7D0A", // DRUID
};

/* Ranks */
std::string world_chat_GM_RANKS[4] =
{
    "Player",
    "MOD",
    "GM",
    "ADMIN",
};

/* BLIZZARD CHAT ICON FOR GM'S */
std::string world_chat_GMIcon = "|TINTERFACE/CHATFRAME/UI-CHATICON-BLIZZ:13:13:0:-1|t";

/* COLORED TEXT FOR CURRENT FACTION || NOT FOR GMS */
std::string world_chat_TeamIcon[2] =
{
    "|cff3399FFAlliance|r",
    "|cffCC0000Horde|r"
};

/* Config Variables */
struct WCConfig
{
    bool Enabled;
    std::string ChannelName;
    bool LoginState;
    bool CrossFaction;
    bool Announce;
};

WCConfig WC_Config;

class WorldChat_Config : public WorldScript
{
public: WorldChat_Config() : WorldScript("WorldChat_Config") { };
    void OnBeforeConfigLoad(bool reload) override
    {
        if (!reload) {
            WC_Config.Enabled = sConfigMgr->GetOption<bool>("World_Chat.Enable", true);
            WC_Config.ChannelName = sConfigMgr->GetOption<std::string>("World_Chat.ChannelName", "World");
            WC_Config.LoginState = sConfigMgr->GetOption<bool>("World_Chat.OnLogin.State", true);
            WC_Config.CrossFaction = sConfigMgr->GetOption<bool>("World_Chat.CrossFactions", true);
            WC_Config.Announce = sConfigMgr->GetOption<bool>("World_Chat.Announce", true);
        }
    }
};

/* STRUCTURE FOR WorldChat map */
struct ChatElements
{
    uint8 chat = (WC_Config.LoginState) ? 1 : 0; // CHAT DISABLED BY DEFAULT
};

/* UNORDERED MAP FOR STORING IF CHAT IS ENABLED OR DISABLED */
std::unordered_map<uint32, ChatElements>WorldChat;

void SendWorldMessage(Player* sender, const std::string msg, int team) {

    if (!WC_Config.Enabled)
    {
        ChatHandler(sender->GetSession()).PSendSysMessage(Acore::StringFormat("[WC] {}World Chat System is disabled.|r", WORLD_CHAT_RED));
        return;
    }

    if (!sender->CanSpeak())
    {
        ChatHandler(sender->GetSession()).PSendSysMessage(Acore::StringFormat("[WC] {}You can't use World Chat while muted!|r", WORLD_CHAT_RED));
        return;
    }

    if (!WorldChat[sender->GetGUID().GetCounter()].chat)
    {
        ChatHandler(sender->GetSession()).PSendSysMessage(Acore::StringFormat("[WC] {}World Chat is hidden. (.chat off)|r", WORLD_CHAT_RED));
        return;
    }

    std::string message;

    WorldSessionMgr::SessionMap sessions = sWorldSessionMgr->GetAllSessions();

    for (WorldSessionMgr::SessionMap::iterator itr = sessions.begin(); itr != sessions.end(); ++itr)
    {
        if (!itr->second)
        {
            continue;
        }

        if (!itr->second->GetPlayer())
        {
            continue;
        }

        if (!itr->second->GetPlayer()->IsInWorld())
        {
            continue;
        }

        Player* target = itr->second->GetPlayer();
        uint64 guid2 = target->GetGUID().GetCounter();

        if (WorldChat[guid2].chat == 1 && (team == -1 || target->GetTeamId() == team))
        {
            if (WC_Config.CrossFaction || (sender->GetTeamId() == target->GetTeamId()) || target->GetSession()->GetSecurity())
            {
                if (sender->isGMChat())
                {
                    message = Acore::StringFormat("[World][{}][{}|Hplayer:{}|h{}|h|r]: {}{}|r", ((sender->HasFlag(PLAYER_FLAGS, PLAYER_FLAGS_DEVELOPER)) ? (world_chat_ClassColor[5] + "DEV|r") : world_chat_GMIcon) , world_chat_ClassColor[sender->getClass() - 1], sender->GetName(), sender->GetName(), WORLD_CHAT_WHITE, msg);
                }
                else
                {
                    message =  Acore::StringFormat("[World][{}][{}|Hplayer:{}|h{}|h|r]: {}{}|r", world_chat_TeamIcon[sender->GetTeamId()], world_chat_ClassColor[sender->getClass() - 1], sender->GetName(), sender->GetName(), WORLD_CHAT_WHITE, msg);
                }
                ChatHandler(target->GetSession()).PSendSysMessage(Acore::StringFormat("{}", message));
            }
        }
    }
}

using namespace Acore::ChatCommands;

class World_Chat : public CommandScript
{
public:
    World_Chat() : CommandScript("World_Chat") { }

    static bool HandleWorldChatCommand(ChatHandler * pChat, const char * msg)
    {

        if (!*msg)
        {
            return false;
        }

        SendWorldMessage(pChat->GetSession()->GetPlayer(), msg, -1);

        return true;
    }

    static bool HandleWorldChatHordeCommand(ChatHandler * pChat, const char * msg)
    {

        if (!*msg)
        {
            return false;
        }

        SendWorldMessage(pChat->GetSession()->GetPlayer(), msg, TEAM_HORDE);

        return true;
    }

    static bool HandleWorldChatAllianceCommand(ChatHandler * pChat, const char * msg)
    {

        if (!*msg)
        {
            return false;
        }

        SendWorldMessage(pChat->GetSession()->GetPlayer(), msg, TEAM_ALLIANCE);

        return true;
    }

    static bool HandleWorldChatOnCommand(ChatHandler* handler, const char* /*msg*/)
    {
        Player* player = handler->GetSession()->GetPlayer();
        uint64 guid = player->GetGUID().GetCounter();

        if (!WC_Config.Enabled) {
            ChatHandler(player->GetSession()).PSendSysMessage(Acore::StringFormat("[WC] {}World Chat System is disabled.|r", WORLD_CHAT_RED));
            return true;
        }

        if (WorldChat[guid].chat) {
            ChatHandler(player->GetSession()).PSendSysMessage(Acore::StringFormat("[WC] {}World Chat is already visible.|r", WORLD_CHAT_RED));
            return true;
        }

        WorldChat[guid].chat = 1;

        ChatHandler(player->GetSession()).PSendSysMessage(Acore::StringFormat("[WC] {}World Chat is now visible.|r", WORLD_CHAT_GREEN));

        return true;
    };

    static bool HandleWorldChatOffCommand(ChatHandler* handler, const char* /*msg*/)
    {
        Player* player = handler->GetSession()->GetPlayer();
        uint64 guid = player->GetGUID().GetCounter();

        if (!sConfigMgr->GetOption<bool>("World_Chat.Enable", true))
        {
            ChatHandler(player->GetSession()).PSendSysMessage(Acore::StringFormat("[WC] {}World Chat System is disabled.|r", WORLD_CHAT_RED));
            return true;
        }

        if (!WorldChat[guid].chat)
        {
            ChatHandler(player->GetSession()).PSendSysMessage(Acore::StringFormat("[WC] {}World Chat is already hidden.|r", WORLD_CHAT_RED));
            return true;
        }

        WorldChat[guid].chat = 0;

        ChatHandler(player->GetSession()).PSendSysMessage(Acore::StringFormat("[WC] {}World Chat is now hidden.|r", WORLD_CHAT_GREEN));

        return true;
    };

    ChatCommandTable GetCommands() const override
    {
        static ChatCommandTable wcCommandTable =
        {
            { "on",      HandleWorldChatOnCommand, SEC_PLAYER, Console::No },
            { "off",     HandleWorldChatOffCommand, SEC_PLAYER, Console::No },
            { "",        HandleWorldChatCommand, SEC_PLAYER, Console::No },
        };

        static ChatCommandTable commandTable =
        {
            { "chat", wcCommandTable },
            { "chath", HandleWorldChatHordeCommand, SEC_MODERATOR, Console::Yes},
            { "chata", HandleWorldChatAllianceCommand, SEC_MODERATOR, Console::Yes},
        };

        return commandTable;

    }
};

class WorldChat_Announce : public PlayerScript
{
public:

    WorldChat_Announce() : PlayerScript("WorldChat_Announce") {}

    void OnPlayerLogin(Player* player)
    {
        // Announce Module
        if (WC_Config.Enabled && WC_Config.Announce)
        {
            ChatHandler(player->GetSession()).SendSysMessage(("This server is running the |cff4CFF00WorldChat |rmodule. Use .chat" + ((WC_Config.ChannelName != "") ? " or use /join " + WC_Config.ChannelName : "") + " to communicate"+ ((!WC_Config.CrossFaction) ? " with your faction." : ".")));
        }
    }

    void OnPlayerChat(Player* player, uint32 /*type*/, uint32 lang, std::string& msg, Channel* channel)
    {
        if (WC_Config.ChannelName != "" && lang != LANG_ADDON && channel->GetName() == WC_Config.ChannelName)
        {
            SendWorldMessage(player, msg, -1);
            msg = -1;
        }
    }
};

void AddSC_WorldChatScripts()
{
    new WorldChat_Announce();
    new WorldChat_Config();
    new World_Chat();
}
