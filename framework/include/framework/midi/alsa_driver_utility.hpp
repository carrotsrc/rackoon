#ifndef __ALSADRIVERINTERFACE_HPP_1440957281__
#define __ALSADRIVERINTERFACE_HPP_1440957281__
#include <memory>

#include "framework/fwcommon.hpp"
#include "framework/midi/midi.hpp"
#include "framework/midi/input_handle.hpp"
#include "framework/midi/output_handle.hpp"

namespace strangeio {
namespace midi {

class driver_utility {
public:
	driver_utility();
	midi_in_uptr open_input_port(std::string dev, std::string name);
	void close_input_port(midi_in_uptr);
	
	midi_out_uptr open_output_port(std::string dev, std::string name);
	void close_output_port(midi_out_uptr);
private:
};

} // Midi
} // StrangeIO
#endif