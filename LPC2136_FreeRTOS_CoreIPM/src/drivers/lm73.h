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
 * lm73.h
 *
 *  Created on: Aug 8, 2014
 *      Author: tom
 */

#ifndef LM73_H_
#define LM73_H_

struct lm73_ws {
	int last_reading; //Last temperature reading in millidegrees
	unsigned char address; //address
	unsigned char buffer[4]; //I2C buffer
	unsigned int read_complete; //is set to 0 when reading starts and set to 1 once it completes.
};


void lm73_init(struct lm73_ws* ws);
void lm73_readTemp(struct lm73_ws* ws);



#endif /* LM73_H_ */

