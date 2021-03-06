#include "framework/component/unit.hpp"
using namespace strangeio;
class Tau : public component::unit {
public:
	Tau(std::string label);
	
	void set_configuration(std::string key, std::string value);
	std::string get_configuration(std::string key);

	void feed_line(memory::cache_ptr samples, int id) override;

	component::cycle_state cycle() override;
	component::cycle_state init() override;

	// Checks
	int init_count() const;
	int feed_count() const;
	int block_count() const;
	int config_test() const;

private:
	int m_init_count, m_feed_count, m_block_count, m_config_test;
	const PcmSample* m_ptr;

};