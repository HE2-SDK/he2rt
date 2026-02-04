#define BLOOM_WORKGROUP_SIZE 16

namespace he2rt::shaders::bloom {
	enum Binding {
		input = 0,
		output,
	};

	struct Config {
		uint inputLod;
		float kernelSize;
	};
}
