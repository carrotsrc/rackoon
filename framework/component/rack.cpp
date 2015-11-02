#include <chrono>
#include <iostream>

#ifdef __linux__
	#include <sched.h>
	#include <errno.h>
	#include <string.h>
	#define	SIO_SCHED SCHED_FIFO
#endif

#include "framework/alias.hpp"
#include "framework/component/rack.hpp"

using namespace strangeio;
using namespace strangeio::component;

using pclock = std::chrono::steady_clock;

rack::rack()
	: backend()
	, m_schpolicy({0})
	, m_active(false)
	, m_running(false)
	, m_resync(false)
	, m_cycle_queue(0)
	, m_last_trigger(0)
	

{

}

rack::~rack() {
	if(m_active) {
		stop();
	}
	
	if(m_cache) delete m_cache;
}


void rack::mount_dependencies(unit* u) {
    
	u->set_rack(this);

	u->set_cache_utility(m_cache);
	u->set_loop_utility(m_loop);
	u->set_queue_utility(m_queue);
}

void rack::trigger_sync(sync_flag flags) {
	// don't unflag previous flags
	if(flags) m_resync_flags |= flags;

	m_resync = true;
}

void rack::trigger_cycle() {
	auto ns = siortn::debug::epoch_ns();
	m_thread_trig = ns;
	m_last_trigger = ns;
	
	++m_cycle_queue;
	if(m_cycle_queue == 1) {
		m_tps = siortn::debug::clock_time();
		m_trigger.notify_one();
	}
	
}

void rack::resync() {
	if(m_cache == nullptr) return;
	if(m_cache->block_size() > 0) return;
	
	m_cache->build_cache(m_global_profile.period * m_global_profile.channels);
}



void rack::warmup() {
	cycle(cycle_type::warmup);
	cycle(cycle_type::sync);
	sync((sync_flag)sync_flags::glob_sync);
}

#include <chrono>
#include <limits>

void rack::start() {
	m_running = true;
	m_active = false;
	
	
	m_rack_thread = siothr::scheduled(m_schpolicy, [this](){

		// profiling
		auto peak = 0, peak_sync = 0;
		auto trough = std::numeric_limits<int>::max(), trough_sync = std::numeric_limits<int>::max();
		
		auto decay = 50u;
		auto avg_cycle = 0.0l;
		
		
		// ---------

		m_active = true;
		std::unique_lock<std::mutex> lock(m_trigger_mutex);

		while(m_running) {
			m_trigger.wait(lock);			
			m_tpe = siortn::debug::clock_time();
			
			
			if(!m_running) {
				lock.unlock();
				break;
			}

			while(m_cycle_queue > 0) {
				auto t_start = siortn::debug::clock_time(); // Profile: Cycle

				auto s_start = siortn::debug::zero_timepoint();
				auto s_end = siortn::debug::zero_timepoint();
				cycle();
				/* Put the sync cycle *after* the ac cycle.
				 * The reason being that we are now currently 
				 * in the latency window. If we did it before 
				 * the ac cycle, we would be syncing at the 
				 * trigger point of sound driver's ring buffer, 
				 * and we need to get samples there ASAP... not 
				 * faff around with syncing the units
				 */
				if(m_resync) {
					s_start = siortn::debug::clock_time(); // Profile: Sync
					// syncs really shouldn't happen too often
					if(m_resync_flags) {
						if(m_resync_flags & (sync_flag)sync_flags::upstream) {
							/* upstream takes priority for now
							 * because it will override the entire
							 * sync.
							 */
							sync((sync_flag)sync_flags::upstream);
							m_resync_flags ^=  (sync_flag)sync_flags::upstream;
						}
						sync((sync_flag)m_resync_flags);

						m_resync_flags = 0;
					} else {
						cycle(cycle_type::sync);
					}

					// Switch off the flag (might need to lock?)
					m_resync = false;
					s_end = siortn::debug::clock_time(); // ~Profile: Sync
					
				}
				--m_cycle_queue;


				// Profiling

				auto t_end = siortn::debug::clock_time(); // ~Profile: Cycle
				
				auto delta = siortn::debug::clock_delta_us(t_start, t_end);
				auto s_delta = siortn::debug::clock_delta_us(s_start, s_end);
				avg_cycle += delta;
				peak = delta > peak ? delta : peak;
				trough = delta < trough ? delta : trough;
				

				peak_sync = s_delta > peak_sync ? s_delta : peak_sync;
				trough_sync = s_delta < trough_sync ? s_delta : trough_sync;

				if(--decay == 0) {
					peak = 0;
					peak_sync = 0;
					avg_cycle = 0;
					trough = std::numeric_limits<int>::max();
					trough_sync = std::numeric_limits<int>::max();
					decay = 50;
				
				}
			}

			
		}
		m_active = false;
		
	}, "rack");

	// busy loop until activation
	while(!m_active) {
		asm(""); //  don't optimise away loop!
		continue;
	}
}

void rack::stop() {
	m_running = false;
	m_trigger.notify_one();
	m_rack_thread.join();
}

bool rack::active() {
	return m_active;
}

bool rack::running() {
	return m_running;
}

void rack::assign_schpolicy(strangeio::thread::sched_desc policy) {
	m_schpolicy = policy;
}