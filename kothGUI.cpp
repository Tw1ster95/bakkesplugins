#include "pch.h"
#include "koth.h"

// Do ImGui rendering here
void koth::Render()
{
	if (!ImGui::Begin(menuTitle_.c_str(), &isWindowOpen_, ImGuiWindowFlags_None))
	{
		// Early out if the window is collapsed, as an optimization.
		ImGui::End();
		return;
	}
	
	// Enable/Disable plugin
	if (*enabled) {
		if (ImGui::Button("Disable")) {
			cvarManager->executeCommand("koth_enable 0");
		}
	} else {
		if (ImGui::Button("Enable")) {
			cvarManager->executeCommand("koth_enable 1");
		}
	}

	// Add and Randomize All Players
	if (ImGui::Button("Add and Randomize All Players")) {
		randomizeAllPlayers();
	}


	// Randomize Current Players
	if (ImGui::Button("Randomize Current Players")) {
		randomizePlayers();
	}

	// Start Game
	if (ImGui::Button("Start")) {
		gameWrapper->Execute([this](GameWrapper* gw) {startGame();});
	}

	// Reset Players
	if (ImGui::Button("Reset Players")) {
		resetPlayers();
	}

	ImGui::Spacing();
	ImGui::Spacing();
	ImGui::Spacing();
	ImGui::Spacing();

	if (*enabled) {
		ImGui::Text("Players:");
		if (koth_players[0] == NULL) {
			ImGui::Spacing();
			ImGui::Spacing();
			ImGui::Text("No players selected.");
		}
		else {
			for (int i = 0;i < koth_max_players;i++) {
				if (koth_players[i] == NULL)
					break;
				ImGui::BulletText(koth_player_names[i].c_str());
			}
		}
	}

	ImGui::End();

	if (!isWindowOpen_)
	{
		cvarManager->executeCommand("togglemenu " + GetMenuName());
	}
}

// Name of the menu that is used to toggle the window.
std::string koth::GetMenuName()
{
	return "koth";
}

// Title to give the menu
std::string koth::GetMenuTitle()
{
	return menuTitle_;
}

// Don't call this yourself, BM will call this function with a pointer to the current ImGui context
void koth::SetImGuiContext(uintptr_t ctx)
{
	ImGui::SetCurrentContext(reinterpret_cast<ImGuiContext*>(ctx));
}

// Should events such as mouse clicks/key inputs be blocked so they won't reach the game
bool koth::ShouldBlockInput()
{
	return ImGui::GetIO().WantCaptureMouse || ImGui::GetIO().WantCaptureKeyboard;
}

// Return true if window should be interactive
bool koth::IsActiveOverlay()
{
	return true;
}

// Called when window is opened
void koth::OnOpen()
{
	isWindowOpen_ = true;
}

// Called when window is closed
void koth::OnClose()
{
	isWindowOpen_ = false;
}
