/*
* Copyright (C) 2022 iceman50
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
#include "SettingsDlg.h"
#include "GUI.h"

#include <pluginsdk/Config.h>
#include <pluginsdk/Logger.h>
#include <pluginsdk/Util.h>

#include <dwt/widgets/Button.h>
#include <dwt/widgets/CheckBox.h>
#include <dwt/widgets/ColorDialog.h>
#include <dwt/widgets/ComboBox.h>
#include <dwt/widgets/FontDialog.h>
#include <dwt/widgets/Grid.h>
#include <dwt/widgets/GroupBox.h>
#include <dwt/widgets/Label.h>
#include <dwt/widgets/LoadDialog.h>
#include <dwt/widgets/MessageBox.h>
#include <dwt/widgets/SaveDialog.h>
#include <dwt/widgets/TextBox.h>

// dwt defines another tstring...
typedef tstring _tstring;
#define tstring _tstring

using dcapi::Config;
using dcapi::Logger;
using dcapi::Util;

using namespace dwt;

SettingsDlg::SettingsDlg(dwt::Widget* parent) :
	BaseType(parent),
	grid(nullptr)
{
	onInitDialog([this] { return handleInitDialog(); });
}

SettingsDlg::~SettingsDlg() {
}

int SettingsDlg::run() {
	create(Seed(dwt::Point(500, 270)));
	return show();
}

bool SettingsDlg::handleInitDialog() {
	grid = addChild(Grid::Seed(2, 1));
	grid->column(0).mode = GridInfo::FILL;
	grid->setSpacing(6);

	{
		//Row 1...
		auto cur = grid->addChild(Grid::Seed(3, 1));
		cur->column(4).mode = GridInfo::FILL;
		cur->column(4).align = GridInfo::BOTTOM_RIGHT;
		cur->setSpacing(20);

		{
			Button::Seed bs;
			bs.padding.x += 20;

			auto cur2 = cur->addChild(GroupBox::Seed(_T("Colors")))->addChild(Grid::Seed(1, 4));

			bs.caption = _T("Background Color");
			cur2->addChild(bs)->onClicked([this] { colorDialog(Config::getIntConfig("BgColor"), GUI::COLOR_BG); });

			bs.caption = _T("UDP Color");
			cur2->addChild(bs)->onClicked([this] { colorDialog(Config::getIntConfig("UDPColor"), GUI::COLOR_UDP); });

			bs.caption = _T("NMDC Color");
			cur2->addChild(bs)->onClicked([this] { colorDialog(Config::getIntConfig("NMDCColor"), GUI::COLOR_NMDC); });

			bs.caption = _T("ADC Color");
			cur2->addChild(bs)->onClicked([this] { colorDialog(Config::getIntConfig("ADCColor"), GUI::COLOR_ADC); });
		}

		{
			//Timestamp format
			auto cur2 = cur->addChild(GroupBox::Seed(_T("Timestamp format")))->addChild(Grid::Seed(1, 1));
			cur2->column(0).size = 100;

			timestamp = Config::getConfig("TimeStampFormat");

			TextBox::Seed seed(Util::toT(timestamp));
			seed.style |= ES_AUTOHSCROLL;
			auto box = cur2->addChild(seed);
			box->setTextLimit(20);
			box->onUpdated([this, box] { timestamp = Util::fromT(box->getText()); Config::setConfig("TimeStampFormat", timestamp); });
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
				if(SaveDialog(this).open(file)) {
					log = Util::fromT(file);
					Config::setConfig("Log", log);
					box->setText(file);
				}
			});		
		}
	}

	{ //OK & Cancel need to be our last row
		auto cur = grid->addChild(Grid::Seed(1, 1));
		cur->column(0).mode = GridInfo::FILL;
		cur->column(0).align = GridInfo::BOTTOM_RIGHT;
		cur->setSpacing(grid->getSpacing());

		Button::Seed bs(_T("Close")); // Save
		bs.style |= BS_DEFPUSHBUTTON;
		bs.padding.x = 20;
		cur->addChild(bs)->onClicked([this] { ok(); });
	}

	setText(_T("Dev plugin settings"));

	layout();
	centerWindow();

	return false;
}

void SettingsDlg::ok() {
	endDialog(IDOK);
}

void SettingsDlg::layout() {
	auto size = getClientSize();
	grid->resize(dwt::Rectangle(3, 3, size.x - 6, size.y - 6));
}

void SettingsDlg::colorDialog(COLORREF color, GUI::COLOR_FLAGS colorFlag) {
		ColorDialog::ColorParams params(color);
		if(ColorDialog(this).open(params)) {
		switch (colorFlag) {
			case GUI::COLOR_ADC:
				Config::setConfig("ADCColor", static_cast<int>(params.getColor()));
				break;
			case GUI::COLOR_NMDC:
				Config::setConfig("NMDCColor", static_cast<int>(params.getColor()));
				break;
			case GUI::COLOR_UDP:
				Config::setConfig("UDPColor", static_cast<int>(params.getColor()));
				break;
			case GUI::COLOR_BG:
				Config::setConfig("BgColor", static_cast<int>(params.getColor()));
				GUI::setColor(RGB(0, 0, 0), params.getColor());
		}
	}
	GUI::redrawTable();
}
