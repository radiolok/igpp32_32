#include "igpp/patterns.hpp"

#include <cstring>

namespace igpp::patterns {

namespace {

// Outline of every panel plus the diagonals of the whole field: shows panel
// order, orientation and the column/row mapping at a glance.
void render_panels(uint32_t, Frame& f) {
    fill(f, false);
    for (int x = 0; x < kWidth; ++x) {
        for (int y = 0; y < kHeight; ++y) {
            const int px = x % kPanelSize, py = y % kPanelSize;
            const bool edge = px == 0 || py == 0 || px == kPanelSize - 1 || py == kPanelSize - 1;
            const bool diag = x == y || x == kWidth - 1 - y;
            if (edge || diag) {
                set_pixel(f, x, y, true);
            }
        }
    }
    // Panel number as 1..9 dots in the top-left corner of each panel.
    for (int r = 0; r < kPanelRows; ++r) {
        for (int c = 0; c < kPanelCols; ++c) {
            const int n = r * kPanelCols + c + 1;
            for (int k = 0; k < n; ++k) {
                set_pixel(f, c * kPanelSize + 3 + 2 * k, r * kPanelSize + 3, true);
            }
        }
    }
}

void render_checker(uint32_t i, Frame& f) {
    for (int x = 0; x < kWidth; ++x) {
        for (int y = 0; y < kHeight; ++y) {
            set_pixel(f, x, y, ((x + y + int(i)) & 1) == 0);
        }
    }
}

void render_column_walk(uint32_t i, Frame& f) {
    fill(f, false);
    const int x = int(i % kWidth);
    for (int y = 0; y < kHeight; ++y) {
        set_pixel(f, x, y, true);
    }
}

void render_row_walk(uint32_t i, Frame& f) {
    fill(f, false);
    const int y = int(i % kHeight);
    for (int x = 0; x < kWidth; ++x) {
        set_pixel(f, x, y, true);
    }
}

// One dot per panel moving along a diagonal: checks that every anode/cathode
// pair of every panel can fire, without lighting much at once.
void render_dot_walk(uint32_t i, Frame& f) {
    fill(f, false);
    const int k = int(i % (kPanelSize * kPanelSize));
    const int px = k % kPanelSize, py = k / kPanelSize;
    for (int r = 0; r < kPanelRows; ++r) {
        for (int c = 0; c < kPanelCols; ++c) {
            set_pixel(f, c * kPanelSize + px, r * kPanelSize + py, true);
        }
    }
}

void render_all_on(uint32_t, Frame& f) { fill(f, true); }

void render_msp_picture(uint32_t i, Frame& f) {
    std::memcpy(f.data(), kMspPicture[i & 1], kFrameBytes);
}

const Pattern kPatterns[] = {
    {"msp-picture", 2, 2000, render_msp_picture},
    {"test-panels", 1, 5000, render_panels},
    {"test-checker", 2, 500, render_checker},
    {"test-columns", kWidth, 60, render_column_walk},
    {"test-rows", kHeight, 60, render_row_walk},
    {"test-dots", kPanelSize * kPanelSize, 20, render_dot_walk},
    {"test-all-on", 1, 3000, render_all_on},
};

}  // namespace

const Pattern* all(size_t& count) {
    count = sizeof(kPatterns) / sizeof(kPatterns[0]);
    return kPatterns;
}

const Pattern* find(const char* name) {
    for (const auto& p : kPatterns) {
        if (std::strcmp(p.name, name) == 0) {
            return &p;
        }
    }
    return nullptr;
}

}  // namespace igpp::patterns
