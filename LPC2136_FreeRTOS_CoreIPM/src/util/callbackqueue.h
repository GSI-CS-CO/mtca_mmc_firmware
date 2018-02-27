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
 * callbackqueue.h
 *
 *  Created on: Aug 8, 2014
 *      Author: tom
 */

#ifndef CALLBACKQUEUE_H_
#define CALLBACKQUEUE_H_

#define CALLBACK_QUEUE_LENGTH 16


struct callback_queue_entry{
    void (*callback)(void*); //Callback function
    void* pvt;
};

typedef struct callback_queue{
    struct callback_queue_entry queue[32];
    unsigned char write_idx;
    unsigned char read_idx;
    unsigned char len;
}CB_QUEUE;

void cb_init(CB_QUEUE* cb);
void cb_add_callback(CB_QUEUE* cb, void (*callback)(void*),void* pvt);
void cb_process(CB_QUEUE* cb);

#endif /* CALLBACKQUEUE_H_ */
