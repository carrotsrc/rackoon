/* Copyright 2015 Charlie Fyvie-Gauld
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published 
 *  by the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef NORMALISEDSIZEBUFFER_H
#define NORMALISEDSIZEBUFFER_H
#include "common.h"

namespace RackoonIO {

namespace Buffers {

template<typename T>
class NormalisedSizeBuffer {
public:
	enum State {
		DISPATCH,
		PARTIAL,
		OVERFLOW
	};
	NormalisedSizeBuffer(int, int);
	State supply(T*, int);
	T *flush();

	void reset() { state = PARTIAL; };
	State getState();

private:
	int load;
	int nSize;

	T *buffer, *remainder, *rRead, *rWrite, *bound;
	State state;
};

/*
 * Template definitions
 */

template<typename T>
NormalisedSizeBuffer<T>::NormalisedSizeBuffer ( int normal, int overflow ) {
	nSize = normal;
	buffer = (T*) malloc(sizeof(T)*nSize);
	remainder = (T*) malloc(sizeof(T)*nSize+overflow);
	bound = remainder + (nSize+overflow);
	rWrite = rRead = remainder;
	load = 0;
	state = PARTIAL;
}

template<typename T>
typename NormalisedSizeBuffer<T>::State
NormalisedSizeBuffer<T>::supply (T* period, int pSize) {
	if(rWrite < rRead && (rWrite + pSize) > rRead)
		return OVERFLOW;

	int delta = pSize;
	T *pRead = period;

	if(rWrite + pSize >= bound) {
		delta = bound - rWrite;
		memcpy(rWrite, pRead, delta * sizeof(T)); 
		rWrite = remainder;
		pRead += delta;
		delta = pSize-delta;

	}

	memcpy(rWrite, pRead, delta*sizeof(T));
	rWrite += delta;

	load += pSize;

	state = (load >= nSize) ? DISPATCH : PARTIAL;
	return state;
}

template<typename T>
T *NormalisedSizeBuffer<T>::flush () {
	if(state == PARTIAL || load < nSize)
		return nullptr;

	int delta = nSize;
	T* bWrite = buffer;

	if(rRead + nSize >= bound) {
		delta = bound - rRead;
		memcpy(bWrite, rRead, delta * sizeof(T));
		rRead = remainder;
		bWrite += delta;
		delta = nSize - delta;
	}

	memcpy(bWrite, rRead, delta * sizeof(T));
	rRead += delta;
	load -= nSize;
	state = (load >= nSize) ? DISPATCH : PARTIAL;

	return buffer;
}

template<typename T>
typename NormalisedSizeBuffer<T>::State
NormalisedSizeBuffer<T>::getState () {
	return state;
}


} // Buffers
} // RackoonIO

#endif
