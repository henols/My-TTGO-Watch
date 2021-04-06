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
#include "config.h"
#include <stdio.h>
#include <time.h>
#include <TTGO.h>
#include <soc/rtc.h>

#include "bma.h"
#include "powermgm.h"
#include "callback.h"

#include "gui/statusbar.h"

volatile bool DRAM_ATTR bma_irq_flag = false;
portMUX_TYPE DRAM_ATTR BMA_IRQ_Mux = portMUX_INITIALIZER_UNLOCKED;

/**
 * move internal stepcounter into noninit ram section
 */
__NOINIT_ATTR uint32_t stepcounter_valid;           /** @brief stepcount valid mask, if 0xa5a5a5a5 when stepcounter is valid after reset */
__NOINIT_ATTR uint32_t stepcounter_before_reset;    /** @brief stepcounter before reset */
__NOINIT_ATTR uint32_t stepcounter;                 /** @brief stepcounter */

static struct tm bma_old_date;

bma_config_t bma_config;
callback_t *bma_callback = NULL;

bool first_loop_run = true;

void IRAM_ATTR bma_irq( void );
bool bma_send_event_cb( EventBits_t event, void *arg );
bool bma_powermgm_event_cb( EventBits_t event, void *arg );
bool bma_powermgm_loop_cb( EventBits_t event, void *arg );

static void bma_notify_stepcounter();

void bma_setup( void ) {
    TTGOClass *ttgo = TTGOClass::getWatch();
    /*
     * check if stepcounter valid and reset if not valid
     */
    if ( stepcounter_valid != 0xa5a5a5a5 ) {
      stepcounter = 0;
      stepcounter_before_reset = 0;
      stepcounter_valid = 0xa5a5a5a5;
      log_i("stepcounter not valid. reset");
    }
    stepcounter = stepcounter + stepcounter_before_reset;
    /*
     * load config from json
     */
    bma_config.load();
    /*
     * init stepcounter
     */
    ttgo->bma->begin();
    ttgo->bma->attachInterrupt();
    ttgo->bma->direction();
    /*
     * init stepcounter interrupt function
     */
    pinMode( BMA423_INT1, INPUT );
    attachInterrupt( BMA423_INT1, bma_irq, RISING );
    /*
     * load config setting for tilt, stepcounter and wakeup to enabled interrupts
     */
    bma_reload_settings();
    /*
     * register powermgm callback funtions
     */
    powermgm_register_cb( POWERMGM_SILENCE_WAKEUP | POWERMGM_STANDBY | POWERMGM_WAKEUP | POWERMGM_ENABLE_INTERRUPTS | POWERMGM_DISABLE_INTERRUPTS , bma_powermgm_event_cb, "powermgm bma" );
    powermgm_register_loop_cb( POWERMGM_SILENCE_WAKEUP | POWERMGM_STANDBY | POWERMGM_WAKEUP, bma_powermgm_loop_cb, "powermgm bma loop" );
}

bool bma_powermgm_event_cb( EventBits_t event, void *arg ) {
    switch( event ) {
        case POWERMGM_STANDBY:          bma_standby();
                                        break;
        case POWERMGM_WAKEUP:           bma_wakeup();
                                        break;
        case POWERMGM_SILENCE_WAKEUP:   bma_wakeup();
                                        break;
        case POWERMGM_ENABLE_INTERRUPTS:
                                        attachInterrupt( BMA423_INT1, bma_irq, RISING );
                                        break;
        case POWERMGM_DISABLE_INTERRUPTS:
                                        detachInterrupt( BMA423_INT1 );
                                        break;
    }
    return( true );
}

bool bma_powermgm_loop_cb( EventBits_t event , void *arg ) {
    static bool BMA_tilt = false;
    static bool BMA_doubleclick = false;
    static bool BMA_stepcounter = false;

    TTGOClass *ttgo = TTGOClass::getWatch();
    /*
     * handle IRQ event
     */
    portENTER_CRITICAL(&BMA_IRQ_Mux);
    bool temp_bma_irq_flag = bma_irq_flag;
    bma_irq_flag = false;
    portEXIT_CRITICAL(&BMA_IRQ_Mux);

    /*
     * check for the event
     */
    if ( temp_bma_irq_flag ) {                
        while( !ttgo->bma->readInterrupt() );
        /*
         * set powermgm wakeup event and save BMA_* event
         */
        if ( ttgo->bma->isDoubleClick() ) {
            powermgm_set_event( POWERMGM_WAKEUP_REQUEST );
            BMA_doubleclick = true;
        }
        if ( ttgo->bma->isTilt() ) {
            powermgm_set_event( POWERMGM_WAKEUP_REQUEST );
            BMA_tilt = true;
        }
        if ( ttgo->bma->isStepCounter() ) {
            BMA_stepcounter = true;
        }
    }

    switch( event ) {
        case POWERMGM_WAKEUP:   {
            /*
             * check if an BMA_* event triggered
             * one event per loop
             */
            if ( BMA_doubleclick ) {
                BMA_doubleclick = false;
                bma_send_event_cb( BMACTL_DOUBLECLICK, NULL );
            }
            else if ( BMA_tilt ) {
                BMA_tilt = false;
                bma_send_event_cb( BMACTL_TILT, NULL );
            }
            else if ( BMA_stepcounter ) {
                BMA_stepcounter = false;
                bma_notify_stepcounter();
            }
            break;
        }
    }

    /*
     *  force update statusbar after restart/boot
     */
    if ( first_loop_run ) {
        first_loop_run = false;
        bma_notify_stepcounter();
    }
    return( true );
}

static void bma_notify_stepcounter() {
    static uint32_t last_val = 0;
    TTGOClass *ttgo = TTGOClass::getWatch();
    stepcounter_before_reset = ttgo->bma->getCounter();

    uint32_t delta = stepcounter + stepcounter_before_reset - last_val;
    if (delta > 0) {
        // New val
        last_val = stepcounter + stepcounter_before_reset;
        bma_send_event_cb( BMACTL_STEPCOUNTER, &last_val );
    }
}

void bma_standby( void ) {
    TTGOClass *ttgo = TTGOClass::getWatch();
    time_t now;

    log_i("go standby");

    if ( bma_get_config( BMA_STEPCOUNTER ) )
        ttgo->bma->enableStepCountInterrupt( false );

    time( &now );
    localtime_r( &now, &bma_old_date );

    gpio_wakeup_enable ( (gpio_num_t)BMA423_INT1, GPIO_INTR_HIGH_LEVEL );
    esp_sleep_enable_gpio_wakeup ();
}

void bma_wakeup( void ) {
    TTGOClass *ttgo = TTGOClass::getWatch();

    log_i("go wakeup");

    if ( bma_get_config( BMA_STEPCOUNTER ) )
        ttgo->bma->enableStepCountInterrupt( true );

    /*
     * check for a new day and reset stepcounter if configure
     */
    if ( bma_get_config( BMA_DAILY_STEPCOUNTER ) ) {
        time_t now;
        tm info;
        time( &now );
        localtime_r( &now, &info );
        if ( info.tm_yday != bma_old_date.tm_yday ) {
            log_i("reset setcounter: %d != %d", info.tm_yday, bma_old_date.tm_yday );
            ttgo->bma->resetStepCounter();
            localtime_r( &now, &bma_old_date );
        }
    }

    /*
     * force bma_powermgm_loop_cb update
     */
    first_loop_run = true;
}

void bma_reload_settings( void ) {

    TTGOClass *ttgo = TTGOClass::getWatch();

    ttgo->bma->enableStepCountInterrupt( bma_config.enable[ BMA_STEPCOUNTER ] );
    ttgo->bma->enableWakeupInterrupt( bma_config.enable[ BMA_DOUBLECLICK ] );
    ttgo->bma->enableTiltInterrupt( bma_config.enable[ BMA_TILT ] );
}

void IRAM_ATTR bma_irq( void ) {
    portENTER_CRITICAL_ISR(&BMA_IRQ_Mux);
    bma_irq_flag = true;
    portEXIT_CRITICAL_ISR(&BMA_IRQ_Mux);
}

bool bma_register_cb( EventBits_t event, CALLBACK_FUNC callback_func, const char *id ) {
    if ( bma_callback == NULL ) {
        bma_callback = callback_init( "bma" );
        if ( bma_callback == NULL ) {
            log_e("bma_callback alloc failed");
            while(true);
        }
    }
    return( callback_register( bma_callback, event, callback_func, id ) );
}

bool bma_send_event_cb( EventBits_t event, void *arg ) {
    return( callback_send( bma_callback, event, arg ) );
}

void bma_save_config( void ) {
    bma_config.save();
}

void bma_read_config( void ) {
    bma_config.load();
}

bool bma_get_config( int config ) {
    return bma_config.get_config(config);
}

void bma_set_config( int config, bool enable ) {
    bma_config.bma_set_config( config, enable);
    bma_config.save();
    bma_reload_settings();
}

void bma_set_rotate_tilt( uint32_t rotation ) {
    struct bma423_axes_remap remap_data;

    TTGOClass *ttgo = TTGOClass::getWatch();

    switch( rotation / 90 ) {
        case 0:     remap_data.x_axis = 0;
                    remap_data.x_axis_sign = 1;
                    remap_data.y_axis = 1;
                    remap_data.y_axis_sign = 1;
                    remap_data.z_axis  = 2;
                    remap_data.z_axis_sign  = 1;
                    ttgo->bma->set_remap_axes(&remap_data);
                    break;
        case 1:     remap_data.x_axis = 1;
                    remap_data.x_axis_sign = 1;
                    remap_data.y_axis = 0;
                    remap_data.y_axis_sign = 0;
                    remap_data.z_axis  = 2;
                    remap_data.z_axis_sign  = 1;
                    ttgo->bma->set_remap_axes(&remap_data);
                    break;
        case 2:     remap_data.x_axis = 0;
                    remap_data.x_axis_sign = 1;
                    remap_data.y_axis = 1;
                    remap_data.y_axis_sign = 0;
                    remap_data.z_axis  = 2;
                    remap_data.z_axis_sign  = 1;
                    ttgo->bma->set_remap_axes(&remap_data);
                    break;
        case 3:     remap_data.x_axis = 1;
                    remap_data.x_axis_sign = 1;
                    remap_data.y_axis = 0;
                    remap_data.y_axis_sign = 1;
                    remap_data.z_axis  = 2;
                    remap_data.z_axis_sign  = 1;
                    ttgo->bma->set_remap_axes(&remap_data);
                    break;
    }
}

uint32_t bma_get_stepcounter( void ) {
    return stepcounter + stepcounter_before_reset;
}