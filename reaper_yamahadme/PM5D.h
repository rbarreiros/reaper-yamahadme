/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * LS9.h
 * Copyright (C) 2011 Rui Barreiros <rbarreiros@gmail.com>
 * http://www.audioluz.net
 */

#ifndef __PM5D_H__
#define __PM5D_H__

#include "YamahaDME.h"

class PM5D : public YamahaDME
{
public:
	// Constructor
	PM5D(int inDev, int outDev, YamahaDME::SynchDirection dir, int *errStats) : YamahaDME(inDev, outDev, dir, errStats) { desk = YamahaDME::PM5D; }

	// Processing
	void onMidiEvent(MidiEvt *evt) {}

	// Conversions
	double getFaderYamahaToReaper(int vol) { return 0.0; }
	int getFaderReaperToYamaha(double vol) { return 0; }

	// Synch
	void synchToReaper() {}
};

#endif