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

#ifndef _ringbuffer_h
#define _ringbuffer_h

#define ringBuffer_typedef(T, NAME) \
  typedef struct { \
    int size; \
    int start; \
    int end; \
    T* elems; \
  } NAME

#define ringBuffer_typedefFixedSize(T, NAME, SIZE) \
  typedef struct { \
    int size; \
    int start; \
    int end; \
    T elems[SIZE]; \
  } NAME

#define bufferInit(BUF, S, T) \
  BUF.size = S+1; \
  BUF.start = 0; \
  BUF.end = 0; \
  BUF.elems = (T*)calloc(BUF.size, sizeof(T))


#define bufferDestroy(BUF) free(BUF->elems)
#define nextStartIndex(BUF) ((BUF->start + 1) % BUF->size)
#define nextEndIndex(BUF) ((BUF->end + 1) % BUF->size)
#define isBufferEmpty(BUF) (BUF->end == BUF->start)
#define isBufferFull(BUF) (nextEndIndex(BUF) == BUF->start)

#define bufferWrite(BUF, ELEM) \
  BUF->elems[BUF->end] = ELEM; \
  BUF->end = (BUF->end + 1) % BUF->size; \
  if (isBufferEmpty(BUF)) { \
    BUF->start = nextStartIndex(BUF); \
  }

#define bufferRead(BUF, ELEM) \
    ELEM = BUF->elems[BUF->start]; \
    BUF->start = nextStartIndex(BUF);

#endif

