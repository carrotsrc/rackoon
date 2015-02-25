/*
 * Copyright (C) 2015  Charlie Fyvie-Gauld
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef RACKTELEMETRY_H
#define RACKTELEMETRY_H
#include "common.h"
#include "framework/rack/Rack.h"


namespace RackoonIO {

namespace Telemetry {


class RackTelemetry
{
public:
	RackTelemetry(Rack*);
	void metricUnitCycle();
protected:
	Rack *rack;

	std::chrono::microseconds peakDeltaUnitCycle, lowDeltaUnitCycle, avgDeltaUnitCycle;


	void onUnitCycleStart(void);
	void onUnitCycleEnd(void);
};

} // Telemetry

} // RackoonIO

#endif // RACKTELEMETRY_H