// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <algorithm>
#include <cstdint>
#include <mach/vm_statistics.h>

namespace Mem {

	inline uint64_t used_memory(const vm_statistics64& p, uint64_t page_size, uint64_t total) {
		// File-backed and purgeable pages are reclaimable. Count the physical
		// compressor footprint, not the uncompressed size of its contents.
		const int64_t pages = static_cast<int64_t>(p.active_count) + p.inactive_count
			+ p.wire_count + p.speculative_count + p.compressor_page_count
			- p.purgeable_count - p.external_page_count;
		return std::min(static_cast<uint64_t>(std::max<int64_t>(0, pages)) * page_size, total);
	}

}
