// Copyright 2005-2016 The Mumble Developers. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file at the root of the
// Mumble source tree or at <https://www.mumble.info/LICENSE>.

#include "../mumble_plugin_win32_64bit.h" // Include standard plugin header.
#include "../mumble_plugin_utils.h" // Include plugin header for special functions, like "escape".

static int fetch(float *avatar_pos, float *avatar_front, float *avatar_top, float *camera_pos, float *camera_front, float *camera_top, std::string &context, std::wstring &identity) {
	for (int i=0;i<3;i++) {
		avatar_pos[i] = avatar_front[i] = avatar_top[i] = camera_pos[i] = camera_front[i] = camera_top[i] = 0.0f;
	}

	bool ok, state;
	char server_name[100], team[4];
	BYTE squad, squad_leader;

	// State pointers
	procptr64_t state_base = peekProc<procptr64_t>(pModule + 0x3319598);
	if (!state_base) return false;
	procptr64_t state_offset_0 = peekProc<procptr64_t>(state_base + 0x5F0);
	if (!state_offset_0) return false;
	procptr64_t state_offset_1 = peekProc<procptr64_t>(state_offset_0 + 0x488);
	if (!state_offset_1) return false;
	procptr64_t state_offset_2 = peekProc<procptr64_t>(state_offset_1 + 0x8);
	if (!state_offset_2) return false;
	procptr64_t state_offset_3 = peekProc<procptr64_t>(state_offset_2 + 0x8);
	if (!state_offset_3) return false;

	// Camera pointer
	procptr64_t camera_base = peekProc<procptr64_t>(pModule + 0x35B3730);
	if (!camera_base) return false;

	// Server name pointers
	procptr64_t server_name_base = peekProc<procptr64_t>(pModule + 0x35EA740);
	if (!server_name_base) return false;
	procptr64_t server_name_offset = peekProc<procptr64_t>(server_name_base + 0x80);
	if (!server_name_offset) return false;

	// Team pointers
	procptr64_t team_base = peekProc<procptr64_t>(pModule + 0x35FB780);
	if (!team_base) return false;
	procptr64_t team_offset_0 = peekProc<procptr64_t>(team_base + 0x7D0);
	if (!team_offset_0) return false;
	procptr64_t team_offset_1 = peekProc<procptr64_t>(team_offset_0 + 0x440);
	if (!team_offset_1) return false;
	procptr64_t team_offset_2 = peekProc<procptr64_t>(team_offset_1 + 0x138);
	if (!team_offset_2) return false;

	// Squad pointers
	procptr64_t squad_base = peekProc<procptr64_t>(pModule + 0x31016D0);
	if (!squad_base) return false;
	procptr64_t squad_offset_0 = peekProc<procptr64_t>(squad_base + 0x30);
	if (!squad_offset_0) return false;
	procptr64_t squad_offset_1 = peekProc<procptr64_t>(squad_offset_0 + 0x578);
	if (!squad_offset_1) return false;
	procptr64_t squad_offset_2 = peekProc<procptr64_t>(squad_offset_1 + 0xC0);
	if (!squad_offset_2) return false;
	procptr64_t squad_offset_3 = peekProc<procptr64_t>(squad_offset_2 + 0x58);
	if (!squad_offset_3) return false;

	// Peekproc and assign game addresses to our containers, so we can retrieve positional data
	ok = peekProc(state_offset_3 + 0x50, state) && // Magical state value: 1 when in-game and 0 when not spawned or playing.
			peekProc(pModule + 0x35D2C60, avatar_pos) && // Avatar position values (X, Y and Z).
			peekProc(camera_base + 0x2B0, camera_pos) && // Camera position values (X, Y and Z).
			peekProc(camera_base + 0x260, camera_front) && // Avatar front vector values (X, Y and Z).
			peekProc(camera_base + 0x250, camera_top) && // Avatar top vector values (X, Y and Z).
			peekProc(server_name_offset, server_name) && // Server name.
			peekProc(team_offset_2 + 0x13, team) && // Team name.
			peekProc(squad_offset_3 + 0x240, squad) && // Squad value: 0 (not in a squad), 1 (Apples), 2 (Butter), 3 (Charlie)... 26 (Zebra).
			peekProc(squad_offset_3 + 0xDCC, squad_leader); // Squad leader value: 0 (False), 1 (True).

	// This prevents the plugin from linking to the game in case something goes wrong during values retrieval from memory addresses.
	if (!ok) {
		return false;
	}

	if (!state) { // If not in-game
		context.clear(); // Clear context
		identity.clear(); // Clear identity
		// Set vectors values to 0.
		for (int i=0;i<3;i++) {
			avatar_pos[i] = avatar_front[i] = avatar_top[i] = camera_pos[i] =  camera_front[i] = camera_top[i] = 0.0f;
		}

		return true; // This tells Mumble to ignore all vectors.
	}

	// Begin context
	std::ostringstream ocontext;

	// Server name
	escape(server_name, sizeof(server_name));
	if (strcmp(server_name, "") != 0) {
		ocontext << " {\"Server name\": \"" << server_name << "\"}"; // Set context with server name.
	}

	context = ocontext.str();
	// End context

	// Begin identity
	std::wostringstream oidentity;
	oidentity << "{";

	// Team
	escape(team, sizeof(server_name));
	if (strcmp(team, "") != 0) {
		oidentity << std::endl << "\"Team\": \"" << team << "\","; // Set team name in identity.
	}

	// Squad
	if (squad > 0 && squad < 27) { // If squad value is between 1 and 26, set squad name in identity using RAF (1917) Phonetic alphabet.
		// Squad name
		if (squad == 1)
			oidentity << std::endl << "\"Squad\": \"Apples\",";
		else if (squad == 2)
			oidentity << std::endl << "\"Squad\": \"Butter\",";
		else if (squad == 3)
			oidentity << std::endl << "\"Squad\": \"Charlie\",";
		else if (squad == 4)
			oidentity << std::endl << "\"Squad\": \"Duff\",";
		else if (squad == 5)
			oidentity << std::endl << "\"Squad\": \"Edward\",";
		else if (squad == 6)
			oidentity << std::endl << "\"Squad\": \"Freddy\",";
		else if (squad == 7)
			oidentity << std::endl << "\"Squad\": \"George\",";
		else if (squad == 8)
			oidentity << std::endl << "\"Squad\": \"Harry\",";
		else if (squad == 9)
			oidentity << std::endl << "\"Squad\": \"Ink\",";
		else if (squad == 10)
			oidentity << std::endl << "\"Squad\": \"Johnnie\",";
		else if (squad == 11)
			oidentity << std::endl << "\"Squad\": \"King\",";
		else if (squad == 12)
			oidentity << std::endl << "\"Squad\": \"London\",";
		else if (squad == 13)
			oidentity << std::endl << "\"Squad\": \"Monkey\",";
		else if (squad == 14)
			oidentity << std::endl << "\"Squad\": \"Nuts\",";
		else if (squad == 15)
			oidentity << std::endl << "\"Squad\": \"Orange\",";
		else if (squad == 16)
			oidentity << std::endl << "\"Squad\": \"Pudding\",";
		else if (squad == 17)
			oidentity << std::endl << "\"Squad\": \"Queenie\",";
		else if (squad == 18)
			oidentity << std::endl << "\"Squad\": \"Robert\",";
		else if (squad == 19)
			oidentity << std::endl << "\"Squad\": \"Sugar\",";
		else if (squad == 20)
			oidentity << std::endl << "\"Squad\": \"Tommy\",";
		else if (squad == 21)
			oidentity << std::endl << "\"Squad\": \"Uncle\",";
		else if (squad == 22)
			oidentity << std::endl << "\"Squad\": \"Vinegar\",";
		else if (squad == 23)
			oidentity << std::endl << "\"Squad\": \"Willie\",";
		else if (squad == 24)
			oidentity << std::endl << "\"Squad\": \"Xerxes\",";
		else if (squad == 25)
			oidentity << std::endl << "\"Squad\": \"Yellow\",";
		else if (squad == 26)
			oidentity << std::endl << "\"Squad\": \"Zebra\",";
		// Squad leader
		if (squad_leader == 1)
			oidentity << std::endl << "\"Squad leader\": true"; // If squad leader value is true, set squad leader state to "True" in identity.
		else
			oidentity << std::endl << "\"Squad leader\": false"; // If squad leader value is false, set squad leader state to "False" in identity.
		// When not in a squad
	} else {
		oidentity << std::endl << "\"Squad\": null,"; // If squad value isn't between 1 and 26, set squad to "null" in identity.
		oidentity << std::endl << "\"Squad leader\": null"; // If not in a squad, set squad leader state to "null" in identity.
	}

	oidentity << std::endl << "}";
	identity = oidentity.str();
	// End identity

	// Flip the front vector
	for (int i=0;i<3;i++) {
		camera_front[i] = -camera_front[i];
	}

	// Convert from right to left handed
	avatar_pos[0] = -avatar_pos[0];
	camera_pos[0] = -camera_pos[0];
	camera_front[0] = -camera_front[0];
	camera_top[0] = -camera_top[0];

	// Sync avatar front and top vectors with camera ones
	for (int i=0;i<3;i++) {
		avatar_front[i] = camera_front[i];
		avatar_top[i] = camera_top[i];
	}

	return true;
}

static int trylock(const std::multimap<std::wstring, unsigned long long int> &pids) {

	if (! initialize(pids, L"bf1.exe")) { // Retrieve game executable's memory address
		return false;
	}

	// Check if we can get meaningful data from it
	float apos[3], afront[3], atop[3], cpos[3], cfront[3], ctop[3];
	std::wstring sidentity;
	std::string scontext;

	if (fetch(apos, afront, atop, cpos, cfront, ctop, scontext, sidentity)) {
		return true;
	} else {
		generic_unlock();
		return false;
	}
}

static const std::wstring longdesc() {
	return std::wstring(L"Supports Battlefield 1 with context and identity support."); // Plugin long description
}

static std::wstring description(L"Battlefield 1 version 1.0.9.53998"); // Plugin short description
static std::wstring shortname(L"Battlefield 1"); // Plugin short name

static int trylock1() {
	return trylock(std::multimap<std::wstring, unsigned long long int>());
}

static MumblePlugin bf1plug = {
	MUMBLE_PLUGIN_MAGIC,
	description,
	shortname,
	NULL,
	NULL,
	trylock1,
	generic_unlock,
	longdesc,
	fetch
};

static MumblePlugin2 bf1plug2 = {
	MUMBLE_PLUGIN_MAGIC_2,
	MUMBLE_PLUGIN_VERSION,
	trylock
};

extern "C" MUMBLE_PLUGIN_EXPORT MumblePlugin *getMumblePlugin() {
	return &bf1plug;
}

extern "C" MUMBLE_PLUGIN_EXPORT MumblePlugin2 *getMumblePlugin2() {
	return &bf1plug2;
}
