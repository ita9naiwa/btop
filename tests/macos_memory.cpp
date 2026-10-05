// SPDX-License-Identifier: Apache-2.0

#include "osx/memory.hpp"
#include "btop_config.hpp"
#include "btop_draw.hpp"
#include "btop_shared.hpp"
#include "btop_theme.hpp"
#include "btop_tools.hpp"

#include <gtest/gtest.h>

TEST(macos_memory, compressed_ram_is_used) {
	vm_statistics64 p{};
	p.active_count = 200;
	p.inactive_count = 150;
	p.wire_count = 100;
	p.speculative_count = 20;
	p.external_page_count = 120;
	p.purgeable_count = 50;
	const uint64_t page_size = 16384;
	const uint64_t total = 1000 * page_size;
	EXPECT_EQ(Mem::used_memory(p, page_size, total), 300 * page_size);

	p.compressor_page_count = 400;
	p.total_uncompressed_pages_in_compressor = 1600;
	EXPECT_EQ(Mem::used_memory(p, page_size, total), 700 * page_size);
	EXPECT_EQ(total - Mem::used_memory(p, page_size, total), 300 * page_size);
}

TEST(macos_memory, inconsistent_counters_do_not_wrap) {
	vm_statistics64 p{};
	p.external_page_count = 100;
	EXPECT_EQ(Mem::used_memory(p, 4096, 40960), 0);
	p.wire_count = 1000;
	EXPECT_EQ(Mem::used_memory(p, 4096, 40960), 40960);
}

TEST(macos_memory, compact_and_full_layouts) {
	Config::set("shown_boxes", "mem"s);
	Config::set("show_disks", true);
	Config::set("show_swap", false);
	Theme::setTheme();
	Mem::mem_info mem;
	for (const auto& name : Mem::mem_names) {
		mem.stats.at(name) = 1 << 20;
		mem.percent.at(name) = {10, 20};
	}
	for (const auto [width, height] : {std::pair{36, 8}, std::pair{104, 20}}) {
		Term::width = width;
		Term::height = height;
		for (bool graphs : {false, true}) {
			Config::set("mem_graphs", graphs);
			Draw::calcSizes();
			for (const int pressure : {4, 1, 2, 0}) {
				SCOPED_TRACE(::testing::Message() << width << 'x' << height << " graphs=" << graphs << " pressure=" << pressure);
				mem.pressure = pressure;
				const auto output = Mem::draw(mem, false, false);
				EXPECT_NE(output.find(width == 36 ? "(Cmp)" : "(Compr.)"), string::npos);
				EXPECT_NE(output.find(width == 36 ? "F " : "Free:"), string::npos);
				const string state = pressure == 1 ? "Normal" : pressure == 2 ? "Warn" : pressure == 4 ? "Critical" : "N/A";
				// Shorter states must erase the previous footer on incremental draws.
				EXPECT_NE(output.find(Tools::ljust(state, 8)), string::npos);
			}
		}
	}
}
