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

#ifndef PLUGINS_DEV_GUI_H
#define PLUGINS_DEV_GUI_H

#include <unordered_set>

#include <boost/lockfree/queue.hpp>
#include <boost/regex.hpp>

using std::move;
using std::string;
using std::unordered_set;

class GUI
{
public:
	GUI();
	~GUI();

	void create();
	void write(bool hubOrUser, bool sending, ProtocolType proto, string ip, decltype(ConnectionData().port) port, string peer, string message);
	void close();

	// Temp solution to try and keep FilterW from having entries of offline hubs/users
	void cleanFilterW(string ip);

	static bool unloading;

private:
	void timer();
	void initFilter();
	void copy();
	void clear();
	void remove();

	// store the messages to be displayed here; process them with a timer.
	struct Message { bool hubOrUser; bool sending; string protocol; string ip; decltype(ConnectionData().port) port; string peer; string message; };
	boost::lockfree::queue<Message*> messages;

	uint16_t counter;
	bool scroll;
	bool hubMessages;
	bool userMessages;
	unordered_set<tstring> filter;
	tstring filterSel;
	boost::regex regex;
	string log;

	// objects associated to each list litem as LPARAMs.
	struct Item {
		tstring index;
		tstring dir;
		tstring protocol;
		tstring ip;
		tstring port;
		tstring peer;
		tstring message;
	};

	string returnProto(ProtocolType proto);
};

#endif
