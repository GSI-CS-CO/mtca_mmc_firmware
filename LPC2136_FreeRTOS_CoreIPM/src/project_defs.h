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


#ifndef PROJECT_DEFS_H_
#define PROJECT_DEFS_H_


// If LIBERA_HS_EVENT_HACK defined (uncommented):
// Build is meant for AMC also to be used in i-Tech Libera with GSI/FAIR OS/SW
//
// With this modification the following is changed:
// 1. On module init, Hot-Swap handle (HSH) state is not checked and
//    HSH event is not sent out and periodical checking of the HSH is not started
//    This way FTRN MMC does not send HSH event after boot or anytime later
//    when it is in Libera.
//
// 2. If SET_EVENT_RECEIVER request is received from MCH in MTCA crate 
//     with managed power supply, only then we know that we are not in Libera and
//     therefore HSH state is checked, HSH event is sent to MCH and 
//     periodical checking of the HSH is started
//
// 3. IMPORTANT: if build is with this modification and AMC is used in the crate
//    without managed power supply then AMC is not activated because 
//    MCH is not aware of the AMCs presence!!!
// 
// If not used (not defined) use "//" to comment out!
#define LIBERA_HS_EVENT_HACK

/* place for board definition */
#define AMC_FTRN_BOARD


#endif /* PROJECT_DEFS_H_ */
