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

//#include "ListFilter.h"

#include <unordered_set>

#include <boost/lockfree/queue.hpp>
#include <boost/regex.hpp>

#include <dwt/Application.h>

using std::move;
using std::string;
using std::unordered_set;

constexpr ProtocolType PROTOCOL_UDP = static_cast<ProtocolType>(3);
//constexpr ProtocolType PROTOCOL_HTTP = static_cast<ProtocolType>(4);

// objects associated to each list item as LPARAMs.
struct Item {
	tstring timestamp;
	tstring index;
	tstring dir;
	tstring protocol;
	tstring ip;
	tstring port;
	tstring peer;
	tstring message;
};

struct ColumnInfo {
	const char* name;
	const int size;
	const bool numerical;
};

enum {
	COLUMN_FIRST,
	COLUMN_TIMESTAMP = COLUMN_FIRST,
	COLUMN_COUNT,
	COLUMN_DIRECTION,
	COLUMN_PROTOCOL,
	COLUMN_IP,
	COLUMN_PORT,
	COLUMN_PEER,
	COLUMN_MESSAGE,

	COLUMN_LAST
};

enum { ALL, ADC, NMDC, DHT, UDP };

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
	
	static void redrawTable();
	static void setColor(COLORREF text, COLORREF bg);

	static bool unloading;

	enum COLOR_FLAGS {
		COLOR_ADC = 0x01,
		COLOR_NMDC = 0x02,
		COLOR_UDP = 0x04,
		COLOR_BG = 0x08
	};

private:
	void timer();
	void initFilter(tstring opt = tstring());
	void copy();
	void clear();
	void remove();

	// store the messages to be displayed here; process them with a timer.
	struct Message { bool hubOrUser; bool sending; string protocol; string ip; decltype(ConnectionData().port) port; string peer; string message; };
	boost::lockfree::queue<Message*> messages;

	uint32_t counter;
	bool scroll;
	bool hubMessages;
	bool userMessages;
	unordered_set<tstring> filter;
	tstring filterSel;
	boost::regex regex;
	string log;

	LRESULT handleCustomDraw(NMLVCUSTOMDRAW& data);
	void openDoc();
};

#endif
