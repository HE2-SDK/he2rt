#define TONEMAP_WORKGROUP_SIZE 16
#define EXPOSURE_HISTOGRAM_SIZE 256

namespace he2rt::shaders::tonemapper {
	enum Binding {
		input = 0,
		output,
		histogram,
		luminance,
	};

	enum Method {
		eFilmic = 0,
		eUncharted2,
		eClip,
		eACES,
		eAgX,
		eKhronosPBR,
	};

	struct Config {
		int isActive = 1;
		int method = Method::eFilmic;

		// Input color correction
		float exposure = 1.0f;
		float temperature = 6506.11144f;  // Color temperature in degrees Kelvin. The default produces sRGB white for our approximation
		float tint = 3.25895312e-3f;  // Tint adjustment as an ANSI C78.377-2008 Duv value. The default produces sRGB white for our approximation

		// Post effects
		float contrast = 1.0f;
		float brightness = 1.0f;
		float saturation = 1.0f;
		float vignette = 0.0f;

		// Advanced color grading
		float vibrance = 0.0f;   // Boosts desaturated colors while preserving saturated ones
		float shadowBias = 0.0f;   // -1 to 1: Adjust shadow tones
		float midtoneBias = 0.0f;   // -1 to 1: Adjust midtone brightness
		float highlightBias = 0.0f;   // -1 to 1: Adjust highlight tones
		float3 coolColor = float3(1.0f, 1.0f, 1.0f);  // Tint for shadows in split toning
		float3 warmColor = float3(1.0f, 1.0f, 1.0f);  // Tint for highlights in split toning
		float splitBalance = 0.0f;   // -0.5 to 0.5: Balance between cool and warm tones

		// Auto-Exposure
		int autoExposure = 0;      // 0: off, 1: on
		float autoExposureSpeed = 5.0F;   // Speed of auto-exposure adjustment
		float evMinValue = -5.0F;  // Minimum EV value for auto-exposure
		float evMaxValue = 10.0F;  // Maximum EV value for auto-exposure
		uint32_t enableCenterMetering = 0;      // 0 = off, 1 = on
		float centerMeteringSize = 0.5F;   // Size of the center metering area (0.0 to 1.0, where 1.0 is the full screen)
		uint32_t averageMode = 1;      // 0 = Mean, 1 = Median
		// Dither
		int dither = 1;  // 0: no dither, 1: dither
	};
}
