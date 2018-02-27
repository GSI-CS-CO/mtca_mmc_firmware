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
 * m24eeprom.h
 *
 *  Created on: Aug 19, 2014
 *      Author: tom
 */

#ifndef M24EEPROM_H_
#define M24EEPROM_H_

struct m24eeprom_ws{
	unsigned char address; //I2C address of the device
	unsigned char buffer[32];
};


#endif /* M24EEPROM_H_ */
