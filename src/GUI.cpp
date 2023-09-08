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
#include "GUI.h"
#include "SettingsDlg.h"

#include <pluginsdk/Config.h>
#include <pluginsdk/Core.h>
#include <pluginsdk/Util.h>

#include <dwt/Clipboard.h>
#include <dwt/DWTException.h>
#include <dwt/Events.h>
#include <dwt/util/HoldRedraw.h>
#include <dwt/widgets/Button.h>
#include <dwt/widgets/CheckBox.h>
#include <dwt/widgets/ComboBox.h>
#include <dwt/widgets/Grid.h>
#include <dwt/widgets/GroupBox.h>
#include <dwt/widgets/Label.h>
#include <dwt/widgets/Menu.h>
#include <dwt/widgets/MessageBox.h>
#include <dwt/widgets/SaveDialog.h>
#include <dwt/widgets/Table.h>
#include <dwt/widgets/TextBox.h>
#include <dwt/widgets/Window.h>

#include <ctime>
#include <memory>

#include <boost/boost/lexical_cast.hpp>

#define noFilter _T("::0 - No filtering")// ::0 ensures this will stay at the top of the list when IPv6 is used e.g ::1

// dwt defines another tstring...
typedef tstring _tstring;
#define tstring _tstring

using dcapi::Config;
using dcapi::Util;

using namespace dwt;

bool GUI::unloading = false;

WindowPtr window;
TablePtr table;
ComboBoxPtr filterW, filterP;
//ListFilter filter;

static const ColumnInfo cols[] = {
	{ "Timestamp", 120, false },
	{ "#", 50, true },
	{ "Dir", 50, false },
	{ "Protocol", 60, false },
	{ "IP", 100, false },
	{ "Port", 50, true },
	{ "Peer info", 200, false },
	{ "Message", 520, false }	
};

namespace {
void makeColumns(dwt::TablePtr table_, const ColumnInfo* columnInfo, size_t columnCount) {
	std::vector<dwt::Column> n(columnCount);
	std::vector<int> o(columnCount);
	
	for(size_t i = 0; i < columnCount; ++i) {
		n[i].header = Util::toT(columnInfo[i].name);
		n[i].width = columnInfo[i].size;
		n[i].alignment = columnInfo[i].numerical ? dwt::Column::RIGHT : dwt::Column::LEFT;
		o[i] = i;
	}

	table_->setColumns(n);
}
} //unnamed namespace

GUI::GUI() :
	messages(1024),
	counter(0),
	scroll(true),
	hubMessages(true),
	userMessages(true)
{
}

GUI::~GUI() {
	if(window) {
		::DestroyWindow(window->handle());
		window = nullptr;
		Application::uninit();
	}
}

void GUI::create() {
	if(window) {
		window->setFocus();
		return;
	}

	if(Config::getBoolConfig("FirstRun")) {
		Config::setConfig("FirstRun", false);
		//Offload these settings from Plugin
		Config::setConfig("BgColor", static_cast<int>(RGB(255, 255, 255)));
		Config::setConfig("ADCColor", static_cast<int>(RGB(0, 0, 0)));
		Config::setConfig("NMDCColor", static_cast<int>(RGB(0, 0, 0)));
		Config::setConfig("UDPColor", static_cast<int>(RGB(0, 0, 0)));
		Config::setConfig("TimeStampFormat", "[%D - %H:%M:%S]");
	}

	Config::setConfig("Dialog", true);
	if(Config::getConfig("TimeStampFormat").empty()) { // Updating to 1.2 plugins will not have this saved
		Config::setConfig("TimeStampFormat", "[%D - %H:%M:%S]");
	}

	Application::init();

	{
		Window::Seed seed(_T(PLUGIN_NAME));
		seed.location.size.x = 1200;
		seed.location.size.y = 800;

		window = new Window();
		window->create(seed);

		auto iconPath = Util::toT(Config::getInstallPath() + "DevPlugin.ico");
		try {
			window->setSmallIcon(new dwt::Icon(iconPath, dwt::Point(16, 16)));
			window->setLargeIcon(new dwt::Icon(iconPath, dwt::Point(32, 32)));
		} catch(const dwt::DWTException&) { }

		window->onClosing([]() -> bool {
			window = nullptr;
			Application::uninit();
			if(!unloading) { Config::setConfig("Dialog", false); }
			return true;
		});
		window->onDestroy([this] { clear(); });
	}

	auto grid = window->addChild(Grid::Seed(2, 1));
	grid->column(0).mode = GridInfo::FILL;
	grid->row(0).mode = GridInfo::FILL;
	grid->row(0).align = GridInfo::STRETCH;
	grid->setSpacing(8);

	{
		Table::Seed seed;
		seed.style |= WS_BORDER | LVS_SHOWSELALWAYS;
		seed.lvStyle = LVS_EX_DOUBLEBUFFER | LVS_EX_HEADERDRAGDROP | LVS_EX_FULLROWSELECT | LVS_EX_LABELTIP;
		table = grid->addChild(seed);
		makeColumns(table, cols, COLUMN_LAST);
		table->onSized([this](const SizedEvent& e) { table->setColumnWidth(8, e.size.x - 50 - 50 - 60 - 100 - 50 - 75 - 20); });

		table->onContextMenu([this](const ScreenCoordinate& pt) -> bool {
			auto menu = window->addChild(Menu::Seed());
			auto hasSel = table->hasSelected();
			menu->appendItem(_T("Copy selected messages"), [=] { copy(); }, nullptr, hasSel);
			menu->appendItem(_T("Remove selected messages"), [=] { remove(); }, nullptr, hasSel);
			menu->appendSeparator();
			menu->appendItem(_T("Select all"), [] { table->selectAll(); }, nullptr, !table->empty());

			menu->appendSeparator();
			menu->appendItem(_T("Open protocol documentation"), [=] { openDoc(); }, nullptr, hasSel);

			menu->appendSeparator();
			menu->appendItem(_T("Open settings"), [=] { SettingsDlg(window).run(); }, nullptr, true);

			menu->open(pt.x() == -1 || pt.y() == -1 ? table->getContextMenuPos() : pt);
			return true;
		});

		table->onCustomDraw([this](NMLVCUSTOMDRAW& data) { return GUI::handleCustomDraw(data); });
	}

	{
		auto label = grid->addChild(GroupBox::Seed(_T("Options")))->addChild(Grid::Seed(2, 1)); // 
		auto cur = label->addChild(Grid::Seed(2, 2));
		cur->setSpacing(10);

		{
			auto cur2 = cur->addChild(GroupBox::Seed(_T("Filtering options")))->addChild(Grid::Seed(1, 4));
			cur2->column(0).size = 75;
			cur2->column(2).size = 100;

			Label::Seed ls;
			ls.style |= SS_CENTER;

			//Centered Label "Filter by IP" - col(0)
			ls.caption = _T("Filter by IP");
			cur2->addChild(ls);

			{
				ComboBox::Seed seed;
				seed.style |= CBS_DROPDOWNLIST | CBS_SORT;
				filterW = cur2->addChild(seed);
				initFilter();
				filterW->onSelectionChanged([this] {
					auto str = filterW->getText();
				if (str == noFilter) {
				filterSel.clear();
				} else {
					filterSel = move(str);
				}
				});
			}

			//Centered Label "Filter by protocol" - col(2)
			ls.caption = _T("Filter by protocol");
			cur2->addChild(ls);

			{
				ComboBox::Seed seed;
				seed.style |= CBS_DROPDOWNLIST;
				filterP = cur2->addChild(seed);
				filterP->addValue(Util::toT("Show all"));
				filterP->addValue(Util::toT("ADC only"));
				filterP->addValue(Util::toT("NMDC only"));
				if(dcapi::Core::appName != "DC++") { //afaik ApexDC++ uses DHT for the time being this will be fine
					filterP->addValue(Util::toT("DHT only")); //DC++ Not implemented
				}
				filterP->addValue(Util::toT("UDP only"));
				filterP->setSelected(ALL);
			}
		}

		{
			auto cur2 = cur->addChild(GroupBox::Seed(_T("Regex filter (only applies to messages)")))->addChild(Grid::Seed(1, 2));
			cur2->column(0).size = 250;

			TextBox::Seed seed;
			seed.style |= ES_AUTOHSCROLL;
			auto box = cur2->addChild(seed);

			Button::Seed bs(_T("Apply"));
			bs.padding.x += 8;
			cur2->addChild(bs)->onClicked([this, box] {
				regex = "";
			try {
				regex.assign(Util::fromT(box->getText()));
			}catch (const std::runtime_error&) {
				dwt::MessageBox(window).show(_T("Invalid regular expression"), window->getText(),
											 dwt::MessageBox::BOX_OK, dwt::MessageBox::BOX_ICONEXCLAMATION);
			}
			});
		}

		{
			auto cur2 = label->addChild(Grid::Seed(1, 8));
			cur2->column(6).size = 250; // Set up spacing for the close button
			cur2->column(7).align = GridInfo::BOTTOM_RIGHT;
			cur2->setSpacing(20);

			auto hubMessagesW = cur2->addChild(CheckBox::Seed(_T("Add hub messages")));
			hubMessagesW->setChecked(true);
			hubMessagesW->onClicked([this, hubMessagesW] { hubMessages = hubMessagesW->getChecked(); });

			auto userMessagesW = cur2->addChild(CheckBox::Seed(_T("Add user messages")));
			userMessagesW->setChecked(true);
			userMessagesW->onClicked([this, userMessagesW] { userMessages = userMessagesW->getChecked(); });

			auto scrollW = cur2->addChild(CheckBox::Seed(_T("Auto-scroll")));
			scrollW->setChecked(true);
			scrollW->onClicked([this, scrollW] { scroll = scrollW->getChecked(); });

			auto onTop = cur2->addChild(CheckBox::Seed(_T("Keep this window on top")));
			onTop->onClicked([onTop] { window->setZOrder(onTop->getChecked() ? HWND_TOPMOST : HWND_NOTOPMOST); });

			Button::Seed bs;
			bs.padding.x += 20;

			bs.caption = _T("Clear the list");
			cur2->addChild(bs)->onClicked([this] { clear(); });

			bs.caption = _T("Open settings");
			cur2->addChild(bs)->onClicked([this] { SettingsDlg(window).run(); });

			bs.caption = _T("Close");
			bs.style |= BS_DEFPUSHBUTTON;
			bs.padding.x += 20;
			Button* bp = cur2->addChild(bs);
			bp->onClicked([] { window->close(true); });
			cur2->setWidget(bp, 0, 7);
		}
	}

	grid->resize(window->getClientSize());
	window->onSized([grid](const SizedEvent& e) { grid->resize(e.size); });

	table->setFocus();
	// Ensure we run setColor every time we open the dialog to maintain background color
	table->setColor(RGB(0, 0, 0), Config::getIntConfig("BgColor"));

	window->setTimer([this]() -> bool { timer(); return true; }, 500);
}

void GUI::write(bool hubOrUser, bool sending, ProtocolType proto, string ip, decltype(ConnectionData().port) port, string peer, string message) {

	auto pStr = [&](ProtocolType p) -> string {
		switch (p) {
			case PROTOCOL_ADC: return "ADC"; break;
			case PROTOCOL_NMDC: return "NMDC"; break;
			case PROTOCOL_DHT: return "DHT"; break; // Reserved
			case PROTOCOL_UDP: return "UDP"; break; // use our own definition for UDP
			default: return "Unknown";
		}
	};

	auto msg = new Message();
	msg->hubOrUser = hubOrUser;
	msg->sending = sending;
	msg->protocol = pStr(proto);
	msg->ip = move(ip);
	msg->port = port;
	msg->peer = move(peer);
	msg->message = move(message);
	messages.push(msg);
}

void GUI::close() {
	if(window) {
		window->close();
	}
}

void GUI::timer() {
	int pos = -1;

	FILE* f = nullptr;
	if(!log.empty()) {
		f = fopen(log.c_str(), "a");
	}

	std::unique_ptr<Message> messagePtr;
	while(messages.pop(messagePtr)) {
		auto& message = *messagePtr.get();

		if(!(message.hubOrUser ? hubMessages : userMessages)) {
			continue;
		}

		auto ip = Util::toT(message.ip);
		if(!filterSel.empty() && ip != filterSel) {
			continue;
		}

		auto pStr = filterP->getSelected();
		if(pStr != ALL) {
			if(pStr == ADC && message.protocol != "ADC") continue;
			if(pStr == NMDC && message.protocol != "NMDC") continue;
			if(pStr == DHT && message.protocol != "DHT") continue; //DC++ Not implemented
			if(pStr == UDP && message.protocol != "UDP") continue;
		}

		if(!regex.empty()) {
			try {
				if(!boost::regex_search(message.message, regex)) {
					continue;
				}
			} catch(const std::runtime_error&) {
				// most likely a stack overflow, ignore...
				continue;
			}
		}

		const auto& timeFmt = Config::getConfig("TimeStampFormat");

		char timestamp [50] {};
		std::time_t time = std::time(nullptr);
		std::strftime(timestamp, 30, timeFmt.c_str(), std::localtime(&time));
		auto timeStr = string(timestamp);

		if(f) {
			fprintf(f, "%s %u [%s] [%s] %s:%u (%s): %s\n", timeStr.c_str(), counter, message.sending ? "Out" : "In",
				message.protocol.c_str(), message.ip.c_str(), message.port, message.peer.c_str(), message.message.c_str());
		}

		auto item = new Item;
		item->timestamp = Util::toT(timeStr);
		item->index = Util::toT(boost::lexical_cast<string>(counter));
		item->dir = message.sending ? _T("Out") : _T("In");
		item->protocol = Util::toT(message.protocol);
		item->ip = move(ip);
		item->port = Util::toT(boost::lexical_cast<string>(message.port));
		item->peer = Util::toT(message.peer);
		item->message = Util::toT(message.message);

		std::vector<tstring> row = { 
			item->timestamp, 
			item->index, 
			item->dir, 
			item->protocol, 
			item->ip, 
			item->port, 
			item->peer, 
			item->message 
		};
		pos = table->insert(row, reinterpret_cast<LPARAM>(item));

		++counter;

		if(filter.find(item->ip) == filter.end()) {
			filterW->addValue(*filter.insert(item->ip).first);
		}
	}

	if(f) {
		fclose(f);
	}

	if(scroll && pos != -1) { // make sure something was added
		table->ensureVisible(pos);
	}
}

void GUI::initFilter(tstring opt) {
	filter.clear();
	filterSel.clear();
	filterW->clear();
	filterW->addValue(noFilter);

	if(opt.empty()) {
		filterW->setSelected(0); // noFilter ***SHOULD*** always be 0
	} else {
		filter.insert(opt); //We need to re-add the selected IP
		filterW->setSelected(filterW->addValue(opt));
		filterSel = opt;
	}
}

void GUI::copy() {
	tstring str;

	int i = -1;
	while((i = table->getNext(i, LVNI_SELECTED)) != -1) {
		auto data = table->getData(i);
		if(data) {
			auto& item = *reinterpret_cast<Item*>(data);
			if(!str.empty()) { str += _T("\r\n"); }
			str += item.timestamp + item.index + _T(" [") + item.dir + _T("] ") + _T(" [") + item.protocol + _T("] ") + item.ip + _T(":") + item.port + _T(" (") + item.peer + _T("): ") + item.message;
		}
	}

	dwt::Clipboard::setData(str, window);
}

void GUI::clear() {
	std::vector<Item*> toDelete;

	for(size_t i = 0, n = table->size(); i < n; ++i) {
		auto data = table->getData(i);
		if(data) {
			toDelete.push_back(reinterpret_cast<Item*>(data));
		}
	}

	table->clear();
	counter = 0;

	for(auto item: toDelete) {
		delete item;
	}

	const auto& temp = filterW->getText();
	if(!temp.empty() && temp != noFilter) {
		initFilter(temp);
		return;
	}

	initFilter();
}

void GUI::remove() {
	util::HoldRedraw hold(table);
	int i;
	while((i = table->getNext(-1, LVNI_SELECTED)) != -1) {
		auto data = table->getData(i);
		table->erase(i);
		if(data) {
			delete reinterpret_cast<Item*>(data);
		}
	}
}

void GUI::cleanFilterW(string ip) {
	if(filter.find(Util::toT(ip)) != filter.end()) {
		filterW->erase(filterW->findString(Util::toT(ip)));
		initFilter();
	}
}

LRESULT GUI::handleCustomDraw(NMLVCUSTOMDRAW& data) {
	const auto& item = static_cast<int>(data.nmcd.dwItemSpec);
	const auto& adcClr = static_cast<COLORREF>(Config::getIntConfig("ADCColor"));
	const auto& nmdcClr = static_cast<COLORREF>(Config::getIntConfig("NMDCColor"));
	const auto& udpClr = static_cast<COLORREF>(Config::getIntConfig("UDPColor"));

	if(data.nmcd.dwDrawStage == CDDS_PREPAINT) {
		return CDRF_NOTIFYITEMDRAW;
	}

	if((data.nmcd.dwDrawStage & CDDS_ITEMPREPAINT) == CDDS_ITEMPREPAINT && data.dwItemType == LVCDI_ITEM && data.nmcd.lItemlParam) {

		const auto& rect = table->getRect(item, LVIR_BOUNDS);

		Item* it = (Item*)data.nmcd.lItemlParam;
		if(data.nmcd.hdr.hwndFrom == table->handle()) {
			if(it->protocol == _T("ADC")) {
				data.clrText = adcClr;
			} else if(it->protocol == _T("NMDC")) {
				data.clrText = nmdcClr;
			} else if(it->protocol == _T("UDP")) {
				data.clrText = udpClr;
			}
		}
	}

	return CDRF_DODEFAULT;
}

void GUI::openDoc() {
	const tstring& ADC_Doc = _T("https://adc.sourceforge.net/ADC.html");
	const tstring& NMDC_Doc = _T("https://nmdc.sourceforge.net/NMDC.html");
	const tstring& UDP_Doc = _T("https://en.wikipedia.org/wiki/User_Datagram_Protocol");

	int i = -1;
	while((i = table->getNext(i, LVNI_SELECTED)) != -1) {
		auto data = table->getData(i);
		if(data) {
			const auto& item = *reinterpret_cast<Item*>(data);
			const auto isAdc = item.protocol == Util::toT("ADC");
			const auto isNmdc = item.protocol == Util::toT("NMDC");

			auto openLink = [](const tstring& doc) {
				::ShellExecute(0, 0, doc.c_str(), 0, 0, SW_SHOW);
			};

			if(isAdc) {
				openLink(ADC_Doc);
			} else if(isNmdc) {
				openLink(NMDC_Doc);
			} else {
				openLink(UDP_Doc);
			}

			break;
		}
	}
}

void GUI::redrawTable() {
	table->Control::redraw(true);
}

void GUI::setColor(COLORREF text, COLORREF bg) {
	table->setColor(text, bg);
}
