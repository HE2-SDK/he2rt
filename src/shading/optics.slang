#pragma once

float schlickFresnelFactor(float cos_theta, float offset = 0.0f, float power = 5.0f, float gain = 0.0f) {
    return gain + pow(1.0 + offset - cos_theta, power);
}

__generic<int N> vector<float, N> schlickFresnel(float cos_theta, vector<float, N> f0, vector<float, N> f90) {
    return lerp(f0, f90, schlickFresnelFactor(cos_theta));
}
