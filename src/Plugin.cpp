/*
 * Copyright (C) 2012-2013 Jacek Sieka, arnetheduck on gmail point com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "stdafx.h"
#include "Plugin.h"

/* Include plugin SDK helpers. There are more interfaces available that can be included in the same
fashion (check the pluginsdk directory). */
#include <pluginsdk/Config.h>
#include <pluginsdk/Connections.h>
#include <pluginsdk/Core.h>
#include <pluginsdk/Hooks.h>
#include <pluginsdk/Hubs.h>
#include <pluginsdk/Logger.h>
#include <pluginsdk/UI.h>
#include <pluginsdk/Util.h>

/* Plugin SDK helpers are in the "dcapi" namespace; ease their calling. */
using dcapi::Config;
using dcapi::Connections;
using dcapi::Core;
using dcapi::Hooks;
using dcapi::Hubs;
using dcapi::Logger;
using dcapi::UI;
using dcapi::Util;

const string showCommand = "Show the dialog";
const string hideCommand = "Hide the dialog";

Plugin::Plugin() {
}

Plugin::~Plugin() {
	Hooks::clear();

	if(UI::handle()) {
		UI::removeCommand(showCommand);
		UI::removeCommand(hideCommand);
	}

	GUI::unloading = true;
}

Plugin* instance;

Bool DCAPI Plugin::main(PluginState state, DCCorePtr core, dcptr_t) {
	switch(state) {
	case ON_INSTALL:
	case ON_LOAD:
	case ON_LOAD_RUNTIME:
		{
			instance = new Plugin();
			return instance->onLoad(core, state == ON_INSTALL, state == ON_INSTALL || state == ON_LOAD_RUNTIME) ? True : False;
		}

	case ON_UNINSTALL:
	case ON_UNLOAD:
		{
			delete instance;
			instance = 0;
			return True;
		}

	default:
		{
			return False;
		}
	}
}

bool Plugin::onLoad(DCCorePtr core, bool install, bool runtime) {
	/* Initialization phase. Initiate additional interfaces that you may have included from the
	plugin SDK. */
	Core::init(core);
	if(!Config::init(PLUGIN_GUID) || !Connections::init() || !Hooks::init() || !Hubs::init() || !Logger::init() || !UI::init(PLUGIN_GUID) || !Util::init()) {
		return false;
	}

	if(install) {
		// This only executes when the plugin has been installed for the first time.
		Config::setConfig("Dialog", true);
		Config::setConfig("BgColor", (int)RGB(255, 255, 255));
		Config::setConfig("ADCColor", (int)RGB(0, 0, 0));
		Config::setConfig("NMDCColor", (int)RGB(0, 0, 0));
		Config::setConfig("UDPColor", (int)RGB(0, 0, 0));

		Logger::log("The dev plugin has been installed; check the plugins menu and the /raw chat command.");
	}

	if(runtime) {
		// If the plugin doesn't support being enabled at runtime, cancel its loading here.
	}

	// Start the plugin logic here; add hooks with functions from the Hooks interface.
	Hooks::Network::onHubDataIn([this](HubDataPtr hHub, char* message, bool&) { return onHubDataIn(hHub, message); });
	Hooks::Network::onHubDataOut([this](HubDataPtr hHub, char* message, bool&) { return onHubDataOut(hHub, message); });
	Hooks::Network::onClientDataIn([this](ConnectionDataPtr hConn, char* message, bool&) { return onClientDataIn(hConn, message); });
	Hooks::Network::onClientDataOut([this](ConnectionDataPtr hConn, char* message, bool&) { return onClientDataOut(hConn, message); });
	Hooks::Network::onUDPDataIn([this](UDPDataPtr data, char* message, bool&) { return onUDPDataIn(data, message); });
	Hooks::Network::onUDPDataOut([this](UDPDataPtr data, char* message, bool&) { return onUDPDataOut(data, message); });
	Hooks::UI::onChatCommand([this](HubDataPtr hHub, CommandDataPtr cmd, bool&) { return onChatCommand(hHub, cmd); });

	Hooks::Users::onOffline([this](UserDataPtr hUser, bool&) { return onUserRemoved(hUser); });
	Hooks::Hubs::onOffline([this](HubDataPtr hHub, bool&) { return onHubRemoved(hHub); });

	Hooks::UI::onCreated([this](dcptr_t, bool&) -> bool { if(Config::getBoolConfig("Dialog")) { gui.create(); } return false; });
	UI::addCommand(showCommand, [this] { gui.create(); }, string());
	UI::addCommand(hideCommand, [this] { gui.close(); }, string());

	return true;
}

bool Plugin::onHubDataIn(HubDataPtr hHub, char* message) {
	gui.write(true, false, hHub->protocol, hHub->ip, hHub->port, "Hub <" + string(hHub->url) + ">", message);
	return false;
}

bool Plugin::onHubDataOut(HubDataPtr hHub, char* message) {
	gui.write(true, true, hHub->protocol, hHub->ip, hHub->port, "Hub <" + string(hHub->url) + ">", message);
	return false;
}

namespace { string userInfo(ConnectionDataPtr hConn) {
	auto user = Connections::handle()->get_user(hConn);
	string ret = user ? string(user->nick) + " <" + string(user->hubHint) + ">" : "[unknown]";
	if(user) { Hubs::handle()->release_user(user); }
	return ret;
} }

bool Plugin::onClientDataIn(ConnectionDataPtr hConn, char* message) {
	gui.write(false, false, hConn->protocol, hConn->ip, hConn->port, "User " + userInfo(hConn), message);
	return false;
}

bool Plugin::onClientDataOut(ConnectionDataPtr hConn, char* message) {
	gui.write(false, true, hConn->protocol, hConn->ip, hConn->port, "User " + userInfo(hConn), message);
	return false;
}

bool Plugin::onUDPDataIn(UDPDataPtr data, char* message) {
	string proto;
	if(message[0] == '$') { proto = "NMDC Search"; }
	else { proto = "ADC Search"; }
	gui.write(false, false, (ProtocolType)3, data->ip, data->port, proto, message);
	return false;
}

bool Plugin::onUDPDataOut(UDPDataPtr data, char* message) {
	string proto;
	if(message[0] == '$') { proto = "NMDC Search"; }
	else { proto = "ADC Search"; }
	gui.write(false, true, (ProtocolType)3, data->ip, data->port, proto, message);
	return false;
}

bool Plugin::onChatCommand(HubDataPtr hub, CommandDataPtr cmd) {
	if(stricmp(cmd->command, "help") == 0) {
		Hubs::handle()->local_message(hub, "/raw <message>", MSG_SYSTEM);

	} else if(stricmp(cmd->command, "raw") == 0) {
		if(strlen(cmd->params) == 0) {
			Hubs::handle()->local_message(hub, "Specify a message to send", MSG_SYSTEM);
		} else {
			Hubs::handle()->send_protocol_cmd(hub, cmd->params);
		}
	}

	return false;
}

bool Plugin::onUserRemoved(UserDataPtr hUser) {
	//Todo when I can get the users IP
//	gui.cleanFilterW(...);
	return false;
}

bool Plugin::onHubRemoved(HubDataPtr hHub) {
	gui.cleanFilterW(hHub->ip);
	return false;
}
