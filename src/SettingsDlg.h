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

#ifndef PLUGINS_DEV_SETTINGSDLG_H
#define PLUGINS_DEV_SETTINGSDLG_H

#include <unordered_set>

#include <dwt/widgets/ColorDialog.h>
#include <dwt/forward.h>
#include <dwt/widgets/ModalDialog.h>

#include "GUI.h"

class SettingsDlg : public dwt::ModalDialog {
	typedef dwt::ModalDialog BaseType;

public:
	SettingsDlg(dwt::Widget* parent);
	virtual ~SettingsDlg();

	int run();

private:

	bool handleInitDialog();
	void ok();

	void layout();

	dwt::GridPtr grid;

	string log;
	string timestamp;

	dwt::ColorDialog colorDlg;
	void colorDialog(COLORREF color, GUI::COLOR_FLAGS colorFlag);
};

#endif
