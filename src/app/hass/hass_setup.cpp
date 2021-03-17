/****************************************************************************
 *   Tu May 22 21:23:51 2020
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
#include <config.h>

#include "hass_app.h"
#include "hass_setup.h"

#include "gui/mainbar/mainbar.h"
#include "gui/mainbar/main_tile/main_tile.h"
#include "gui/statusbar.h"
#include "gui/keyboard.h"

lv_obj_t *hass_setup_tile = NULL;
lv_obj_t *hass_setup_tile_2 = NULL;
lv_style_t hass_setup_style;
uint32_t hass_setup_tile_num;

lv_obj_t *hass_server_textfield = NULL;
lv_obj_t *hass_user_textfield = NULL;
lv_obj_t *hass_password_textfield = NULL;
lv_obj_t *hass_topic_textfield = NULL;
lv_obj_t *hass_port_textfield = NULL;
lv_obj_t *hass_autoconnect_onoff = NULL;
lv_obj_t *hass_widget_onoff = NULL;

LV_IMG_DECLARE(exit_32px);

static void hass_textarea_event_cb( lv_obj_t * obj, lv_event_t event );
static void hass_num_textarea_event_cb( lv_obj_t * obj, lv_event_t event );
static void exit_hass_widget_setup_event_cb( lv_obj_t * obj, lv_event_t event );
static void hass_autoconnect_onoff_event_handler( lv_obj_t * obj, lv_event_t event );
static void hass_widget_onoff_event_handler( lv_obj_t *obj, lv_event_t event );

void hass_setup_tile_setup( uint32_t tile_num ) {

    hass_config_t *hass_config = hass_get_config();

    hass_setup_tile_num = tile_num;
    hass_setup_tile = mainbar_get_tile_obj( hass_setup_tile_num );
    hass_setup_tile_2 = mainbar_get_tile_obj( hass_setup_tile_num + 1 );

    lv_style_copy( &hass_setup_style, mainbar_get_style() );
    lv_style_set_bg_color( &hass_setup_style, LV_OBJ_PART_MAIN, LV_COLOR_GRAY);
    lv_style_set_bg_opa( &hass_setup_style, LV_OBJ_PART_MAIN, LV_OPA_100);
    lv_style_set_border_width( &hass_setup_style, LV_OBJ_PART_MAIN, 0);
    lv_obj_add_style( hass_setup_tile, LV_OBJ_PART_MAIN, &hass_setup_style );
    lv_obj_add_style( hass_setup_tile_2, LV_OBJ_PART_MAIN, &hass_setup_style );

    lv_obj_t *exit_btn = lv_imgbtn_create( hass_setup_tile, NULL);
    lv_imgbtn_set_src( exit_btn, LV_BTN_STATE_RELEASED, &exit_32px);
    lv_imgbtn_set_src( exit_btn, LV_BTN_STATE_PRESSED, &exit_32px);
    lv_imgbtn_set_src( exit_btn, LV_BTN_STATE_CHECKED_RELEASED, &exit_32px);
    lv_imgbtn_set_src( exit_btn, LV_BTN_STATE_CHECKED_PRESSED, &exit_32px);
    lv_obj_add_style( exit_btn, LV_IMGBTN_PART_MAIN, &hass_setup_style );
    lv_obj_align( exit_btn, hass_setup_tile, LV_ALIGN_IN_TOP_LEFT, 10, 10 );
    lv_obj_set_event_cb( exit_btn, exit_hass_widget_setup_event_cb );
    
    lv_obj_t *exit_label = lv_label_create( hass_setup_tile, NULL);
    lv_obj_add_style( exit_label, LV_OBJ_PART_MAIN, &hass_setup_style  );
    lv_label_set_text( exit_label, "hass setup");
    lv_obj_align( exit_label, exit_btn, LV_ALIGN_OUT_RIGHT_MID, 5, 0 );

    lv_obj_t *hass_server_cont = lv_obj_create( hass_setup_tile, NULL );
    lv_obj_set_size( hass_server_cont, lv_disp_get_hor_res( NULL ) , 37);
    lv_obj_add_style( hass_server_cont, LV_OBJ_PART_MAIN, &hass_setup_style  );
    lv_obj_align( hass_server_cont, hass_setup_tile, LV_ALIGN_IN_TOP_MID, 0, 47 );
    lv_obj_t *hass_server_label = lv_label_create( hass_server_cont, NULL);
    lv_obj_add_style( hass_server_label, LV_OBJ_PART_MAIN, &hass_setup_style  );
    lv_label_set_text( hass_server_label, "server");
    lv_obj_align( hass_server_label, hass_server_cont, LV_ALIGN_IN_LEFT_MID, 5, 0 );
    hass_server_textfield = lv_textarea_create( hass_server_cont, NULL);
    lv_textarea_set_text( hass_server_textfield, hass_config->server );
    lv_textarea_set_pwd_mode( hass_server_textfield, false);
    lv_textarea_set_one_line( hass_server_textfield, true);
    lv_textarea_set_cursor_hidden( hass_server_textfield, true);
    lv_obj_set_width( hass_server_textfield, lv_disp_get_hor_res( NULL ) / 4 * 3  - 5 );
    lv_obj_align( hass_server_textfield, hass_server_cont, LV_ALIGN_IN_RIGHT_MID, -5, 0 );
    lv_obj_set_event_cb( hass_server_textfield, hass_textarea_event_cb );

    lv_obj_t *hass_port_cont = lv_obj_create( hass_setup_tile, NULL );
    lv_obj_set_size( hass_port_cont, lv_disp_get_hor_res( NULL ) , 37);
    lv_obj_add_style( hass_port_cont, LV_OBJ_PART_MAIN, &hass_setup_style  );
    lv_obj_align( hass_port_cont, hass_server_cont, LV_ALIGN_OUT_BOTTOM_MID, 0,  0 );
    lv_obj_t *hass_port_label = lv_label_create( hass_port_cont, NULL);
    lv_obj_add_style( hass_port_label, LV_OBJ_PART_MAIN, &hass_setup_style  );
    lv_label_set_text( hass_port_label, "port");
    lv_obj_align( hass_port_label, hass_port_cont, LV_ALIGN_IN_LEFT_MID, 5, 0 );
    hass_port_textfield = lv_textarea_create( hass_port_cont, NULL);
    char buf[10];
    sprintf(buf, "%d", hass_config->port );
    lv_textarea_set_text( hass_port_textfield, buf);
    lv_textarea_set_pwd_mode( hass_port_textfield, false);
    lv_textarea_set_one_line( hass_port_textfield, true);
    lv_textarea_set_cursor_hidden( hass_port_textfield, true);
    lv_obj_set_width( hass_port_textfield, lv_disp_get_hor_res( NULL ) / 4 * 3  - 5 );
    lv_obj_align( hass_port_textfield, hass_port_cont, LV_ALIGN_IN_RIGHT_MID, -5, 0 );
    lv_obj_set_event_cb( hass_port_textfield, hass_num_textarea_event_cb );

    lv_obj_t *hass_user_cont = lv_obj_create( hass_setup_tile, NULL );
    lv_obj_set_size( hass_user_cont, lv_disp_get_hor_res( NULL ) , 37);
    lv_obj_add_style( hass_user_cont, LV_OBJ_PART_MAIN, &hass_setup_style  );
    lv_obj_align( hass_user_cont, hass_port_cont, LV_ALIGN_OUT_BOTTOM_MID, 0, 0 );
    lv_obj_t *hass_user_label = lv_label_create( hass_user_cont, NULL);
    lv_obj_add_style( hass_user_label, LV_OBJ_PART_MAIN, &hass_setup_style  );
    lv_label_set_text( hass_user_label, "user");
    lv_obj_align( hass_user_label, hass_user_cont, LV_ALIGN_IN_LEFT_MID, 5, 0 );
    hass_user_textfield = lv_textarea_create( hass_user_cont, NULL);
    lv_textarea_set_text( hass_user_textfield, hass_config->user );
    lv_textarea_set_pwd_mode( hass_user_textfield, false);
    lv_textarea_set_one_line( hass_user_textfield, true);
    lv_textarea_set_cursor_hidden( hass_user_textfield, true);
    lv_obj_set_width( hass_user_textfield, lv_disp_get_hor_res( NULL ) / 4 * 3  - 5 );
    lv_obj_align( hass_user_textfield, hass_user_cont, LV_ALIGN_IN_RIGHT_MID, -5, 0 );
    lv_obj_set_event_cb( hass_user_textfield, hass_textarea_event_cb );

    lv_obj_t *hass_password_cont = lv_obj_create( hass_setup_tile, NULL );
    lv_obj_set_size( hass_password_cont, lv_disp_get_hor_res( NULL ) , 37);
    lv_obj_add_style( hass_password_cont, LV_OBJ_PART_MAIN, &hass_setup_style  );
    lv_obj_align( hass_password_cont, hass_user_cont, LV_ALIGN_OUT_BOTTOM_MID, 0, 0 );
    lv_obj_t *hass_password_label = lv_label_create( hass_password_cont, NULL);
    lv_obj_add_style( hass_password_label, LV_OBJ_PART_MAIN, &hass_setup_style  );
    lv_label_set_text( hass_password_label, "pass");
    lv_obj_align( hass_password_label, hass_password_cont, LV_ALIGN_IN_LEFT_MID, 5, 0 );
    hass_password_textfield = lv_textarea_create( hass_password_cont, NULL);
    lv_textarea_set_text( hass_password_textfield, hass_config->password );
    lv_textarea_set_pwd_mode( hass_password_textfield, false);
    lv_textarea_set_one_line( hass_password_textfield, true);
    lv_textarea_set_cursor_hidden( hass_password_textfield, true);
    lv_obj_set_width( hass_password_textfield, lv_disp_get_hor_res( NULL ) / 4 * 3  - 5 );
    lv_obj_align( hass_password_textfield, hass_password_cont, LV_ALIGN_IN_RIGHT_MID, -5, 0 );
    lv_obj_set_event_cb( hass_password_textfield, hass_textarea_event_cb );

    lv_obj_t *hass_topic_cont = lv_obj_create( hass_setup_tile, NULL );
    lv_obj_set_size( hass_topic_cont, lv_disp_get_hor_res( NULL ) , 37);
    lv_obj_add_style( hass_topic_cont, LV_OBJ_PART_MAIN, &hass_setup_style  );
    lv_obj_align( hass_topic_cont, hass_password_cont, LV_ALIGN_OUT_BOTTOM_MID, 0, 0 );
    lv_obj_t *hass_topic_label = lv_label_create( hass_topic_cont, NULL);
    lv_obj_add_style( hass_topic_label, LV_OBJ_PART_MAIN, &hass_setup_style  );
    lv_label_set_text( hass_topic_label, "topic");
    lv_obj_align( hass_topic_label, hass_topic_cont, LV_ALIGN_IN_LEFT_MID, 5, 0 );
    hass_topic_textfield = lv_textarea_create( hass_topic_cont, NULL);
    lv_textarea_set_text( hass_topic_textfield, hass_config->topic );
    lv_textarea_set_pwd_mode( hass_topic_textfield, false);
    lv_textarea_set_one_line( hass_topic_textfield, true);
    lv_textarea_set_cursor_hidden( hass_topic_textfield, true);
    lv_obj_set_width( hass_topic_textfield, lv_disp_get_hor_res( NULL ) / 4 * 3  - 5 );
    lv_obj_align( hass_topic_textfield, hass_topic_cont, LV_ALIGN_IN_RIGHT_MID, -5, 0 );
    lv_obj_set_event_cb( hass_topic_textfield, hass_textarea_event_cb );

    lv_tileview_add_element( hass_setup_tile, hass_server_cont );
    lv_tileview_add_element( hass_setup_tile, hass_port_cont );
    lv_tileview_add_element( hass_setup_tile, hass_user_cont );
    lv_tileview_add_element( hass_setup_tile, hass_password_cont );
    lv_tileview_add_element( hass_setup_tile, hass_topic_cont );

    lv_obj_t *hass_autoconnect_onoff_cont = lv_obj_create( hass_setup_tile_2, NULL);
    lv_obj_set_size( hass_autoconnect_onoff_cont, lv_disp_get_hor_res( NULL ), 32);
    lv_obj_add_style( hass_autoconnect_onoff_cont, LV_OBJ_PART_MAIN, &hass_setup_style );
    lv_obj_align( hass_autoconnect_onoff_cont, hass_setup_tile_2, LV_ALIGN_IN_TOP_MID, 0, 49 );
    hass_autoconnect_onoff = lv_switch_create( hass_autoconnect_onoff_cont, NULL);
    lv_obj_add_protect( hass_autoconnect_onoff, LV_PROTECT_CLICK_FOCUS);
    lv_obj_add_style( hass_autoconnect_onoff, LV_SWITCH_PART_INDIC, mainbar_get_switch_style() );
    lv_switch_off( hass_autoconnect_onoff, LV_ANIM_ON);
    lv_obj_align( hass_autoconnect_onoff, hass_autoconnect_onoff_cont, LV_ALIGN_IN_RIGHT_MID, -5, 0);
    lv_obj_set_event_cb( hass_autoconnect_onoff, hass_autoconnect_onoff_event_handler );
    lv_obj_t *hass_autoconnect_label = lv_label_create(hass_autoconnect_onoff_cont, NULL);
    lv_obj_add_style( hass_autoconnect_label, LV_OBJ_PART_MAIN, &hass_setup_style );
    lv_label_set_text( hass_autoconnect_label, "autoconnect");
    lv_obj_align( hass_autoconnect_label, hass_autoconnect_onoff_cont, LV_ALIGN_IN_LEFT_MID, 5, 0);

    lv_obj_t *hass_widget_onoff_cont = lv_obj_create( hass_setup_tile_2, NULL);
    lv_obj_set_size( hass_widget_onoff_cont, lv_disp_get_hor_res( NULL ), 32);
    lv_obj_add_style( hass_widget_onoff_cont, LV_OBJ_PART_MAIN, &hass_setup_style );
    lv_obj_align( hass_widget_onoff_cont, hass_autoconnect_onoff_cont, LV_ALIGN_OUT_BOTTOM_MID, 0, 0 );
    hass_widget_onoff = lv_switch_create( hass_widget_onoff_cont, NULL);
    lv_obj_add_protect( hass_widget_onoff, LV_PROTECT_CLICK_FOCUS);
    lv_obj_add_style( hass_widget_onoff, LV_SWITCH_PART_INDIC, mainbar_get_switch_style() );
    lv_switch_off( hass_widget_onoff, LV_ANIM_ON);
    lv_obj_align( hass_widget_onoff, hass_widget_onoff_cont, LV_ALIGN_IN_RIGHT_MID, -5, 0);
    lv_obj_set_event_cb( hass_widget_onoff, hass_widget_onoff_event_handler );
    lv_obj_t *hass_widget_onoff_label = lv_label_create( hass_widget_onoff_cont, NULL);
    lv_obj_add_style( hass_widget_onoff_label, LV_OBJ_PART_MAIN, &hass_setup_style );
    lv_label_set_text( hass_widget_onoff_label, "mainbar widget");
    lv_obj_align( hass_widget_onoff_label, hass_widget_onoff_cont, LV_ALIGN_IN_LEFT_MID, 5, 0);

    if ( hass_config->autoconnect )
        lv_switch_on( hass_autoconnect_onoff, LV_ANIM_OFF);
    else
        lv_switch_off( hass_autoconnect_onoff, LV_ANIM_OFF);

    if ( hass_config->widget )
        lv_switch_on( hass_widget_onoff, LV_ANIM_OFF );
    else
        lv_switch_off( hass_widget_onoff, LV_ANIM_OFF );
}

static void exit_hass_widget_setup_event_cb( lv_obj_t * obj, lv_event_t event ) {
    switch( event ) {
        case( LV_EVENT_CLICKED ):           keyboard_hide();
                                            hass_config_t *hass_config = hass_get_config();
                                            strlcpy( hass_config->server, lv_textarea_get_text( hass_server_textfield ), sizeof( hass_config->server ) );
                                            strlcpy( hass_config->user, lv_textarea_get_text( hass_user_textfield ), sizeof( hass_config->user ) );
                                            strlcpy( hass_config->password, lv_textarea_get_text( hass_password_textfield ), sizeof( hass_config->password ) );
                                            strlcpy( hass_config->topic, lv_textarea_get_text( hass_topic_textfield ), sizeof( hass_config->topic ) );
                                            hass_config->port = atoi(lv_textarea_get_text( hass_port_textfield ));
                                            hass_save_config();                                            
                                            mainbar_jump_to_tilenumber( hass_get_app_main_tile_num(), LV_ANIM_ON );
                                            break;
    }
}

static void hass_textarea_event_cb( lv_obj_t * obj, lv_event_t event ) {
    if( event == LV_EVENT_CLICKED ) {
        keyboard_set_textarea( obj );
    }
}

static void hass_num_textarea_event_cb( lv_obj_t * obj, lv_event_t event ) {
    if( event == LV_EVENT_CLICKED ) {
        num_keyboard_set_textarea( obj );
    }
}

static void hass_autoconnect_onoff_event_handler( lv_obj_t * obj, lv_event_t event ) {
    switch ( event ) {
        case (LV_EVENT_VALUE_CHANGED):      hass_config_t *hass_config = hass_get_config();
                                            hass_config->autoconnect = lv_switch_get_state( obj );
                                            break;
    }
}

static void hass_widget_onoff_event_handler(lv_obj_t *obj, lv_event_t event)
{
    switch ( event ) {
        case ( LV_EVENT_VALUE_CHANGED ):    hass_config_t *hass_config = hass_get_config();
                                            hass_config->widget = lv_switch_get_state( obj );
                                            if ( hass_config->widget ) {
                                                hass_add_widget();
                                            }
                                            else {
                                                hass_remove_widget();
                                            }
                                            break;
    }
}
