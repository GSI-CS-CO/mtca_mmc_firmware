/*
 *   openMMC -- Open Source modular IPM Controller firmware
 *
 *   Copyright (C) 2015  Julian Mendez  <julian.mendez@cern.ch>
 *   Copyright (C) 2015-2016  Henrique Silva <henrique.silva@lnls.br>
 *
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


/*********************************************
 * Common defines
 *********************************************/
#define LANG_CODE               0
#define FRU_FILE_ID             "FTRNFRU"       //Allows knowing the source of the FRU present in the memory

#define BOARD_INFO_AREA_ENABLE
#define PRODUCT_INFO_AREA_ENABLE
#define MULTIRECORD_AREA_ENABLE

/*********************************************
 * Board information area
 *********************************************/
#define BOARD_MANUFACTURER      "Cosylab"
#define BOARD_NAME              "FTRN_AMC"
#define BOARD_SN                "000000001"
#define BOARD_PN                "FTRN"

/*********************************************
 * Product information area
 *********************************************/
#define PRODUCT_MANUFACTURER    "Cosylab"
#define PRODUCT_NAME            "FTRN AMC"
#define PRODUCT_PN              "00001"
#define PRODUCT_VERSION         "v3"
#define PRODUCT_SN              "000001"
#define PRODUCT_TAG             ""

//#define PRODUCT_CUSTOM_DATA_0 "MMC FW 0.8 TEST"

