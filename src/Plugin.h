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

#ifndef PLUGIN_PLUGIN_H
#define PLUGIN_PLUGIN_H

#include "GUI.h"

using std::string;

class Plugin
{
public:
	static Bool DCAPI main(PluginState state, DCCorePtr core, dcptr_t);

private:
	Plugin();
	~Plugin();

	bool onLoad(DCCorePtr core, bool install, bool runtime);
	bool onHubDataIn(HubDataPtr hHub, char* message);
	bool onHubDataOut(HubDataPtr hHub, char* message);
	bool onClientDataIn(ConnectionDataPtr hConn, char* message);
	bool onClientDataOut(ConnectionDataPtr hConn, char* message);
	bool onUDPDataIn(UDPDataPtr data, char* message);
	bool onUDPDataOut(UDPDataPtr data, char* message);
	bool onChatCommand(HubDataPtr hub, CommandDataPtr cmd);
	
	bool onUserRemoved(UserDataPtr hUser);
	bool onHubRemoved(HubDataPtr hHub);

	GUI gui;

	string commandName;
};

#endif
