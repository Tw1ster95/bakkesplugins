#include "pch.h"
#include "koth.h"
#include "bakkesmod/wrappers/GameObject/Stats/StatEventWrapper.h"
#include "bakkesmod/wrappers/GameObject/BallWrapper.h"
#include "utils/parser.h"
#include <string>


BAKKESMOD_PLUGIN(koth, "King of the hill plugin", plugin_version, 0);

std::shared_ptr<CVarManagerWrapper> _globalCvarManager;

// The structure of a ticker stat event
struct TickerStruct {
	// person who got a stat
	uintptr_t Receiver;
	// person who is victim of a stat (only exists for demos afaik)
	uintptr_t Victim;
	// wrapper for the stat event
	uintptr_t StatEvent;
};

PriWrapper koth::getPriByID(int id)
{
	auto server = gameWrapper->GetCurrentGameState();
	if (server.IsNull())
	{
		cvarManager->log("Server null");
		return -1;
	}
	ArrayWrapper<PriWrapper> pris = server.GetPRIs();
	int len = pris.Count();
	cvarManager->log("getPriByID() Getting pri by ID: " + std::to_string(id));
	for (int i = 0; i < len; i++)
	{
		PriWrapper player = pris.Get(i);
		if (player.GetPlayerID() == id)
		{
			cvarManager->log("getPriByID() Pri found for ID: " + std::to_string(id));
			return player;
		}
	}
	cvarManager->log("getPriByID() Pri was not found for ID: " + std::to_string(id));
	return -1;
}

int koth::getPlayerNumByID(int id)
{
	for (int i = 0; i < koth_max_players; i++)
	{
		if (koth_players[i] == -1)
			break;
		if (koth_players[i] == id)
		{
			cvarManager->log("getPlayerNumByID() Player num " + std::to_string(i) + " found for ID: " + std::to_string(id));
			return i;
		}
	}
	cvarManager->log("getPlayerNumByID() Player num was not found for ID: " + std::to_string(id));
	return -1;
}

void koth::resetPlayers() {
	if (!*enabled)
	{
		cvarManager->log("Plugin is disabled. Use koth_enable 1 to re-enable it.");
		return;
	}
	cvarManager->log("resetPlayers() Resetting players data.");
	for (int i = 0; i < koth_max_players; i++)
	{
		koth_players[i] = -1;
		koth_queue[i] = -1;
		koth_player_names[i] = "";
	}
	koth_playing[0] = -1;
	koth_playing[1] = -1;
	cvarManager->log("resetPlayers() Players data has been reset.");
}
void koth::randomizePlayers() {
	cvarManager->log("randomizePlayers(): Randomizing players array.");
	int koth_p;
	for (koth_p = 0; koth_p < koth_max_players; koth_p++)
		if (koth_players[koth_p] == -1)
			break;

	koth_p--;

	if (koth_p > 1) {
		int temp_p;
		int random[2];
		for (int i = 0; i < 10; i++)
		{
			random[0] = rand() % koth_p;
			random[1] = rand() % koth_p;
			if (random[0] != random[1]) {
				temp_p = koth_players[random[0]];
				koth_players[random[0]] = koth_players[random[1]];
				koth_players[random[1]] = temp_p;
			}
		}
		cvarManager->log("randomizePlayers(): Players array randomized.");
	}
	else
		cvarManager->log("randomizePlayers(): Not enough players to randomize.");
}

void koth::randomizeAllPlayers() {
	if (!*enabled)
	{
		cvarManager->log("Plugin is disabled. Use koth_enable 1 to re-enable it.");
		return;
	}
	auto server = gameWrapper->GetCurrentGameState();
	if (server.IsNull())
	{
		cvarManager->log("Server null");
		return;
	}
	resetPlayers();
	int koth_p = 0;
	ArrayWrapper<PriWrapper> pris = server.GetPRIs();
	int len = pris.Count();
	cvarManager->log("randomizeAllPlayers(): Getting players.");
	for (int i = 0; i < len; i++)
	{
		if (koth_p >= koth_max_players)
			break;

		PriWrapper player = pris.Get(i);
		std::string playerName = player.GetPlayerName().ToString();
		koth_players[koth_p] = player.GetPlayerID();
		koth_player_names[koth_p] = playerName;
		cvarManager->log("randomizeAllPlayers(): Putting " + playerName + " to team " + std::to_string(koth_p + 1));
		koth_p++;
	}
	randomizePlayers();
}

void koth::MoveQueueBack()
{
	cvarManager->log("MoveQueueBack() Moving QUEUE one back.");
	for (int y = 0; y < koth_max_players; y++)
	{
		if (y == koth_max_players - 1)
		{
			if(koth_queue[y] != -1)
				cvarManager->log("MoveQueueBack() Removing player" + koth_player_names[koth_queue[y]] + " from pos " + std::to_string(koth_max_players - 1) + " in the queue.");
			koth_queue[y] = -1;
		}
		else {
			if (koth_queue[y+1] != -1)
				cvarManager->log("MoveQueueBack() Moving " + koth_player_names[koth_queue[y+1]] + " from pos " + std::to_string(y + 2) + " to " + std::to_string(y + 1));
			koth_queue[y] = koth_queue[y + 1];
		}
	}
	cvarManager->log("MoveQueueBack() QUEUE moved.");
}

void koth::insertInQueue(int num) {
	cvarManager->log("insertInQueue() Inserting player " + koth_player_names[num] + " to queue.");
	if (koth_queue[koth_max_players - 1] >= 0) {
		cvarManager->log("insertInQueue() Couldn't add " + koth_player_names[num] + " to the queue. Queue is full.");
	}
	else {
		for (int i = 0; i < koth_max_players; i++) {
			if (koth_queue[i] == -1)
			{
				koth_queue[i] = num;
				cvarManager->log("insertInQueue() Adding " + koth_player_names[num] + " to the end of the queue at position " + std::to_string(i + 1));
				break;
			}
		}
	}
}

bool koth::isPlayingKOTH(int id) {
	for (int i = 0; i < koth_max_players; i++) {
		if (koth_players[i] == id)
			return true;
	}
	return false;
}

void koth::ChangeTeamByID(int id, int team) {
	auto server = gameWrapper->GetCurrentGameState();
	if (server.IsNull())
	{
		cvarManager->log("ChangeTeamByID() Server null");
		return;
	}
	cvarManager->log("ChangeTeamByID() Moving player ID " + std::to_string(id) + " to team " + (team == 2 ? "Spectator" : Teams[team]));
	ArrayWrapper<PriWrapper> pris = server.GetPRIs();
	int len = pris.Count();
	std::string playerName;
	for (int i = 0; i < len; i++)
	{
		PriWrapper player = pris.Get(i);
		if (player.GetPlayerID() == id) {
			playerName = player.GetPlayerName().ToString();
			if (team == 2) {
				player.ServerSpectate();
				cvarManager->log("ChangeTeamByID() Player " + playerName + " moved to Spectator.");
			} else {
				player.ServerChangeTeam(team);
				cvarManager->log("ChangeTeamByID() Player " + playerName + " moved to team " + Teams[team]);
			}
		}
	}
}

void koth::startGame() {
	if (!*enabled)
	{
		cvarManager->log("Plugin is disabled. Use koth_enable 1 to re-enable it.");
		return;
	}
	int i = 0;
	for (i = 0; i < koth_max_players; i++)
	{
		if (koth_players[i] == -1)
			break;
	}
	if (i < koth_players_needed)
	{
		cvarManager->log("startGame() The game needs atleast " + std::to_string(koth_players_needed) + " players to start. Current players number: " + std::to_string(i));
		return;
	}
	for (i = 0; i < koth_max_players; i++)
	{
		if (koth_players[i] == -1)
			break;
		cvarManager->log("startGame() Handling player " + koth_player_names[i]);
		if (i < 2) {
			koth_playing[i] = i;
			cvarManager->log("startGame() Putting player " + koth_player_names[i] + " on team " + Teams[i]);
			ChangeTeamByID(koth_players[i], i);
		}
		else {
			insertInQueue(i);
			cvarManager->log("startGame() Putting player " + koth_player_names[i] + " on team " + Teams[2]);
			ChangeTeamByID(koth_players[i], 2);
		}
	}
	cvarManager->log("startGame() _______________________________________________________");
	cvarManager->log("startGame() Starting King of the hill with these players:");
	cvarManager->log("startGame() " + Teams[0] + " team: " + koth_player_names[0]);
	cvarManager->log("startGame() " + Teams[1] + " team: " + koth_player_names[1]);
	cvarManager->log("startGame() QUEUE:");
	for (int i = 0; i < koth_max_players; i++)
	{
		if(koth_queue[i] >= 0)
			cvarManager->log("startGame() " + std::to_string(i + 1) + ": " + koth_player_names[koth_queue[i]]);
	}
	cvarManager->log("startGame() _______________________________________________________");
}

void koth::statTickerEvent(ServerWrapper caller, void* args) {
	if (!*enabled)
		return;
	if (koth_queue[0] == -1) {
		cvarManager->log("statTickerEvent() Queue NULL!");
		return;
	}

	auto tArgs = (TickerStruct*)args;

	// separates the parts of the stat event args
	auto receiver = PriWrapper(tArgs->Receiver);
	auto victim = PriWrapper(tArgs->Victim);
	auto statEvent = StatEventWrapper(tArgs->StatEvent);
	// name of the stat as shown in rocket league 
	//  (Demolition, Extermination, etc.)
	auto label = statEvent.GetLabel();
	auto event_type = label.ToString();

	if (event_type != "Goal")
		return;
	cvarManager->log("statTickerEvent() Goal was scored. Changing teams.");

	int win_team = receiver.GetTeamNum();
	int change_team = (win_team == 1) ? 0 : 1;

	auto server = gameWrapper->GetCurrentGameState();
	if (server.IsNull())
	{
		cvarManager->log("statTickerEvent() Server null");
		return;
	}
	ArrayWrapper<PriWrapper> pris = server.GetPRIs();
	int len = pris.Count();
	int team;
	int p_num;
	for (int i = 0; i < len; i++)
	{
		PriWrapper player = pris.Get(i);
		std::string playerName = player.GetPlayerName().ToString();
		team = player.GetTeamNum();
		if (team == change_team) {
			p_num = getPlayerNumByID(player.GetPlayerID());
			if (p_num != -1) {
				insertInQueue(p_num);
			}
		}
		else if (team != win_team) {
			if (player.GetPlayerID() == koth_players[koth_queue[0]]) {
				koth_playing[change_team] = koth_queue[0];
				cvarManager->log("statTickerEvent() Adding " + koth_player_names[koth_queue[0]] + " to team " + std::to_string(change_team));
			}
		}
	}
	MoveQueueBack();
	cvarManager->log("statTickerEvent() _______________________________________________________");
	cvarManager->log("statTickerEvent() Players playing:");
	cvarManager->log("statTickerEvent() " + Teams[0] + " team: " + koth_player_names[koth_playing[0]]);
	cvarManager->log("statTickerEvent() " + Teams[1] + " team: " + koth_player_names[koth_playing[1]]);
	cvarManager->log("statTickerEvent() QUEUE:");
	for (int i = 0; i < koth_max_players; i++)
	{
		if (koth_queue[i] != -1)
			cvarManager->log("statTickerEvent() " + std::to_string(i + 1) + ": " + koth_player_names[koth_queue[i]]);
	}
	cvarManager->log("statTickerEvent() _______________________________________________________");
	change_teams = true;
}

void koth::StartNewRound() {
	if (!*enabled)
		return;
	if (change_teams)
	{
		change_teams = false;
		auto server = gameWrapper->GetCurrentGameState();
		if (server.IsNull())
		{
			cvarManager->log("Server null");
			return;
		}
		ArrayWrapper<PriWrapper> pris = server.GetPRIs();
		int len = pris.Count();
		int id;
		int team;
		for (int i = 0; i < len; i++)
		{
			PriWrapper player = pris.Get(i);
			id = player.GetPlayerID();
			team = player.GetTeamNum();
			if (isPlayingKOTH(id)) {
				if (id == koth_players[koth_playing[0]]) {
					if (team != 0)
					{
						cvarManager->log("StartNewRound() Changing " + koth_player_names[koth_playing[0]] + " team from " + std::to_string(team) + " to 0.");
						player.ServerChangeTeam(0);
					}
				}
				else if (id == koth_players[koth_playing[1]]) {
					if (team != 1)
					{
						cvarManager->log("StartNewRound() Changing " + koth_player_names[koth_playing[0]] + " team from " + std::to_string(team) + " to 1.");
						player.ServerChangeTeam(1);
					}
				}
				else if (team != 255) {
					cvarManager->log("StartNewRound() Changing " + koth_player_names[koth_playing[0]] + " team from " + std::to_string(team) + " to Spectator.");
					player.ServerSpectate();
				}
			}
			else if(team != 255)
				player.ServerSpectate();
		}
	}
}

void koth::RenderList(CanvasWrapper canvas)
{
	if (!gameWrapper->IsInGame() || !*enabled)
		return;

	std::string text;
	canvas.SetPosition(Vector2{ 100, 270 });
	canvas.SetColor(0, 255, 0, 255);
	canvas.DrawString("QUEUE list:", 2, 2);

	canvas.SetPosition(Vector2{ 100, 300 });
	canvas.SetColor(0, 255, 0, 255);
	canvas.DrawString("__________________", 2, 1);

	if (koth_queue[0] == -1)
	{
		canvas.SetPosition(Vector2{ 100, 330 });
		canvas.SetColor(0, 100, 255, 255);
		canvas.DrawString("No players in the queue.", 2, 2);
	}
	else {
		for (int i = 0; i < koth_max_players; i++)
		{
			if (koth_queue[i] == -1)
				break;
			canvas.SetPosition(Vector2{ 100, 330 + i * 30 });
			canvas.SetColor(0, 100, 255, 255);
			text = ((i == 0) ? std::to_string(i + 1) : "Next up") + ": " + koth_player_names[koth_queue[i]];
			canvas.DrawString(text, 2, 2);
		}
	}
}

void koth::onLoad()
{
	_globalCvarManager = cvarManager;

	enabled = std::make_shared<bool>(false);
	cvarManager->registerCvar("koth_enable", "0", "Enable the King of the Hill plugin", true, true, 0, true, 1).bindTo(enabled);

	cvarManager->registerNotifier("koth_reset", [&](std::vector<std::string> args) {
		if (!*enabled)
		{
			cvarManager->log("Plugin is disabled. Use koth_enable 1 to re-enable it.");
			return;
		}
		resetPlayers();
	}, "", 0);

	cvarManager->registerNotifier("koth_randomize", [&](std::vector<std::string> args) {
		randomizeAllPlayers();
	}, "", 0);

	cvarManager->registerNotifier("koth_add_team", [&](std::vector<std::string> args) {
		if (!*enabled)
		{
			cvarManager->log("Plugin is disabled. Use koth_enable 1 to re-enable it.");
			return;
		}
		auto server = gameWrapper->GetCurrentGameState();
		if (server.IsNull())
		{
			cvarManager->log("Server null");
			return;
		}
		int i;
		int team = 0;
		for (i = 0; i < sizeof(koth_players); i++)
		{
			if (!koth_players[i])
			{
				team = i;
				break;
			}
		}
		ArrayWrapper<PriWrapper> pris = server.GetPRIs();
		int len = pris.Count();
		for (i = 0; i < len; i++)
		{
			PriWrapper player = pris.Get(i);
			std::string playerName = player.GetPlayerName().ToString();
			if (playerName == args[1]) {
				for (int y = 0;y < sizeof(koth_players);y++)
				{
					if (koth_players[y] == player.GetPlayerID())
					{
						cvarManager->log("Player " + playerName + " is allready on team " + std::to_string(y + 1));
						return;
					}
				}
				koth_players[team] = player.GetPlayerID();
				koth_player_names[team] = playerName;
				cvarManager->log("Putting " + playerName + " to team " + std::to_string(team + 1));
				break;
			}
		}
	}, "", 0);

	cvarManager->registerNotifier("koth_start", [&](std::vector<std::string> args) {
		startGame();
	}, "", 0);

	gameWrapper->HookEventWithCallerPost<ServerWrapper>("Function TAGame.GFxHUD_TA.HandleStatTickerMessage", std::bind(&koth::statTickerEvent, this, std::placeholders::_1, std::placeholders::_2));
	//gameWrapper->HookEventWithCallerPost<ServerWrapper>("Function TAGame.Replay_TA.StopPlayback", std::bind(&koth::StopPlayback, this));
	gameWrapper->HookEventWithCallerPost<ServerWrapper>("Function TAGame.GameEvent_Soccar_TA.EventStartNewRound", std::bind(&koth::StartNewRound, this));
	gameWrapper->RegisterDrawable(std::bind(&koth::RenderList, this, std::placeholders::_1));

	resetPlayers();
}

void koth::onUnload()
{
}