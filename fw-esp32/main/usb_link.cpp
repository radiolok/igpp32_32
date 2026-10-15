#include "usb_link.hpp"

#include <cstdio>
#include <cstring>
#include <string>

#include "driver/usb_serial_jtag.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "igpp/display.hpp"
#include "igpp/protocol.hpp"
#include "player.hpp"
#include "storage.hpp"

namespace usb_link {

namespace {

constexpr const char* TAG = "usb";
using namespace igpp::proto;

void reply(Type type, const void* payload, uint16_t len) {
    const auto pkt = make_packet(uint8_t(type) | kReplyFlag, static_cast<const uint8_t*>(payload), len);
    usb_serial_jtag_write_bytes(pkt.data(), pkt.size(), pdMS_TO_TICKS(50));
}

void reply_status(Type type, uint8_t code) { reply(type, &code, 1); }

// Accept plain file names only: no paths, no "..".
bool valid_name(const std::string& n) {
    if (n.empty() || n.size() > 64) return false;
    for (char c : n) {
        if (c == '/' || c == '\\' || c < 0x20) return false;
    }
    return n != "." && n != "..";
}

class Link : public Handler {
public:
    void on_frame(const igpp::Frame& frame) override { player::live_frame(frame); }

    void on_packet(Type type, const uint8_t* p, uint16_t len) override {
        switch (type) {
        case Type::Play: {
            const uint16_t idx = len >= 2 ? uint16_t(p[0] | (p[1] << 8)) : 0xFFFF;
            if (idx == 0xFFFF) {
                player::next();
            } else {
                player::play(idx);
            }
            reply_status(type, 0);
            break;
        }
        case Type::Stop:
            player::stop();
            reply_status(type, 0);
            break;
        case Type::Resume:
            player::resume();
            reply_status(type, 0);
            break;
        case Type::Status: {
            StatusReply s = {};
            s.scan_frames = igpp::display::scan_frames();
            s.shown_frames = igpp::display::shown_frames();
            s.rx_frames = parser_ ? parser_->frames() : 0;
            s.rx_dropped = parser_ ? parser_->dropped() : 0;
            s.playlist_size = uint16_t(player::size());
            s.playlist_index = uint16_t(player::index());
            s.mode = uint8_t(player::mode());
            s.storage = uint8_t((storage::sd_mounted() ? 1 : 0) | (storage::flash_mounted() ? 2 : 0));
            reply(type, &s, sizeof(s));
            break;
        }
        case Type::Ping:
            reply(type, p, len);
            break;
        case Type::List: {
            std::string l = player::list();
            if (l.size() > kMaxPayload) l.resize(kMaxPayload);
            reply(type, l.data(), uint16_t(l.size()));
            break;
        }
        case Type::FileOpen: {
            close_file();
            const std::string name(reinterpret_cast<const char*>(p), len);
            const char* root = storage::upload_root();
            if (!root || !valid_name(name)) {
                reply_status(type, 1);
                break;
            }
            path_ = std::string(root) + "/" + name;
            file_ = fopen(path_.c_str(), "wb");
            written_ = 0;
            ESP_LOGI(TAG, "upload %s: %s", path_.c_str(), file_ ? "open" : "failed");
            reply_status(type, file_ ? 0 : 2);
            break;
        }
        case Type::FileData:
            // No reply per chunk: USB flow control throttles the host.
            if (file_ && fwrite(p, 1, len, file_) == len) {
                written_ += len;
            } else if (file_) {
                ESP_LOGE(TAG, "write failed on %s", path_.c_str());
                close_file();
            }
            break;
        case Type::FileClose: {
            const uint32_t size = written_;
            const bool ok = file_ != nullptr;
            close_file();
            ESP_LOGI(TAG, "upload done: %u bytes", unsigned(size));
            uint8_t r[5] = {uint8_t(ok ? 0 : 1), uint8_t(size), uint8_t(size >> 8),
                            uint8_t(size >> 16), uint8_t(size >> 24)};
            reply(type, r, sizeof(r));
            player::rescan();
            break;
        }
        default:
            reply_status(type, 0xFF);
            break;
        }
    }

    void set_parser(StreamParser* p) { parser_ = p; }

private:
    void close_file() {
        if (file_) fclose(file_);
        file_ = nullptr;
    }

    StreamParser* parser_ = nullptr;
    FILE* file_ = nullptr;
    std::string path_;
    uint32_t written_ = 0;
};

Link g_link;
StreamParser g_parser(g_link);

void task(void*) {
    static uint8_t buf[512];
    for (;;) {
        const int n = usb_serial_jtag_read_bytes(buf, sizeof(buf), pdMS_TO_TICKS(5));
        if (n > 0) {
            g_parser.feed(buf, size_t(n), uint32_t(esp_timer_get_time() / 1000));
        }
    }
}

}  // namespace

void start() {
    usb_serial_jtag_driver_config_t cfg = USB_SERIAL_JTAG_DRIVER_CONFIG_DEFAULT();
    cfg.rx_buffer_size = 4096;
    cfg.tx_buffer_size = 2048;
    ESP_ERROR_CHECK(usb_serial_jtag_driver_install(&cfg));
    g_link.set_parser(&g_parser);
    xTaskCreatePinnedToCore(task, "usb_link", 6144, nullptr, 6, nullptr, 0);
    ESP_LOGI(TAG, "USB link ready");
}

uint32_t rx_frames() { return g_parser.frames(); }
uint32_t rx_dropped() { return g_parser.dropped(); }

}  // namespace usb_link
