#include "player.hpp"

#include <dirent.h>
#include <sys/stat.h>

#include <algorithm>
#include <atomic>
#include <cctype>
#include <cstdio>
#include <memory>
#include <mutex>
#include <vector>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "igpp/display.hpp"
#include "igpp/igv.hpp"
#include "igpp/patterns.hpp"
#include "sdkconfig.h"
#include "storage.hpp"

namespace player {

namespace {

constexpr const char* TAG = "player";
using igpp::Frame;
using igpp::kFrameBytes;

struct Item {
    enum class Kind { Builtin, Igv, BinDir } kind;
    std::string name;
    std::string path;
    const igpp::patterns::Pattern* pattern = nullptr;
    uint32_t frames = 0;
    uint16_t frame_ms = 40;
    std::vector<std::string> files;  // BinDir only
};

enum Cmd : int { kNone = -1, kNext = -2, kRescan = -3 };

std::mutex g_mu;
std::vector<Item> g_items;  // guarded by g_mu for readers outside the task
std::atomic<int> g_cmd{kRescan};
std::atomic<Mode> g_mode{Mode::Playlist};
std::atomic<size_t> g_index{0};
std::atomic<TickType_t> g_last_live{0};
bool g_test_mode = false;

bool ends_with_ci(const std::string& s, const char* ext) {
    const size_t n = std::char_traits<char>::length(ext);
    if (s.size() < n) return false;
    for (size_t i = 0; i < n; ++i) {
        if (std::tolower(static_cast<unsigned char>(s[s.size() - n + i])) != ext[i]) return false;
    }
    return true;
}

bool is_dir(const std::string& path) {
    struct stat st;
    return stat(path.c_str(), &st) == 0 && S_ISDIR(st.st_mode);
}

std::vector<std::string> list_dir(const std::string& path) {
    std::vector<std::string> out;
    DIR* d = opendir(path.c_str());
    if (!d) return out;
    while (dirent* e = readdir(d)) {
        if (e->d_name[0] != '.') out.emplace_back(e->d_name);
    }
    closedir(d);
    std::sort(out.begin(), out.end());
    return out;
}

void scan_root(const char* root, std::vector<Item>& items) {
    for (const auto& name : list_dir(root)) {
        const std::string path = std::string(root) + "/" + name;
        if (ends_with_ci(name, ".igv")) {
            FILE* f = fopen(path.c_str(), "rb");
            if (!f) continue;
            uint8_t hdr[igpp::igv::kHeaderSize];
            igpp::igv::Header h;
            const size_t n = fread(hdr, 1, sizeof(hdr), f);
            fclose(f);
            const auto err = igpp::igv::parse_header(hdr, n, h);
            if (err != igpp::igv::Error::Ok) {
                ESP_LOGW(TAG, "%s: %s", path.c_str(), igpp::igv::error_str(err));
                continue;
            }
            if (h.frame_count == 0) continue;
            items.push_back({Item::Kind::Igv, name, path, nullptr, h.frame_count, h.frame_ms, {}});
        } else if (is_dir(path)) {
            // A folder of raw 1152-byte frames, as written by igpp.py --byte.
            std::vector<std::string> files;
            for (const auto& f : list_dir(path)) {
                if (!is_dir(path + "/" + f)) files.push_back(path + "/" + f);
            }
            if (files.empty()) continue;
            const uint32_t count = uint32_t(files.size());
            items.push_back({Item::Kind::BinDir, name + "/", path, nullptr, count,
                             uint16_t(CONFIG_IGPP_BIN_FRAME_MS), std::move(files)});
        }
    }
}

void rebuild() {
    std::vector<Item> items;
    if (!g_test_mode) {
        if (storage::sd_mounted()) scan_root(storage::kSdRoot, items);
        if (storage::flash_mounted()) scan_root(storage::kFlashRoot, items);
    }
    if (items.empty()) {
        size_t n = 0;
        const auto* pats = igpp::patterns::all(n);
        for (size_t i = 0; i < n; ++i) {
            items.push_back({Item::Kind::Builtin, pats[i].name, "", &pats[i], pats[i].frame_count,
                             pats[i].frame_ms, {}});
        }
    }
    std::lock_guard<std::mutex> lock(g_mu);
    g_items = std::move(items);
    if (g_index >= g_items.size()) g_index = 0;
    ESP_LOGI(TAG, "playlist: %u items%s", unsigned(g_items.size()), g_test_mode ? " (test mode)" : "");
}

// Reads frames of one item in order.
class Source {
public:
    explicit Source(const Item& item) : item_(item) {
        if (item_.kind == Item::Kind::Igv) {
            file_ = fopen(item_.path.c_str(), "rb");
            if (file_) setvbuf(file_, nullptr, _IOFBF, 8192);
        }
    }
    ~Source() {
        if (file_) fclose(file_);
    }
    bool read(uint32_t i, Frame& out) {
        switch (item_.kind) {
        case Item::Kind::Builtin:
            item_.pattern->render(i, out);
            return true;
        case Item::Kind::Igv:
            if (!file_) return false;
            if (i == 0 && fseek(file_, long(igpp::igv::frame_offset(0)), SEEK_SET) != 0) return false;
            return fread(out.data(), 1, kFrameBytes, file_) == kFrameBytes;
        case Item::Kind::BinDir: {
            FILE* f = fopen(item_.files[i].c_str(), "rb");
            if (!f) return false;
            const bool ok = fread(out.data(), 1, kFrameBytes, f) == kFrameBytes;
            fclose(f);
            return ok;
        }
        }
        return false;
    }

private:
    const Item& item_;
    FILE* file_ = nullptr;
};

bool live_timed_out() {
    return xTaskGetTickCount() - g_last_live.load() > pdMS_TO_TICKS(CONFIG_IGPP_LIVE_TIMEOUT_MS);
}

// Returns false when playback must be interrupted.
bool keep_playing() {
    if (g_cmd.load() != kNone) return false;
    if (g_mode.load() == Mode::Live && live_timed_out()) {
        g_mode = Mode::Playlist;
        ESP_LOGI(TAG, "live stream ended, back to playlist");
    }
    return g_mode.load() == Mode::Playlist;
}

void play_item(const Item& item) {
    static Frame frame;  // 1152 B, keep off the task stack
    Source src(item);
    const TickType_t min_ticks = pdMS_TO_TICKS(CONFIG_IGPP_ITEM_MIN_SECONDS * 1000);
    const TickType_t started = xTaskGetTickCount();
    TickType_t wake = started;
    do {
        for (uint32_t i = 0; i < item.frames; ++i) {
            if (!keep_playing()) return;
            if (!src.read(i, frame)) {
                ESP_LOGW(TAG, "%s: read error at frame %u", item.name.c_str(), unsigned(i));
                return;
            }
            igpp::display::submit(frame, pdMS_TO_TICKS(100));
            xTaskDelayUntil(&wake, std::max<TickType_t>(1, pdMS_TO_TICKS(item.frame_ms)));
        }
    } while (xTaskGetTickCount() - started < min_ticks);
}

void task(void*) {
    for (;;) {
        const int cmd = g_cmd.exchange(kNone);
        if (cmd == kRescan) {
            rebuild();
        } else if (cmd == kNext) {
            g_index = (g_index + 1) % std::max<size_t>(1, g_items.size());
        } else if (cmd >= 0) {
            g_index = size_t(cmd) % std::max<size_t>(1, g_items.size());
        }

        if (!keep_playing() || g_items.empty()) {
            vTaskDelay(pdMS_TO_TICKS(20));
            continue;
        }
        const size_t idx = g_index;
        ESP_LOGI(TAG, "playing %u: %s", unsigned(idx), g_items[idx].name.c_str());
        play_item(g_items[idx]);
        // Advance unless a command already chose what is next.
        if (g_cmd.load() == kNone && g_mode.load() == Mode::Playlist) {
            g_index = (idx + 1) % g_items.size();
        }
    }
}

}  // namespace

void start(bool test_mode) {
    g_test_mode = test_mode;
    xTaskCreatePinnedToCore(task, "player", 6144, nullptr, 5, nullptr, 0);
}

void next() {
    g_mode = Mode::Playlist;
    g_cmd = kNext;
}

void play(uint16_t i) {
    g_mode = Mode::Playlist;
    g_cmd = int(i);
}

void stop() {
    g_mode = Mode::Stopped;
    igpp::display::blank();
}

void resume() { g_mode = Mode::Playlist; }

void rescan() { g_cmd = kRescan; }

void live_frame(const Frame& frame) {
    g_last_live = xTaskGetTickCount();
    g_mode = Mode::Live;
    igpp::display::submit(frame, pdMS_TO_TICKS(30));
}

Mode mode() { return g_mode.load(); }

size_t size() {
    std::lock_guard<std::mutex> lock(g_mu);
    return g_items.size();
}

size_t index() { return g_index.load(); }

std::string list() {
    std::lock_guard<std::mutex> lock(g_mu);
    std::string out;
    for (const auto& it : g_items) {
        out += it.name;
        out += '\n';
    }
    return out;
}

}  // namespace player
