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
 * project_defs.h
 *
 *  Created on: 12. jun. 2017
 *      Author: skirn
 */
/*
 * Global project definitions
 *  specify whether project will be compiled for DENX proto board
 *  or Cosylab own MMC board AMC_FTRN
 *
 *  for DENX proto board:
 *  #define DENX_PROTO_BOARD
 *
 *  for our MMC board:
 *  #define AMC_FTRN_BOARD
 *
 *  Only one define can be active at once, DENX_PROTO_BOARD or AMC_FTRN_BOARD
 *
 */
#ifndef PROJECT_DEFS_H_
#define PROJECT_DEFS_H_

/* place for board definition */
#define AMC_FTRN_BOARD

#endif /* PROJECT_DEFS_H_ */
