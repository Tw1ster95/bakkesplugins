#pragma once

#include "bakkesmod/plugin/bakkesmodplugin.h"
#include "bakkesmod/plugin/pluginwindow.h"

#include "version.h"
constexpr auto plugin_version = stringify(VERSION_MAJOR) "." stringify(VERSION_MINOR) "." stringify(VERSION_PATCH) "." stringify(VERSION_BUILD);

static const int koth_max_players = 8;
static const int koth_players_needed = 3;

class koth : public BakkesMod::Plugin::BakkesModPlugin, public BakkesMod::Plugin::PluginWindow
{
public:
	std::shared_ptr<bool> enabled;

	std::string Teams[3] = {
		"Orange",
		"Blue",
		"Spectator"
	};

	int koth_players[koth_max_players];
	std::string koth_player_names[koth_max_players];
	int koth_playing[2];
	int koth_queue[koth_max_players];
	bool change_teams = true;

	//Boilerplate
	virtual void onLoad();
	virtual void onUnload();

	virtual PriWrapper getPriByID(int);
	virtual int getPlayerNumByID(int);
	virtual bool isPlayingKOTH(int);
	void resetPlayers();
	void randomizePlayers();
	void randomizeAllPlayers();
	void MoveQueueBack();
	void insertInQueue(int);
	void ChangeTeamByID(int, int);
	void startGame();

	void statTickerEvent(ServerWrapper caller, void* args);
	//void StopPlayback();
	void StartNewRound();
	virtual void RenderList(CanvasWrapper canvas);

	// Inherited via PluginWindow
	bool isWindowOpen_ = false;
	bool isMinimized_ = false;
	std::string menuTitle_ = "koth";

	bool btnStart = false;

	virtual void Render() override;
	virtual std::string GetMenuName() override;
	virtual std::string GetMenuTitle() override;
	virtual void SetImGuiContext(uintptr_t ctx) override;
	virtual bool ShouldBlockInput() override;
	virtual bool IsActiveOverlay() override;
	virtual void OnOpen() override;
	virtual void OnClose() override;
};