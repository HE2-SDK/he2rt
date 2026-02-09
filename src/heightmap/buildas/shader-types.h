#pragma once

namespace he2rt::heightmap::buildas {
	static const uint32_t MIP_COUNT = 5;
	static const uint32_t WORKGROUP_SIZE = (1 << MIP_COUNT);

	enum Binding {
		input = 0,
		output,
	};

	struct Config {
		uint32_t inputLod;
	};
}
