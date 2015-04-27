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

#include <pluginsdk/Config.h>
#include <pluginsdk/Util.h>

#include <memory>

#include <boost/lexical_cast.hpp>

#define noFilter _T("0 - No filtering")

// dwt defines another tstring...
typedef tstring _tstring;
#define tstring _tstring

using dcapi::Config;
using dcapi::Util;

using namespace dwt;

bool GUI::unloading = false;

WindowPtr window;
TablePtr table;
ComboBoxPtr filterW;

/*
ComboBoxPtr userFilter, hubFilter, SearchFilter, protoFilter
CheckBoxPtr eUserFilter, eHubFilter, eSearchFilter, eProtoFilter, eFileLogging, eColorFormat
GroupBoxPtr searchGroup, filterGroup, miscGroup, colorGroup
*/

GUI::GUI() :
	messages(1024),
	counter(0),
	scroll(true),
	hubMessages(true),
	userMessages(true)/* hubMessages and userMessages will be superceded by eUserFilter & eHubFilter
	,eUserFilter(true),
	eHubFilter(true),
	eSearchFilter(true),
	eProtoFilter(true),
	eFileLogging(false)				  
					  
					  */
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

	Config::setConfig("Dialog", true);
	/*
	auto setBoolCfg = [&] (string setting, CheckBoxPtr enable) {
		if(enable) {
			Config::setConfig(setting, enable);
	}

	setBoolCfg("", eUserFilter);
	setCfg("", eHubFilter);
	setCfg("", eSearchFilter);
	setCfg("", eProtoFilter);
	setCfg("", eFileLogging);
	*/

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

	auto grid = window->addChild(Grid::Seed(3, 1));
	grid->column(0).mode = GridInfo::FILL;
	grid->row(0).mode = GridInfo::FILL;
	grid->row(0).align = GridInfo::STRETCH;
	grid->setSpacing(8);

	{
		Table::Seed seed;
		seed.style |= WS_BORDER | LVS_SHOWSELALWAYS;
		seed.lvStyle = LVS_EX_DOUBLEBUFFER | LVS_EX_HEADERDRAGDROP | LVS_EX_FULLROWSELECT | LVS_EX_LABELTIP;
		table = grid->addChild(seed);

		std::vector<Column> columns;
		columns.emplace_back(_T("#"), 50);
		columns.emplace_back(_T("Dir"), 50);
		columns.emplace_back(_T("Protocol"), 60);
		columns.emplace_back(_T("IP"), 100);
		columns.emplace_back(_T("Port"), 50);
		columns.emplace_back(_T("Peer info"), 200);
		columns.emplace_back(_T("Message"));
		table->setColumns(columns);
		table->onSized([this](const SizedEvent& e) { table->setColumnWidth(6, e.size.x - 50 - 50 - 60 - 100 - 50 - 200 - 20); });

		table->onContextMenu([this](const ScreenCoordinate& pt) -> bool {
			auto menu = window->addChild(Menu::Seed());
			auto hasSel = table->hasSelected();
			menu->appendItem(_T("Copy selected messages"), [=] { copy(); }, nullptr, hasSel);
			menu->appendItem(_T("Remove selected messages"), [=] { remove(); }, nullptr, hasSel);
			menu->appendSeparator();
			menu->appendItem(_T("Select all"), [] { table->selectAll(); }, nullptr, !table->empty());
			menu->open(pt.x() == -1 || pt.y() == -1 ? table->getContextMenuPos() : pt);
			return true;
		});

		table->onCustomDraw([this](NMLVCUSTOMDRAW& data) { return GUI::handleCustomDraw(data); });
	}

	{
		auto cur = grid->addChild(Grid::Seed(1, 5));
		cur->setSpacing(30);

		auto hubMessagesW = cur->addChild(CheckBox::Seed(_T("Add hub messages")));
		hubMessagesW->setChecked(true);
		hubMessagesW->onClicked([this, hubMessagesW] { hubMessages = hubMessagesW->getChecked(); });

		auto userMessagesW = cur->addChild(CheckBox::Seed(_T("Add user messages")));
		userMessagesW->setChecked(true);
		userMessagesW->onClicked([this, userMessagesW] { userMessages = userMessagesW->getChecked(); });

		{
			ComboBox::Seed seed;
			seed.style |= CBS_DROPDOWNLIST | CBS_SORT;
			filterW = cur->addChild(seed);
			initFilter();
			filterW->onSelectionChanged([this] {
				auto str = filterW->getText();
				if(str == noFilter) {
					filterSel.clear();
				} else {
					filterSel = move(str);
				}
			});
		}

		{
			auto cur2 = cur->addChild(GroupBox::Seed(_T("Regex filter")))->addChild(Grid::Seed(1, 2));
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
				} catch(const std::runtime_error&) {
					dwt::MessageBox(window).show(_T("Invalid regular expression"), window->getText(),
						dwt::MessageBox::BOX_OK, dwt::MessageBox::BOX_ICONEXCLAMATION);
				}
			});
		}

		{
			auto cur2 = cur->addChild(GroupBox::Seed(_T("Log to a file")))->addChild(Grid::Seed(1, 2));
			cur2->column(0).size = 250;

			log = Config::getConfig("Log");

			TextBox::Seed seed(Util::toT(log));
			seed.style |= ES_AUTOHSCROLL;
			auto box = cur2->addChild(seed);
			box->onUpdated([this, box] { log = Util::fromT(box->getText()); Config::setConfig("Log", log); });

			Button::Seed bs(_T("Browse"));
			bs.padding.x += 8;
			cur2->addChild(bs)->onClicked([this, box] {
				auto file = Util::toT(log);
				if(SaveDialog(window).open(file)) {
					log = Util::fromT(file);
					Config::setConfig("Log", log);
					box->setText(file);
				}
			});
		}
	}

	{
		auto cur = grid->addChild(Grid::Seed(1, 5));
		cur->column(4).mode = GridInfo::FILL;
		cur->column(4).align = GridInfo::BOTTOM_RIGHT;
		cur->setSpacing(30);

		Button::Seed bs;
		bs.padding.x += 20;

		bs.caption = _T("Copy selected messages");
		cur->addChild(bs)->onClicked([this] { copy(); });

		auto scrollW = cur->addChild(CheckBox::Seed(_T("Auto-scroll")));
		scrollW->setChecked(true);
		scrollW->onClicked([this, scrollW] { scroll = scrollW->getChecked(); });

		bs.caption = _T("Clear the list");
		cur->addChild(bs)->onClicked([this] { clear(); });

		auto onTop = cur->addChild(CheckBox::Seed(_T("Keep this window on top")));
		onTop->onClicked([onTop] { window->setZOrder(onTop->getChecked() ? HWND_TOPMOST : HWND_NOTOPMOST); });

		bs.caption = _T("Close");
		bs.style |= BS_DEFPUSHBUTTON;
		bs.padding.x += 20;
		cur->addChild(bs)->onClicked([] { window->close(true); });
	}

	grid->resize(window->getClientSize());
	window->onSized([grid](const SizedEvent& e) { grid->resize(e.size); });

	table->setFocus();

	window->setTimer([this]() -> bool { timer(); return true; }, 500);
}

void GUI::write(bool hubOrUser, bool sending, ProtocolType proto, string ip, decltype(ConnectionData().port) port, string peer, string message) {
	auto msg = new Message();
	msg->hubOrUser = hubOrUser;
	msg->sending = sending;
	msg->protocol = returnProto(proto);
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


		//TODO Handle filtering here ... if(message != isFiltered(message)) { continue; }
		if(!(message.hubOrUser ? hubMessages : userMessages)) {
			continue;
		}

		auto ip = Util::toT(message.ip);
		if(!filterSel.empty() && ip != filterSel) {
			continue;
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

		if(f) {
			fprintf(f, "%u [%s] [%s] %s:%u (%s): %s\n", counter, message.sending ? "Out" : "In",
				message.protocol.c_str(), message.ip.c_str(), message.port, message.peer.c_str(), message.message.c_str());
		}

		auto item = new Item;
		item->index = Util::toT(boost::lexical_cast<string>(counter));
		item->dir = message.sending ? _T("Out") : _T("In");
		item->protocol = Util::toT(message.protocol);
		item->ip = move(ip);
		item->port = Util::toT(boost::lexical_cast<string>(message.port));
		item->peer = Util::toT(message.peer);
		item->message = Util::toT(message.message);

		std::vector<tstring> row;
		row.push_back(item->index);
		row.push_back(item->dir);
		row.push_back(item->protocol);
		row.push_back(item->ip);
		row.push_back(item->port);
		row.push_back(item->peer);
		row.push_back(item->message);
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

void GUI::initFilter() {
	filterW->setSelected(filterW->addValue(noFilter));
}

void GUI::copy() {
	tstring str;

	int i = -1;
	while((i = table->getNext(i, LVNI_SELECTED)) != -1) {
		auto data = table->getData(i);
		if(data) {
			auto& item = *reinterpret_cast<Item*>(data);
			if(!str.empty()) { str += _T("\r\n"); }
			str += item.index + _T(" [") + item.dir + _T("] ") + _T(" [") + item.protocol + _T("] ") + item.ip + _T(":") + item.port + _T(" (") + item.peer + _T("): ") + item.message;
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

	filterW->clear();
	filter.clear();
	filterSel.clear();
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
	if (filter.find(Util::toT(ip)) != filter.end()) {
		filterW->erase(filterW->findString(Util::toT(ip)));
	}
}

string GUI::returnProto(ProtocolType protocol) {
	switch (protocol) {
		case PROTOCOL_ADC: return "ADC"; break;
		case PROTOCOL_NMDC: return "NMDC"; break;
		case PROTOCOL_DHT: return "DHT"; break; // Reserved
		case 3/* UDP */: return "UDP"; break; //Specifically for UDP data since there is no ProtocolType for UDP
		default: return "Unknown";
	}
}

LRESULT GUI::handleCustomDraw(NMLVCUSTOMDRAW& data) {
	auto item = static_cast<int>(data.nmcd.dwItemSpec);
	auto column = data.iSubItem;

	if (data.nmcd.dwDrawStage == CDDS_PREPAINT) {
		return CDRF_NOTIFYITEMDRAW;
	}

	if ((data.nmcd.dwDrawStage & CDDS_ITEMPREPAINT) == CDDS_ITEMPREPAINT && data.dwItemType == LVCDI_ITEM && data.nmcd.lItemlParam) {

		RECT r;
		ListView_GetItemRect(table->handle(), item, &r, LVIR_BOUNDS);

		Item* it = (Item*)data.nmcd.lItemlParam;

		if (data.nmcd.hdr.hwndFrom == table->handle()) {
			data.clrTextBk = RGB(0, 0, 0);

			if (/*(eColorFormat) &&*/ it->protocol == _T("ADC")) {
				data.clrText = RGB(255, 51, 51);
			} else if (it->protocol == _T("NMDC")) {
				data.clrText = RGB(102, 0, 204);
			} else if (it->protocol == _T("UDP")) {
				data.clrText = RGB(0, 255, 28);
			}

		}
		
		/*
		HFONT font = nullptr;
		auto ret = data.nmcd.lItemlParam;
		if (ret == CDRF_NEWFONT && font) {
			::SelectObject(data.nmcd.hdc, font);
		}
		return ret;
		*/
	}

	return CDRF_DODEFAULT;
}