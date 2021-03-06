#include "catch.hpp"
#include "suite.hpp"

#if !DEVBUILD
	#error The testing suite requires DEVBUILD to be enabled
#endif
#define MidiControllerId 50
using namespace strangeio;

#include "framework/routine/sound.hpp"
TEST_CASE( "Interleaving routines of two channels", "[strangeio::routine],[strangeio::routine::sound]" ) {
	PcmSample interleaved[12] = {
		0.50f, 0.10f,
		0.51f, 0.11f,
		0.52f, 0.12f,
		0.53f, 0.13f,
		0.54f, 0.14f,
		0.55f, 0.15f
		};

	PcmSample out[12] = {0};
	routine::sound::deinterleave2(interleaved, out, 6);

	SECTION( "Deinterleave a two channel period of samples" ) {

		REQUIRE( out[0] == 0.50f );
		REQUIRE( out[5] == 0.55f );
		REQUIRE( out[6] == 0.10f );
		REQUIRE( out[11] == 0.15f );
	}

	PcmSample reinter[12] = {0};
	routine::sound::interleave2(out, reinter, 6);

	SECTION( "Interleave a two channel period of samples" ) {

		REQUIRE( reinter[0] == 0.50f );
		REQUIRE( reinter[1] == 0.10f );

		REQUIRE( reinter[10] == 0.55f );
		REQUIRE( reinter[11] == 0.15f );
	}
}

#include "framework/routine/midi.hpp"

TEST_CASE( "Convert velocity to a normalised range", "[strangeio::routine],[strangeio::routine::midi]" ) {

	
	SECTION( "Normalise to a range of -1.0 to +1.0" ) {
		REQUIRE( routine::midi::normalise_velocity64(127) == 1.0f );
		REQUIRE( routine::midi::normalise_velocity64(64) == 0.0f );
		REQUIRE( routine::midi::normalise_velocity64(0) == -1.0f );
	}

	SECTION( "Normalise to a range of 0.0 to 1.0" ) {
		REQUIRE( routine::midi::normalise_velocity128(127) == 1.0f );
		REQUIRE( routine::midi::normalise_velocity128(64) == 0.5f );
		REQUIRE( routine::midi::normalise_velocity128(0) == 0.0f );
	}
}

#include "framework/component/rack.hpp"
#include "framework/memory/cache_manager.hpp"
using namespace strangeio::memory;

TEST_CASE("CacheManager", "[strangeio::memory]") {
	
	cache_manager cache(32);

	SECTION("Verify Initial State") {
		REQUIRE(cache.num_blocks() == 32);
	}

	SECTION("Verify uninitialised allocation") {
		REQUIRE(cache.alloc_raw(1) == nullptr);
	}

	SECTION("Verify cache sizes") {
		cache.build_cache(512);
		REQUIRE(cache.block_size() == 512);
		REQUIRE(cache.cache_size() == 512*32);
		auto handles = cache.get_const_handles();
		REQUIRE(handles.size() == 32);
		REQUIRE(handles[0].num_blocks == 32);
		REQUIRE(handles[handles.size()-1].num_blocks == 1);
	}

	SECTION("Verify single allocation") {
		cache.build_cache(512);
		auto ptr = cache.alloc_raw(1);
		REQUIRE(ptr != nullptr);
		auto& handles = cache.get_const_handles();
		REQUIRE(handles[0].ptr == ptr);
		REQUIRE(handles[0].in_use == true);
		REQUIRE(handles[0].num_blocks == 1);
	}

	SECTION("Verify multiple 1 block allocations") {
		cache.build_cache(512);
		auto& handles = cache.get_const_handles();
		auto ptr = cache.alloc_raw(1);

		REQUIRE(ptr != nullptr);

		REQUIRE(handles[0].ptr == ptr);
		REQUIRE(handles[0].in_use == true);
		REQUIRE(handles[0].num_blocks == 1);
		
		auto ptr2 = cache.alloc_raw(1);
		REQUIRE(ptr2 != nullptr);
		REQUIRE(handles[1].ptr == ptr2);
		REQUIRE(handles[1].in_use == true);
		REQUIRE(handles[1].num_blocks == 1);
	}

	SECTION("Verify single multi-block allocation") {
		cache.build_cache(512);
		auto& handles = cache.get_const_handles();
		auto ptr = cache.alloc_raw(5);
		
		REQUIRE(ptr != nullptr);
		REQUIRE(handles[0].ptr == ptr);
		REQUIRE(handles[0].in_use == true);
		REQUIRE(handles[0].num_blocks == 5);

		REQUIRE(handles[4].in_use == true);
		REQUIRE(handles[4].num_blocks == 1);

		REQUIRE(handles[5].in_use == false);
	}

	SECTION("Verify multiple multi-block allocations") {
		cache.build_cache(512);
		auto& handles = cache.get_const_handles();
		auto ptr = cache.alloc_raw(5);
		auto ptr2 = cache.alloc_raw(3);

		REQUIRE(handles[5].ptr == ptr2);
		REQUIRE(handles[5].in_use == true);
		REQUIRE(handles[5].num_blocks == 3);
		REQUIRE(handles[7].num_blocks == 1);
		REQUIRE(handles[8].in_use == false);
	}

	SECTION("Verify single block free") {
		cache.build_cache(512);
		auto& handles = cache.get_const_handles();
		auto ptr = cache.alloc_raw(1);
		cache.free_raw(ptr);

		REQUIRE(handles[0].ptr == ptr);
		REQUIRE(handles[0].in_use == false);
		REQUIRE(handles[0].num_blocks == 32);
	}

	SECTION("Verify multiple single block free") {
		cache.build_cache(512);
		auto& handles = cache.get_const_handles();
		auto ptr = cache.alloc_raw(1);
		auto ptr2 = cache.alloc_raw(1);
		cache.free_raw(ptr);

		REQUIRE(handles[0].ptr == ptr);
		REQUIRE(handles[0].in_use == false);
		REQUIRE(handles[0].num_blocks == 1);

		REQUIRE(handles[1].ptr == ptr2);
		REQUIRE(handles[1].in_use == true);
		REQUIRE(handles[1].num_blocks == 1);

		cache.free_raw(ptr2);
		REQUIRE(handles[1].ptr == ptr2);
		REQUIRE(handles[1].in_use == false);

		// Check downward relink
		REQUIRE(handles[1].num_blocks == 31);

		// Check upward relink
		REQUIRE(handles[0].num_blocks == 32);
	}
	
	SECTION("Verify single multi-block free") {
		cache.build_cache(512);
		auto& handles = cache.get_const_handles();
		auto ptr = cache.alloc_raw(5);
		cache.free_raw(ptr);

		REQUIRE(handles[0].ptr == ptr);
		REQUIRE(handles[0].num_blocks == 32);
		REQUIRE(handles[4].in_use == false);
		REQUIRE(handles[4].num_blocks == 28);
	}

	SECTION("Verify multiple multi-block free") {
		cache.build_cache(512);
		auto& handles = cache.get_const_handles();
		auto ptr = cache.alloc_raw(5);
		auto ptr2 = cache.alloc_raw(3);
		cache.free_raw(ptr);

		REQUIRE(handles[0].num_blocks == 5);
		REQUIRE(handles[0].in_use == false);
		
		cache.free_raw(ptr2);
		REQUIRE(handles[5].ptr == ptr2);

		// Check upward relink
		REQUIRE(handles[5].num_blocks == 27);
		REQUIRE(handles[5].in_use == false);

		// Check downward relink
		REQUIRE(handles[0].num_blocks == 32);
	}
}

#include "framework/memory/cache_ptr.hpp"

TEST_CASE("cache_ptr", "[strangeio::memory]") {

	cache_manager cache(32);
	cache.build_cache(512);
	auto& handles = cache.get_const_handles();

	SECTION("Test creation and destruction") {
		auto cptr = new cache_ptr(cache.alloc_raw(3), 3, &cache);
		REQUIRE(cptr->get() == handles[0].ptr);
		REQUIRE(cptr->block_size() == 512);
		REQUIRE(cptr->num_blocks() == 3);
		REQUIRE(handles[0].in_use == true);

		delete cptr;
		REQUIRE(handles[0].in_use == false);
	}
	
	SECTION("Test empty state") {
		auto cptr = cache_ptr();
		REQUIRE(cptr.get() == nullptr);
		REQUIRE(cptr.num_blocks() == 0);
	}

	SECTION("Test boolean operator overload") {
		auto cptr = cache_ptr();
		REQUIRE(cptr.get() == nullptr);
		REQUIRE(cptr == false);
		auto cptr2 = cache_ptr(cache.alloc_raw(1), 1, &cache);
		REQUIRE(cptr2.get() != nullptr);
		REQUIRE(cptr2 ==  true);
	}

	SECTION("Test dereference") {
		cache_ptr cptr(cache.alloc_raw(3), 3, &cache);
		REQUIRE(*cptr == handles[0].ptr);
	}

	SECTION("Test array access") {
		cache_ptr cptr(cache.alloc_raw(3), 3, &cache);
		cptr[6] = 123.321f;
		REQUIRE(cptr[6] == 123.321f);
	}

	SECTION("Test release") {
		auto cptr = new cache_ptr(cache.alloc_raw(3), 3, &cache);
		auto ptr = cptr->release();
		REQUIRE(cptr->num_blocks() == 0);
		REQUIRE(cptr->get() == nullptr);
		REQUIRE(handles[0].in_use == true);
		delete cptr;
		cache.free_raw(ptr);
	}

	SECTION("Test swap and release") {
		auto cptr = new cache_ptr(cache.alloc_raw(3), 3, &cache);
		auto cptr2 = new cache_ptr(cache.alloc_raw(2), 1, &cache);
		
		REQUIRE(cptr2->get() == handles[3].ptr);
		cptr->swap(*(cptr2));
		REQUIRE(cptr2->get() == handles[0].ptr);
		REQUIRE(cptr->get() == handles[3].ptr);
		
		auto ptr = cptr->release();
		REQUIRE(cptr->num_blocks() == 0);
		REQUIRE(cptr->get() == nullptr);
		REQUIRE(handles[3].in_use == true);
		
		delete cptr2;
		delete cptr;
		cache.free_raw(ptr);
	}
	
	SECTION("Test reset") {
		auto cptr = new cache_ptr(cache.alloc_raw(3), 3, &cache);
		cptr->reset(cache.alloc_raw(1), 1);

		REQUIRE(handles[0].in_use == false);
		REQUIRE(handles[3].in_use == true);
		REQUIRE(cptr->get() == handles[3].ptr);
		delete cptr;
	}
	
	SECTION("Verify scope deletion") {
		{
			cache_ptr cptr(cache.alloc_raw(3), 3, &cache);
			REQUIRE(handles[0].in_use == true);
		}
		REQUIRE(handles[0].in_use == false);
	}

	SECTION("Verify copy constructor") {
		cache_ptr cptr(cache.alloc_raw(2), 2, &cache);
		cache_ptr cpy = cptr;
		REQUIRE(cpy.get() == handles[0].ptr);
		REQUIRE(cpy.num_blocks() == 2);
		REQUIRE(cptr.get() == nullptr);
		REQUIRE(cptr.num_blocks() == 0);
	}
	
	SECTION("Verify move constructor") {
		auto cptr = cache_ptr(cache.alloc_raw(4), 4, &cache);
		REQUIRE(cptr.get() == handles[0].ptr);
		REQUIRE(cptr.num_blocks() == 4);
	}
	
	SECTION("Verify assignment operator") {
		auto cptr = cache_ptr(cache.alloc_raw(4), 4, &cache);
		auto cptr2 = cache_ptr();
		REQUIRE(cptr.get() == handles[0].ptr);
		REQUIRE(cptr2.get() == nullptr);
		cptr2 = cptr;
		REQUIRE(cptr.get() == nullptr);
		REQUIRE(cptr2.get() == handles[0].ptr);
	}

	SECTION("Verify copy_from") {
		cache_ptr cptr(cache.alloc_raw(1), 1, &cache);
		PcmSample test_samples[] = {0.1f, 0.3, 0.5f, 0.7f, 0.9f};
		cptr.copy_from(test_samples, 5);
		REQUIRE(cptr[0] == 0.1f);
		REQUIRE(cptr[2] == 0.5f);
		REQUIRE(cptr[4] == 0.9f);
	}

	SECTION("Verify copy_from") {
		cache_ptr cptr(cache.alloc_raw(1), 1, &cache);
		cptr[0] = 0.11f;
		cptr[1] = 0.13f;
		cptr[2] = 0.15f;
		cptr[3] = 0.17f;
		cptr[4] = 0.19f;
		PcmSample test_out[512]{0};
		cptr.copy_to(test_out);
		REQUIRE(test_out[0] == 0.11f);
		REQUIRE(test_out[2] == 0.15f);
		REQUIRE(test_out[4] == 0.19f);
	}
}

#include "framework/midi/driver_utility.hpp"
using namespace strangeio::midi;

TEST_CASE("DriverUtilityInterface", "[strangeio::midi]") {
	auto midi_interface = driver_utility();

	SECTION("Get input handle") {
		auto handle = midi_interface.open_input_port("TestName","hw:1,0,0");
		CHECK(handle.get() != nullptr);
		if(handle.get() != nullptr) {
			midi_interface.close_input_port(std::move(handle));
		}
	}
}

#include <future>
#include "framework/midi/driver_utility.hpp"
#include "framework/midi/device.hpp"

TEST_CASE("MidiDevice", "[strangeio::midi]") {
	auto midi_interface = driver_utility();
	auto dev = device("hw:1,0,0", "TestName", &midi_interface);
	
	SECTION("Test Midi device init") {
		auto state = dev.init();
		CHECK(state == true);
		if(state) {
			dev.close_handle();
		}
	}


	/* Todo:
	 * 
	 * These tests need to be run when the midi
	 * device thread can end gracefully
	 * 
	 *	SECTION("Start and Stop Midi device") {
	 *		if(device.init() == true) {
	 *			device.start();
	 *		} else {
	 *			WARN("No midi device present");
	 *		}
	 * 	}
	 */
	
	SECTION("Test sync Midi input") {
		if(dev.init() == true) {
			auto f = 303u;
			auto controller_id = MidiControllerId;

			dev.add_binding(controller_id, [&f](msg) {
				f = 808u;
			});

			WARN("Waiting for midi input on controller " << controller_id);
			dev.test_cycle();
			REQUIRE(f == 808u);
			dev.close_handle();
		} else {
			WARN("No midi device present");
		}
	}

}

TEST_CASE("MidiHandler", "[strangeio::midi]") {
	auto midi_interface = driver_utility();
}

TEST_CASE( "Unit", "[strangeio::component]" ) {

		AlphaUnit unit("Alpha1");
		LinkIn lin {
			.label = "nowhere",
			.id = 0xf,
			.unit = nullptr
		};

		SECTION("Verify unit attributes") {
			REQUIRE(unit.utype() == unit_type::mainline);
			REQUIRE(unit.umodel() == "Alpha");
			REQUIRE(unit.ulabel() == "Alpha1");
		}

		SECTION("Verify unit configurations") {
			unit.set_configuration("test_key", "1001");
			REQUIRE(unit.config_test() == 1001);
		}


		SECTION("Test Warmup cycle and state change") {
			REQUIRE(unit.cstate() == component_state::inactive);
			REQUIRE(unit.init_count() == 0); 

			REQUIRE(unit.cycle_line(cycle_type::warmup) == cycle_state::complete); 
			REQUIRE(unit.cstate() ==component_state::active);
			REQUIRE(unit.init_count() == 1);


			unit.cycle_line(cycle_type::warmup);
			REQUIRE(unit.init_count() == 1);
		}

		SECTION("Test Ac cycle") {
			REQUIRE(unit.cycle_line(cycle_type::ac) == cycle_state::complete); 
		}

		SECTION("Test Sync cycle") {
			REQUIRE(unit.cycle_line(cycle_type::sync) == cycle_state::complete); 
		}

		SECTION("Verify unit's profile") {
			unit.cycle_line(cycle_type::warmup);
			auto unit_profile = unit.unit_profile();
			REQUIRE(unit_profile.fs == 44100);
			REQUIRE(unit_profile.channels == 2);
			REQUIRE(unit_profile.latency == 1);
			REQUIRE(unit_profile.period == 1024);
		}


		SECTION("Veryify non-existant links") {
			REQUIRE(unit.has_input("foo") == -1);
			REQUIRE(unit.has_output("foo") == -1);

			REQUIRE(unit.get_input(0) == nullptr);
			REQUIRE(unit.get_output(0) == nullptr);
		}

		SECTION("Verify faulty connections") {
			REQUIRE(unit.connect(0, nullptr) == false);
			REQUIRE(unit.connect(0, &lin) == false);

			unit.delayed_constructor();
			REQUIRE(unit.connect(0, nullptr) == false);
		}


		SECTION("Verify input/output links and connect") {
			unit.delayed_constructor();
			REQUIRE(unit.has_input("power") != -1);
			REQUIRE(unit.has_output("audio") != -1);
			REQUIRE(unit.connect(0, &lin) == true);
		}

		SECTION("Verify connection state") {
			unit.delayed_constructor();
			unit.connect(0, &lin);
			auto out = unit.get_output(0);
			REQUIRE(out->label == "audio");
			REQUIRE(out->connected == true);
			REQUIRE(out->to->label == "nowhere");
			REQUIRE(out->to->unit == nullptr);
		}

		SECTION("Verify disconnect") {
			unit.delayed_constructor();
			unit.connect(0, &lin);
			unit.disconnect(0);
			auto out = unit.get_output(0);
			REQUIRE(out->connected == false);
			REQUIRE(out->to == nullptr);

		}


		SECTION("Verify Sync line profile") {
			unit.cycle_line(cycle_type::warmup);
			sync_profile profile { 0 };
			sync_profile& p = profile;

			unit.sync_line(p, 0, 0);
			REQUIRE(profile.latency == 1);
			REQUIRE(profile.channels == 2);
			REQUIRE(profile.fs == 44100);
			REQUIRE(profile.period == 1024);
			REQUIRE(profile.drift == 0.10f);
			REQUIRE(profile.jumps == 1);

			unit.sync_line(p, 0, 0);
			REQUIRE(profile.drift == 0.11f);
		}

		SECTION("Verify upstream sync toggle") {
			unit.trigger_upstream();
			REQUIRE(unit.upstream_toggle() == true);
		}

		SECTION("Verify upstream sync detoggle") {
			unit.trigger_upstream();
			sync_profile profile { 0 };
			unit.sync_line(profile, (sync_flag)sync_flags::upstream, 0);
			REQUIRE(unit.upstream_toggle() == false);
		}
}

TEST_CASE("Unit Midi Binding", "[strangeio::component],[strangeio::midi]") {
	auto mu = MuUnit("mu");
	auto interface = driver_utility();
	auto dev = device("hw:1,0,0", "TestController", &interface);
	auto controller_id = MidiControllerId;
	
	SECTION("Verify registered handlers") {
		auto handlers = mu.midi_handlers();
		REQUIRE(handlers.size() > 0);
		REQUIRE(handlers.find("mu_bind") != handlers.end());
		handlers["mu_bind"](msg{0});
		REQUIRE(mu.midi_count() > 0);
	}
	SECTION("Verify binding") {
		if(dev.init()) {
			auto handlers = mu.midi_handlers();
			dev.add_binding(controller_id, handlers["mu_bind"]);
			WARN("Waiting for Midi input from controller " << controller_id);
			dev.test_cycle();
			dev.close_handle();
			REQUIRE(mu.midi_count() > 0);
		} else {
			WARN("Midi device not found");
		}
	}
}

TEST_CASE("rack", "[strangeio::component]") {
		rack rack;

		rack.add_mainline("ac1");
		auto omega = new OmegaUnit("Omega2");
		auto epsilon = new EpsilonUnit("Epsilon1");
		omega->delayed_constructor();
		epsilon->delayed_constructor();

		rack.add_unit(unit_uptr(omega));
		rack.add_unit(unit_uptr(epsilon));


		auto gwptr = rack.get_unit("Omega2");

		SECTION("Verify mounted units") {
			auto mounted_units = rack.get_units();
			REQUIRE(mounted_units.size() > 0);
		}

		SECTION("Get units") {
			auto wptr = rack.get_unit("foobar");
			REQUIRE(wptr.expired() == true);

			wptr = rack.get_unit("Omega2");
			REQUIRE(gwptr.expired() == false);

			wptr = rack.get_unit("Epsilon1");
			REQUIRE(gwptr.expired() == false);
		}

		SECTION("Verify unit") {
			auto wptr = rack.get_unit("Omega2");
			auto sptr = wptr.lock();
			REQUIRE(sptr->ulabel() == "Omega2");

			wptr = rack.get_unit("Epsilon1");
			sptr = wptr.lock();
			REQUIRE(sptr->ulabel() == "Epsilon1");
		}

		SECTION("Verify unit clearing") {
			rack.clear_units();
			auto wptr = rack.get_unit("Omega2");
			REQUIRE(wptr.expired());
		}

		SECTION("Verify expiration") {
			rack.clear_units();
			REQUIRE(gwptr.expired() == true);
		}

		SECTION("Verify faulty connections") {
			REQUIRE(rack.connect_mainline("ac2", "Omega2") == false);
			REQUIRE(rack.connect_mainline("ac1", "Omega3") == false);
			REQUIRE(rack.connect_units("Omega3", "audio", "Epsilon1", "audio_in") == false);
			REQUIRE(rack.connect_units("Omega2", "foobar", "Epsilon1", "audio_in") == false);
			REQUIRE(rack.connect_units("Omega2", "audio", "Epsilon2", "audio_in") == false);
			REQUIRE(rack.connect_units("Omega2", "audio", "Epsilon1", "foobar") == false);

		}

		SECTION("Verify empty cycle") {
			REQUIRE(rack.cycle(cycle_type::warmup) == cycle_state::empty);
		}

		SECTION("Verify connect") {
			REQUIRE(rack.connect_mainline("ac1", "Omega2") == true);
			REQUIRE(rack.connect_units("Omega2", "audio", "Epsilon1", "audio_in") == true);
		}
		
		SECTION("Verify start and stop") {
			rack.start();
			REQUIRE(rack.running() == true);
			REQUIRE(rack.active() == true);
			
			rack.stop();
			REQUIRE(rack.running() == false);
			REQUIRE(rack.active() == false);
		}

}

TEST_CASE("Cycle Cascades", "[strangeio::component]") {
		rack sys;

		sys.add_mainline("ac1");
		auto omega = new OmegaUnit("Omega2");
		auto epsilon = new EpsilonUnit("Epsilon1");
		omega->delayed_constructor();
		epsilon->delayed_constructor();

		sys.add_unit(unit_uptr(omega));
		sys.add_unit(unit_uptr(epsilon));

		sys.connect_mainline("ac1", "Omega2");
		sys.connect_units("Omega2", "audio", "Epsilon1", "audio_in");

		SECTION("Verify Warmup cycle") {
			REQUIRE(sys.cycle(cycle_type::warmup) != cycle_state::empty);
			auto wptr = sys.get_unit("Omega2");
			auto sptr = wptr.lock();
			auto omega = std::static_pointer_cast<OmegaUnit>(sptr);
			REQUIRE(omega->init_count() == 1);
		}

		SECTION("Verify Sync cycle") {
			sys.cycle(cycle_type::warmup);
			REQUIRE(sys.profile().sync_duration == profile_duration::zero());
			sys.sync((sync_flag)sync_flags::sync_duration);
			REQUIRE(sys.profile().sync_duration != profile_duration::zero());
		}

		SECTION("Verify Cycle cascade") {
			sys.cycle(cycle_type::warmup);

			REQUIRE(omega->init_count() == 1);
			REQUIRE(epsilon->init_count() == 1);
		}

		SECTION("Verify Sync cascade") {
			sys.cycle(cycle_type::warmup);
			sync_profile profile{0};
			
			REQUIRE(sys.profile_line(profile, "ac1") == true);

			REQUIRE(profile.jumps == 2);
			REQUIRE(profile.fs == 44100);
			REQUIRE(profile.drift != omega->unit_profile().drift);
			REQUIRE(profile.drift != epsilon->unit_profile().drift);

			auto drift_actual = omega->unit_profile().drift + 
								(omega->unit_profile().drift * 
								epsilon->unit_profile().drift);

			REQUIRE(profile.drift == drift_actual);
		}

		SECTION("Verify global sync cascade") {
			sys.cycle(cycle_type::warmup);

			sys.cycle(cycle_type::sync); // initial sync
			sys.sync((sync_flag)sync_flags::glob_sync); // global resync

			auto profile = sys.global_profile();
			REQUIRE(profile.channels > 0);
			REQUIRE(profile.fs > 0);
			REQUIRE(profile.period > 0);

			
			REQUIRE(omega->global_profile().channels == profile.channels);
			REQUIRE(omega->global_profile().fs == profile.fs);
			REQUIRE(omega->global_profile().period == profile.period);

			REQUIRE(epsilon->global_profile().channels == profile.channels);
			REQUIRE(epsilon->global_profile().fs == profile.fs);
			REQUIRE(epsilon->global_profile().period == profile.period);
		}

		SECTION("Verify Feed cascade") {
			sys.cycle(cycle_type::warmup);
			sys.cycle(cycle_type::ac);

			REQUIRE(omega->feed_count() == 0);
			REQUIRE(epsilon->feed_count() == 1);
			REQUIRE(epsilon->feed_check() == 0.9f);
		}
}

TEST_CASE("Partial Cycles", "[strangeio::component]") {
		component::rack rack;

		rack.add_mainline("ac1");
		rack.add_mainline("ac2");

		auto omega_a = new OmegaUnit("Omega_A");
		auto omega_b = new OmegaUnit("Omega_B");
		auto delta = new DeltaUnit("Delta");
		auto epsilon = new EpsilonUnit("Epsilon");

		omega_a->delayed_constructor();
		omega_b->delayed_constructor();
		epsilon->delayed_constructor();

		rack.add_unit(unit_uptr(omega_a));
		rack.add_unit(unit_uptr(omega_b));
		rack.add_unit(unit_uptr(epsilon));
		rack.add_unit(unit_uptr(delta));

		rack.connect_mainline("ac1", "Omega_A");
		rack.connect_units("Omega_A", "audio", "Delta", "channel_a");
		rack.connect_units("Delta", "audio", "Epsilon", "audio_in");

		SECTION("Verify single active channel step") {
			rack.cycle(cycle_type::warmup);
			REQUIRE(omega_a->init_count() == 1);
			REQUIRE(delta->init_count() == 1);
			REQUIRE(epsilon->init_count() == 1);

			rack.cycle();
			REQUIRE(delta->feed_count() == 1);
			REQUIRE(epsilon->feed_count() == 1);
		}

		SECTION("Verify two active channel partial cycle") {
			rack.connect_mainline("ac2", "Omega_B");
			REQUIRE(rack.connect_units("Omega_B", "audio", "Delta", "channel_b") == true);
			rack.cycle(cycle_type::warmup);

			REQUIRE(omega_b->init_count() == 1);

			rack.cycle();
			REQUIRE(delta->partial_count() == 1);
			REQUIRE(delta->feed_count() == 2);
			REQUIRE(epsilon->feed_count() == 1);
		}
}

TEST_CASE("Cache management in cycle", "[strangeio::component]") {
		component::rack rack;
		auto cache = new memory::cache_manager(32);

		const auto& handles = cache->get_const_handles();

		rack.set_cache_utility(cache);
		rack.add_mainline("ac1");
		
		auto phi = new PhiUnit("phi");
		auto tau = new TauUnit("tau");

		rack.add_unit(unit_uptr(phi));
		rack.add_unit(unit_uptr(tau));

		rack.connect_mainline("ac1", "phi");
		rack.connect_units("phi", "audio", "tau", "audio_in");
		rack.cycle(cycle_type::warmup);
		rack.cycle(cycle_type::sync);
		rack.sync((sync_flag)sync_flags::glob_sync);

		SECTION("Verify links") {
			REQUIRE(phi->init_count() == 1);
			REQUIRE(tau->init_count() == 1);
		}

		SECTION("Verify global profile") {
			REQUIRE(rack.global_profile().channels == 2);
			REQUIRE(rack.global_profile().fs == 44100);
			REQUIRE(rack.global_profile().period == 512);
		}
		
		SECTION("Verify cache sync") {
			REQUIRE(cache->block_size() == 1024);
		}

		SECTION("Verify cache alloc and free") {
			rack.cycle(cycle_type::ac);
			REQUIRE(tau->feed_count() == 1);
			REQUIRE(tau->block_count() == 5);
			REQUIRE(tau->ptr() == handles[0].ptr);
			REQUIRE(handles[0].in_use == false);
			REQUIRE(handles[0].num_blocks == cache->num_blocks());
		}
}

#include "framework/dynlib/library.hpp"

TEST_CASE("LibraryLoader", "[strangeio]") {
	SECTION("Load failure") {
		auto ptr = library::load("nonsense.rso");
		REQUIRE(ptr == nullptr);
	}

	SECTION("Load successfully") {
		auto lib = library::load("unit03/BasicUnit.rso");
		REQUIRE(lib != nullptr);
		lib->close();
	}

	SECTION("Load symbol failure") {
		auto lib = library::load("unit03/BasicUnit.rso");
		auto symbol = lib->load_symbol<int*>("foobar");
		REQUIRE(symbol == nullptr);
		lib->close();
	}

	SECTION("Load symbol successfully") {
		auto lib = library::load("unit03/BasicUnit.rso");
		auto symbol = lib->load_symbol<UnitBuilderPtr>("BuildBasicUnit");
		REQUIRE(symbol != nullptr);
		lib->close();
	}
}

TEST_CASE("Load a unit from library", "[strangeio]") {
	auto lib = library::load("unit03/BasicUnit.rso");
	auto loader = lib->load_symbol<UnitBuilderPtr>("BuildBasicUnit");
	auto unit = loader("basic_unit");
	REQUIRE(unit != nullptr);
	REQUIRE(unit->umodel() == "Basic");
	REQUIRE(unit->ulabel() == "basic_unit");
	lib->close();
}

#include "framework/thread/pkg.hpp"
using namespace strangeio::thread;
TEST_CASE("WorkerPackage", "[strangeio::thread]") {
	auto check = 303u;
	auto task = pkg([&check](void){
		check = 808u;
	});
	task.run();
	REQUIRE(check == 808u);
}

#include "framework/thread/worker.hpp"
TEST_CASE("WorkerThread", "[strangeio::thread]") {

	std::condition_variable cv;
	worker worker(&cv);

	
	SECTION("Verify start and stop") {
		worker.start();
		std::this_thread::sleep_for(std::chrono::milliseconds(20));
		REQUIRE(worker.is_active() == true);
		worker.stop();
		REQUIRE(worker.is_active() == false);
	}
	
	SECTION("Verify task loading") {
		worker.start();
		std::this_thread::sleep_for(std::chrono::milliseconds(20));
		std::promise<int> p;
		std::future<int> f = p.get_future();
		worker.assign_package(std::unique_ptr<pkg>(new pkg(
						[&p](){ 
							p.set_value(303u); 
						}
						)));

		REQUIRE( worker.is_loaded() );
		worker.notify();
		f.wait_for(std::chrono::milliseconds(20));
		worker.stop();
		REQUIRE( f.get() == 303u );		
	}

}

#include "framework/thread/pool.hpp"
TEST_CASE("ThreadPool", "[strangeio::thread]") {

	std::condition_variable cv;
	pool pool(2);
	
	SECTION("Verify size of pool") {
		REQUIRE(pool.size() == 2);
		pool.set_size(4);
		REQUIRE(pool.size() == 4);
	}
	
	SECTION("Verify start and stop") {
		pool.init(&cv);
		pool.start();
		
		auto active = 0u;
		for(auto i = 0u; i < pool.size(); i++) {
			if(pool[i]->is_active())
				active++;
		}

		REQUIRE(active == pool.size());
		
		auto inactive = 0u;
		pool.stop();
		for(auto i = 0; i < pool.size(); i++) {
			if(!pool[i]->is_active())
				inactive++;
		}
		REQUIRE(inactive == pool.size());
	}
}

#include "framework/thread/pump.hpp"
TEST_CASE("PackagePump", "[strangeio::thread]") {

	pump pump;

	SECTION("Verify empty pump") {
		REQUIRE(pump.get_load() == 0);
	}
	
	SECTION("Verify load and unload") {
		auto check = 303u;
		auto task = std::unique_ptr<pkg>(new pkg([&check](void){
			check = 808u;
		}));
		
		pump.add_package(std::move(task));
		REQUIRE(pump.get_load() == 1);
		auto p = pump.next_package();
		p->run();
		REQUIRE(check == 808u);
		REQUIRE(pump.get_load() == 0);
	}
}

#include "framework/thread/queue.hpp"

TEST_CASE("PackageQueue", "[strangeio::thread]") {
	queue queue(2);

	SECTION("Verify sizes") {
		REQUIRE( queue.size() == 2 );
		queue.set_size(4);
		REQUIRE( queue.size() == 4 );
		REQUIRE( queue[10] == nullptr );
	}

	SECTION("Verify start and stop") {
		queue.start();

		REQUIRE( queue.is_running() == true );
		REQUIRE( queue[0] != nullptr );
		REQUIRE( queue[0]->is_running() == true );
		REQUIRE( queue[10] == nullptr );

		// Wait for the queue to fully start up
		std::this_thread::sleep_for(std::chrono::milliseconds(20));
		REQUIRE( queue.is_active() == true );

		queue.stop();
		REQUIRE( queue[0] != nullptr );
		REQUIRE( queue[0]->is_active() == false );
		REQUIRE( queue.is_running() == false );
		REQUIRE( queue.is_active() == false );
	}

	SECTION("Verify package assignment") {
		std::promise<int> p;
		std::future<int> f = p.get_future();
		queue.start();

		// Wait for it to full start up
		std::this_thread::sleep_for(std::chrono::milliseconds(20));
		REQUIRE( queue.is_running() == true );
		REQUIRE( queue[1]->is_running() == true );

		queue.add_package([&p]() { 
						p.set_value(808); 
					});

		f.wait();
		REQUIRE( f.get() == 808 );
	}
}

#include "framework/event/loop.hpp"
#include "framework/event/fwmsg.hpp"

TEST_CASE("Loop", "[strangeio::event]") {
	
	event::loop eloop;
	eloop.add_listener((event::event_type)event::fwmsg::test, [](event::msg_sptr e) {
		return;
	});

	SECTION("Verify sizes") {
		REQUIRE(eloop.size() == 1);
		REQUIRE(eloop.listeners((event::event_type) event::fwmsg::test) == 1);
		REQUIRE(eloop.load() == 0);
	}

	SECTION("Verify check on unregistered event") {
		REQUIRE(eloop.listeners((event::event_type) 101) == 0);
	}

	SECTION("Verify adding new event") {
		eloop.add_listener((event::event_type)101, [](event::msg_sptr e) {
			return;
		});

		REQUIRE(eloop.size() == 2);
		REQUIRE(eloop.listeners((event::event_type) 101) == 1);
	}

}

TEST_CASE("Loading events in loop", "[strangeio::event]") {
	
	event::loop eloop;
	eloop.add_listener((event::event_type)event::fwmsg::test, [](event::msg_sptr e) {
		return;
	});

	SECTION("Verify load failure") {
		auto e = event::msg_uptr(new event::notification());
		e->type = (event::event_type) event::fwmsg::process_complete;
		eloop.add_event(std::move(e));
		REQUIRE(eloop.load() == 0);
	}

	SECTION("Verify load") {
		auto e = event::msg_uptr(new event::notification());
		e->type = (event::event_type)event::fwmsg::test;
		eloop.add_event(std::move(e));
		REQUIRE(eloop.load() > 0);
		// there is a leak here -- low priority since it is tear down
	}
}

#include <cstdlib>
#include <ctime>

TEST_CASE("Full event cycle", "[strangeio::event]") {
	queue tqueue(2);
	tqueue.start();
	std::this_thread::sleep_for(std::chrono::milliseconds(20));

	REQUIRE(tqueue.is_running() == true);

	event::loop eloop;
	eloop.set_queue_utility(&tqueue);

	SECTION("Verify processing of single event task queue") {
		std::promise<int> p;
		auto f = p.get_future();
		eloop.add_listener((event::event_type)event::fwmsg::test, [&p](event::msg_sptr e) {
			p.set_value(808u);
		});

		auto e = event::msg_uptr(new event::notification());
		e->type = (event::event_type)event::fwmsg::test;
		eloop.add_event(std::move(e));

		f.wait_for(std::chrono::milliseconds(250));
		REQUIRE( f.get() == 808u );
		REQUIRE( eloop.max_queue() == 1);
		tqueue.stop();
	}

	SECTION("Verify reset state") {
		int p(0);
		eloop.add_listener((event::event_type)event::fwmsg::test, [&p](event::msg_sptr e) {
			p++;
		});

		auto e = event::msg_uptr(new event::notification());
		e->type = (event::event_type)event::fwmsg::test;
		eloop.add_event(std::move(e));
		std::this_thread::sleep_for(std::chrono::milliseconds(20));
		REQUIRE( p == 1 );

		auto e2 = event::msg_uptr(new event::notification());
		e2->type = (event::event_type)event::fwmsg::test;
		eloop.add_event(std::move(e2));
		std::this_thread::sleep_for(std::chrono::milliseconds(20));

		REQUIRE( p == 2 );
		REQUIRE( eloop.max_queue() == 1 );

		tqueue.stop();
	}

	SECTION("Verify processing of two event task queue") {
		int p(0);
		
		eloop.add_listener((event::event_type)event::fwmsg::test, [&p](event::msg_sptr e) {
			std::this_thread::sleep_for(std::chrono::milliseconds(50));
			p++;
		});

		auto e1 = event::msg_uptr(new event::notification());
		auto e2 = event::msg_uptr(new event::notification());
		e1->type = e2->type = (event::event_type)event::fwmsg::test;

		eloop.add_event(std::move(e1));
		eloop.add_event(std::move(e2));

		std::this_thread::sleep_for(std::chrono::milliseconds(250));
		REQUIRE( p == 2 );
		REQUIRE( eloop.max_queue() == 2);
		tqueue.stop();
	}

	SECTION("Verify heavy load") {
		int num_events(5000);
		int p(0);
		srand(time(NULL));
		eloop.add_listener((event::event_type)event::fwmsg::test, [&p](event::msg_sptr e) {
			auto wp = rand() % 4;
			std::this_thread::sleep_for(std::chrono::milliseconds(wp));
			p++;
		});

		WARN("Queuing and processing " << num_events << " events; this may take a minute...");
		
		for(auto i = 0; i < num_events; i++ ) {
			auto e = event::msg_uptr(new event::notification());
			e->type = (event::event_type)event::fwmsg::test;
			eloop.add_event(std::move(e));
			auto wp = rand() % 5;
			std::this_thread::sleep_for(std::chrono::milliseconds(wp));
		}

		while(p < num_events) continue;

		INFO("Max queue: " << eloop.max_queue());
		REQUIRE( p == num_events );
		
		tqueue.stop();
	}


}

#include "framework/config/document.hpp"

TEST_CASE( "Load a configuration document", "[strangeio::config]" ) {
	config::document doc;

	auto config = doc.load("basic.cfg");

	SECTION ( "Checking system" ) {
		REQUIRE( config->system.threads.num_workers == 3 );
	}

	SECTION ( "Checking mainlines" ) {
		REQUIRE( config->setup.mainlines.size()		== 1 );
		REQUIRE( config->setup.mainlines[0]			== "ac1" );
	}

	SECTION ( "Checking midi devices" ) {
		REQUIRE( config->midi.controllers.size()		== 1 );
		REQUIRE( config->midi.controllers[0].label		== "LaunchControl" );
		REQUIRE( config->midi.controllers[0].port		== "hw:1,0,0" );
	}

	SECTION ( "Checking daisychains" ) {
		REQUIRE( config->setup.daisychains.size() == 2 );

		REQUIRE( config->setup.daisychains[0].from	== "rack" );
		REQUIRE( config->setup.daisychains[0].plug	== "ac1" );
		REQUIRE( config->setup.daisychains[0].to	== "phi" );
		REQUIRE( config->setup.daisychains[0].jack	== "power" );

		REQUIRE( config->setup.daisychains[1].from	== "phi" );
		REQUIRE( config->setup.daisychains[1].plug	== "audio" );
		REQUIRE( config->setup.daisychains[1].to	== "tau" );
		REQUIRE( config->setup.daisychains[1].jack	== "audio_in" );
	}

	SECTION ( "Checking units" ) {
		REQUIRE( config->setup.units.size() == 2 );

		REQUIRE( config->setup.units[0].label				== "phi" );
		REQUIRE( config->setup.units[0].unit				== "Phi" );
		REQUIRE( config->setup.units[0].library				== "./unit03/TestUnits.rso" );
		REQUIRE( config->setup.units[0].configs.size()		== 0 );
		REQUIRE( config->setup.units[0].bindings.size()		== 1 );
		REQUIRE( config->setup.units[0].bindings[0].name	== "exported" );
		REQUIRE( config->setup.units[0].bindings[0].module	== "LaunchControl" );
		REQUIRE( config->setup.units[0].bindings[0].code	== 73 );

		REQUIRE( config->setup.units[1].label				== "tau" );
		REQUIRE( config->setup.units[1].unit				== "Tau" );
		REQUIRE( config->setup.units[1].library				== "./unit03/TestUnits.rso" );
		REQUIRE( config->setup.units[1].configs.size()		== 2 );

		REQUIRE( config->setup.units[1].configs[1].type		== "test_config" );
		REQUIRE( config->setup.units[1].configs[1].value	== "808" );
		
		REQUIRE( config->setup.units[1].configs[0].type		== "nowt" );
		REQUIRE( config->setup.units[1].configs[0].value	== "nada" );
		REQUIRE( config->setup.units[1].bindings.size()		== 0 );
	}
}

#include <memory>

#include "framework/config/assembler.hpp"
#include "unit03/Tau.hpp"
#include "unit03/Phi.hpp"

TEST_CASE( "Assemble rack from configuration", "[strangeio::config]" ) {

	std::unique_ptr<component::unit_loader> vloader(new component::unit_loader());
	config::assembler as(std::move(vloader));
	config::document doc;
	component::rack sys;

	
	auto vqueue = new thread::queue(2);
	auto vdriver = new midi::driver_utility();
	auto vmidi = new midi::midi_handler(vdriver);


	sys.set_queue_utility(vqueue);
	sys.set_midi_handler(vmidi);

	const auto vconfig = doc.load("basic.cfg");
	as.assemble((*vconfig), sys);

	SECTION( "Checking number of workers" ) {
		auto queue = sys.get_queue_utility();
		REQUIRE( queue->size() == vconfig->system.threads.num_workers );
		
	}

	SECTION( "Checking units in rack" ) {
		REQUIRE( sys.has_unit("phi") );
		REQUIRE( sys.has_unit("tau") );
	}

	SECTION( "Checking MIDI devices" ) {
		auto lmidi = sys.get_midi_handler();
		auto& devices = lmidi->devices();
		REQUIRE( devices.size() == 1);
		REQUIRE( devices[0].get_alias() == "LaunchControl" );

		auto bindings = devices[0].get_bindings();
		auto binding = bindings.find(73);
		REQUIRE( binding != bindings.end() );
	}


	SECTION( "Checking specific unit" ) {
		auto u = sys.get_unit("phi").lock();
	
		REQUIRE( u->utype() == component::unit_type::mainline );
		REQUIRE( u->umodel() == "Phi" );
		REQUIRE( u->ulabel() == "phi" );
		REQUIRE( u->has_output("audio") > -1);
	}

	SECTION( "Checking daisychain" ) {
		auto phi = sys.get_unit("phi").lock();
		auto tau = sys.get_unit("tau").lock();

		auto out = phi->get_output(0);
		REQUIRE( out->label				== "audio" );
		REQUIRE( out->connected		== true);

		auto lk = out->to->unit;
		REQUIRE( lk					!= nullptr );
		auto ut = static_cast<unit*>(lk);
		REQUIRE( ut->ulabel()		== "tau" );
	}

	SECTION( "Check configuation" ) {
		auto tau  = sys.get_unit("tau").lock();
		REQUIRE(tau->get_configuration("test_config") == std::to_string(808));
	}
}
#include "framework/routine/system.hpp"
TEST_CASE("Assemble rack from setup function", "[strangeio::routine]") {
	config::assembler as;


	auto sys_ptr = strangeio::routine::system::setup(as, "basic.cfg", 32);
	auto& sys = *sys_ptr;


	SECTION( "Checking units in rack" ) {
		REQUIRE( sys.has_unit("phi") );
		REQUIRE( sys.has_unit("tau") );
	}

	SECTION( "Checking MIDI devices" ) {
		auto lmidi = sys.get_midi_handler();
		auto& devices = lmidi->devices();
		REQUIRE( devices.size() == 1);
		REQUIRE( devices[0].get_alias() == "LaunchControl" );

		auto bindings = devices[0].get_bindings();
		auto binding = bindings.find(73);
		REQUIRE( binding != bindings.end() );
	}


	SECTION( "Checking specific unit" ) {
		auto u = sys.get_unit("phi").lock();
	
		REQUIRE( u->utype() == component::unit_type::mainline );
		REQUIRE( u->umodel() == "Phi" );
		REQUIRE( u->ulabel() == "phi" );
		REQUIRE( u->has_output("audio") > -1);
	}

	SECTION( "Checking daisychain" ) {
		auto phi = sys.get_unit("phi").lock();
		auto tau = sys.get_unit("tau").lock();

		auto out = phi->get_output(0);
		REQUIRE( out->label				== "audio" );
		REQUIRE( out->connected		== true);

		auto lk = out->to->unit;
		REQUIRE( lk					!= nullptr );
		auto ut = static_cast<unit*>(lk);
		REQUIRE( ut->ulabel()		== "tau" );
	}

	SECTION( "Check configuation" ) {
		auto tau  = sys.get_unit("tau").lock();
		REQUIRE(tau->get_configuration("test_config") == std::to_string(808));
	}
}
#ifdef __linux__
#include <thread>

#include "framework/alias.hpp"
#include "framework/routine/system.hpp"

TEST_CASE( "Sine Test", "[strangeio::linux]" ) {

	std::unique_ptr<component::unit_loader> vloader(new component::unit_loader());
	config::assembler as(std::move(vloader));
	auto sys = strangeio::routine::system::setup(as, "linux.cfg", 32);

	REQUIRE( sys->has_unit("theta") );
	REQUIRE( sys->has_unit("zeta") );
	sys->warmup();
	REQUIRE(sys->global_profile().channels > 0);
	REQUIRE(sys->global_profile().fs > 0);
	REQUIRE(sys->global_profile().period > 0);

	SECTION("Start process") {

		sys->start();
		REQUIRE(sys->running() == true);
		REQUIRE(sys->active() == true);

		sys->trigger_cycle();
		auto waiter = 0u;
		WARN("Playing 200Hz tone\nEnter 'q' to stop $ ");
		std::cin >> waiter;
		sys->stop();
		REQUIRE(sys->running() == false);
		REQUIRE(sys->active() == false);
	}

}

#endif
