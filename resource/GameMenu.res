"GameMenu"
{
	"1"
	{
		"label" "#GameUI_GameMenu_ResumeGame"
		"command" "ResumeGame"
		"OnlyInGame" "1"
	}
	"5"
	{
		"label" "#GameUI_GameMenu_PlayerList"
		"command" "OpenPlayerListDialog"
		"OnlyInGame" "1"
		"notsingle" "1"
	}
	"8"
	{
		"label" "#GameUI_GameMenu_SaveGame"
		"command" "OpenSaveGameDialog"
		"notmulti" "1"
		"OnlyInGame" "1"
	}
	"10"
	{
		"label" "Wave 1"
		"command" "engine map cs_mansion_wave1"
		"notmulti" "1"
	}
	"11"
	{
		"label" "Wave 2"
		"command" "engine map cs_mansion_wave2"
		"notmulti" "1"
	}
	"12"
	{
		"label" "Wave 3"
		"command" "engine map cs_mansion_wave3"
		"notmulti" "1"
	}
	"13"
	{
		"label" "Wave 4"
		"command" "engine map cs_mansion_wave4"
		"notmulti" "1"
	}
	"18"
	{
		"label" "#GameUI_GameMenu_Options"
		"command" "OpenOptionsDialog"
		"HelpText" "#GameUI_MainMenu_Hint_Configuration"
	}
	"19"
	{
		"label" "#GameUI_GameMenu_Disconnect"
		"command" "Disconnect"
		"OnlyInGame" "1"
		"notsingle" "1"
	}
	"20"
	{
		"label" "#GameUI_GameMenu_LeaveGame"
		"command" "Disconnect"
		"OnlyInGame" "1"
		"notmulti" "1"
	}
	"21"
	{
		"label" "#GameUI_GameMenu_Quit"
		"command" "Quit"
		"HelpText" "#GameUI_MainMenu_Hint_QuitGame"
	}
}
