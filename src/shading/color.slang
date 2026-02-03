#pragma once

__generic<int N> vector<float, N> srgbToLinearSimple(vector<float, N> x) {
    return x;//pow(abs(x), 2.2);
}
