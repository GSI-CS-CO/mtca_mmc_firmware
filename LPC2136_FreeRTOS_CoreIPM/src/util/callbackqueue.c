/*
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *   @license GPL-3.0+ <http://spdx.org/licenses/GPL-3.0+>
 */

/*
 * callbackqueue.c
 *
 *  Created on: Aug 8, 2014
 *      Author: tom
 */

#include <stdio.h>
#include "callbackqueue.h"

/**
 * @brief cb_init
 *intialize callback queue
 */
void cb_init(CB_QUEUE* cb){
    cb->write_idx=0;
    cb->read_idx=0;
    cb->len=0;
}

/**
 * @brief cb_add_callback add new callback to queue
 * @param pvt
 */
void cb_add_callback(CB_QUEUE* cb, void (*callback)(void*),void* pvt){

	cb->queue[cb->write_idx%CALLBACK_QUEUE_LENGTH].callback=callback;
    cb->queue[cb->write_idx%CALLBACK_QUEUE_LENGTH].pvt=pvt;

    cb->write_idx++;
    cb->len++;

    if(cb->len > CALLBACK_QUEUE_LENGTH){
        printf("ERR: Callback overflow!\n");
    }
}

/**
 * @brief cb_process
 *  Processes (invokes) callbacks currently in queue, note that new callbacks are not processed.
 */
void cb_process(CB_QUEUE* cb){
    /*
     *Store current write idx since new callbacks may (will) be added
     *during processing of current callbacks
     */
    int write_idx = cb->write_idx;
    unsigned char len = cb->len;

    while(len){
    	cb->queue[cb->read_idx%CALLBACK_QUEUE_LENGTH].callback(cb->queue[cb->read_idx%CALLBACK_QUEUE_LENGTH].pvt);
        cb->read_idx++;
        cb->len--;
        len--;
    }
}
