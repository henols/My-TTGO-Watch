/****************************************************************************
 *   2021-03-19
 *   Copyright  2021  Henrik Olsson
 *   Email: henols@gmail.com
 ****************************************************************************/

/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
#ifndef _HASS_APP_H
#define _HASS_APP_H

#include <TTGO.h>

//enum HassAppSettingsAction {
//	Ignore, Load, Save
//};

struct HassSensor;

void HassApp_setup(void);
bool HassApp_bluetooth_event_cb(EventBits_t event, void *arg);
void HassApp_build_main(void);
void HassApp_build_settings(void);
//void execute_ir_cmd(HassSensor *config);

#endif // _HASS_APP_H
