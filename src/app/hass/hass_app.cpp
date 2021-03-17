/****************************************************************************
 *   Sep 3 23:05:42 2020
 *   Copyright  2020  Dirk Brosswick
 *   Email: dirk.brosswick@googlemail.com
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
#include "config.h"
#include <TTGO.h>

#include "hass_app.h"
#include "hass_main.h"
#include "hass_setup.h"

#include "gui/mainbar/mainbar.h"
#include "gui/statusbar.h"
#include "gui/app.h"
#include "gui/widget.h"

#include "hardware/json_psram_allocator.h"

hass_config_t hass_config;

// app and widget icon
icon_t *hass_app = NULL;
icon_t *hass_widget = NULL;

uint32_t hass_app_main_tile_num;
uint32_t hass_app_setup_tile_num;

// declare you images or fonts you need
LV_IMG_DECLARE(hass_64px);

// declare callback functions
static void enter_hass_app_event_cb( lv_obj_t * obj, lv_event_t event );

// setup routine for example app
void hass_app_setup( void ) {

    hass_load_config();

    // register 2 vertical tiles and get the first tile number and save it for later use
    hass_app_main_tile_num = mainbar_add_app_tile( 1, 3, "Hoame Assistant App" );
    hass_app_setup_tile_num = hass_app_main_tile_num + 1;

    hass_app = app_register( "Hoame\nAssistant", &hass_64px, enter_hass_app_event_cb );

    if ( hass_config.widget ) {
        hass_add_widget();
    }

    hass_main_tile_setup( hass_app_main_tile_num );
    hass_setup_tile_setup( hass_app_setup_tile_num );
}

uint32_t hass_get_app_main_tile_num( void ) {
    return( hass_app_main_tile_num );
}

uint32_t hass_get_app_setup_tile_num( void ) {
    return( hass_app_setup_tile_num );
}

icon_t *hass_get_app_icon( void ) {
    return( hass_app );
}

icon_t *hass_get_widget_icon( void ) {
    return( hass_widget );
}

static void enter_hass_app_event_cb( lv_obj_t * obj, lv_event_t event ) {
    switch( event ) {
        case( LV_EVENT_CLICKED ):       statusbar_hide( true );
                                        mainbar_jump_to_tilenumber( hass_app_main_tile_num, LV_ANIM_OFF );
                                        break;
    }    
}


hass_config_t *hass_get_config( void ) {
    return( &hass_config );
}

void hass_load_config( void ) {
    fs::File file = SPIFFS.open( HASS_JSON_CONFIG_FILE, FILE_READ );
    if (!file) {
        log_e("Can't open file: %s!", HASS_JSON_CONFIG_FILE );
    }
    else {
        int filesize = file.size();
        SpiRamJsonDocument doc( filesize * 4 );

        DeserializationError error = deserializeJson( doc, file );
        if ( error ) {
            log_e("update check deserializeJson() failed: %s", error.c_str() );
        }
        else {
            strlcpy( hass_config.server, doc["hass"]["server"], sizeof( hass_config.server ) );
			hass_config.port = doc["hass"]["port"] | 1883;
            hass_config.ssl = doc["hass"]["ssl"] | false;
            strlcpy( hass_config.user, doc["hass"]["user"], sizeof( hass_config.user ) );
            strlcpy( hass_config.password, doc["hass"]["password"], sizeof( hass_config.password ) );
            strlcpy( hass_config.topic, doc["hass"]["topic"], sizeof( hass_config.topic ) );
            hass_config.autoconnect = doc["hass"]["autoconnect"] | false;
            hass_config.widget = doc["hass"]["widget"] | false;
        }        
        doc.clear();
    }
    file.close();
}

void hass_save_config( void ) {
    fs::File file = SPIFFS.open( HASS_JSON_CONFIG_FILE, FILE_WRITE );

    if (!file) {
        log_e("Can't open file: %s!", HASS_JSON_CONFIG_FILE );
    }
    else {
        SpiRamJsonDocument doc( 1000 );

        doc["hass"]["server"] = hass_config.server;
        doc["hass"]["port"] = hass_config.port;
        doc["hass"]["ssl"] = hass_config.ssl;
        doc["hass"]["user"] = hass_config.user;
        doc["hass"]["password"] = hass_config.password;
        doc["hass"]["topic"] = hass_config.topic;
        doc["hass"]["port"] = hass_config.port;
        doc["hass"]["autoconnect"] = hass_config.autoconnect;
        doc["hass"]["widget"] = hass_config.widget;

        if ( serializeJsonPretty( doc, file ) == 0) {
            log_e("Failed to write config file");
        }
        doc.clear();
    }
    file.close();
}


bool hass_add_widget( void ) {
    if ( hass_widget == NULL ) {
        hass_widget = widget_register( "n/a", &hass_64px, enter_hass_app_event_cb );
        widget_hide_indicator( hass_widget );
        if ( hass_widget != NULL ) {
            return( true );
        }
        else {
            return( false );
        }
    }
    return( true );
}

bool hass_remove_widget( void ) {
    hass_widget = widget_remove( hass_widget );
    return( true );
}
