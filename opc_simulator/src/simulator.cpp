#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <chrono>
#include <random>
#include <atomic>
#include <csignal>
#include <cstdint>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>

namespace beast = boost::beast;
namespace http = beast::http;
namespace asio = boost::asio;
using tcp = asio::ip::tcp;

constexpr uint32_t TOTAL_SLIPS = 5000;
constexpr uint16_t MSI_DEVICE_COUNT = 20;
constexpr uint16_t MC_DEVICE_COUNT = 30;
constexpr uint16_t MSI_SLIPS_PER_DEVICE = 250;
constexpr uint16_t MC_SLIPS_PER_DEVICE = 167;

static std::atomic<bool> g_running(true);

void signalHandler(int signal) {
    std::cout << "\nReceived signal " << signal << ", shutting down..." << std::endl;
    g_running = false;
}

uint64_t now_s() {
    return (uint64_t)std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

std::string httpPostJson(const std::string& host, uint16_t port,
                         const std::string& path, const std::string& json_body) {
    try {
        asio::io_context ioc;
        tcp::resolver resolver(ioc);
        beast::tcp_stream stream(ioc);

        auto const results = resolver.resolve(host, std::to_string(port));
        stream.connect(results);

        http::request<http::string_body> req{http::verb::post, path, 11};
        req.set(http::field::host, host);
        req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
        req.set(http::field::content_type, "application/json");
        req.body() = json_body;
        req.prepare_payload();

        http::write(stream, req);

        beast::flat_buffer buffer;
        http::response<http::string_body> res;
        http::read(stream, buffer, res);

        beast::error_code ec;
        stream.socket().shutdown(tcp::socket::shutdown_both, ec);

        return res.body();
    } catch (std::exception const& e) {
        std::cerr << "HTTP POST error: " << e.what() << std::endl;
        return "";
    }
}

void generateSpectralData(uint16_t device_id, std::ostringstream& json) {
    static thread_local std::mt19937 gen(std::random_device{}());
    static std::uniform_real_distribution<float> temp_dist(18.0f, 26.0f);
    static std::uniform_real_distribution<float> hum_dist(50.0f, 70.0f);
    static std::uniform_real_distribution<float> light_dist(40.0f, 60.0f);
    static std::normal_distribution<float> noise(0.0f, 0.02f);

    uint64_t timestamp = now_s();
    uint32_t start_slip = (device_id - 1) * MSI_SLIPS_PER_DEVICE + 1;
    uint32_t end_slip = std::min(device_id * MSI_SLIPS_PER_DEVICE, (uint32_t)TOTAL_SLIPS);

    float temperature = temp_dist(gen);
    float humidity = hum_dist(gen);
    float light_intensity = light_dist(gen);

    std::vector<uint16_t> wavelengths = {400, 450, 500, 550, 600, 650, 700};

    bool first = true;
    for (uint32_t slip_id = start_slip; slip_id <= end_slip; ++slip_id) {
        float base_reflectance = 0.7f - 0.0001f * (timestamp / 3600.0f);

        for (uint16_t wavelength : wavelengths) {
            float wavelength_factor = 1.0f - 0.3f * (wavelength - 400) / 300.0f;
            float reflectance = base_reflectance * wavelength_factor + noise(gen);
            reflectance = std::max(0.1f, std::min(0.95f, reflectance));

            if (!first) json << ",";
            json << "{"
                 << "\"timestamp\":" << timestamp << ","
                 << "\"device_id\":" << device_id << ","
                 << "\"slip_id\":" << slip_id << ","
                 << "\"wavelength\":" << wavelength << ","
                 << "\"reflectance\":" << std::fixed << std::setprecision(6) << reflectance << ","
                 << "\"temperature\":" << std::fixed << std::setprecision(2) << (temperature + noise(gen) * 10) << ","
                 << "\"humidity\":" << std::fixed << std::setprecision(2) << (humidity + noise(gen) * 10) << ","
                 << "\"light_intensity\":" << std::fixed << std::setprecision(2) << (light_intensity + noise(gen) * 10)
                 << "}";
            first = false;
        }
    }
}

void generateMicrobialData(uint16_t device_id, std::ostringstream& json) {
    static thread_local std::mt19937 gen(std::random_device{}());
    static std::uniform_real_distribution<float> temp_dist(18.0f, 26.0f);
    static std::uniform_real_distribution<float> hum_dist(50.0f, 70.0f);
    static std::lognormal_distribution<float> fungi_dist(3.0f, 0.5f);
    static std::lognormal_distribution<float> bacteria_dist(2.5f, 0.4f);

    uint64_t timestamp = now_s();
    uint32_t start_slip = (device_id - 101) * MC_SLIPS_PER_DEVICE + 1;
    uint32_t end_slip = std::min((uint32_t)((device_id - 100) * MC_SLIPS_PER_DEVICE), (uint32_t)TOTAL_SLIPS);

    float temperature = temp_dist(gen);
    float humidity = hum_dist(gen);

    bool first = true;
    for (uint32_t slip_id = start_slip; slip_id <= end_slip && slip_id <= TOTAL_SLIPS; ++slip_id) {
        float T = std::clamp(temperature, 0.0f, 40.0f);
        float RH = std::clamp(humidity, 30.0f, 95.0f);

        float log_cfu = -2.5f + 0.12f * T + 0.08f * RH
                        - 0.0015f * T * T - 0.0008f * RH * RH
                        + 0.0005f * T * RH;

        float base_fungi = std::exp(log_cfu) * 0.3f;
        float base_bacteria = base_fungi * 0.8f;

        float fungi = std::max(5.0f, base_fungi + fungi_dist(gen) * 0.5f);
        float bacteria = std::max(3.0f, base_bacteria + bacteria_dist(gen) * 0.5f);

        if (slip_id % 333 == 0 && fungi < 100.0f) {
            fungi = 120.0f + fungi_dist(gen) * 10;
        }

        if (!first) json << ",";
        json << "{"
             << "\"timestamp\":" << timestamp << ","
             << "\"device_id\":" << device_id << ","
             << "\"slip_id\":" << slip_id << ","
             << "\"fungi_concentration\":" << std::fixed << std::setprecision(2) << fungi << ","
             << "\"bacteria_concentration\":" << std::fixed << std::setprecision(2) << bacteria << ","
             << "\"temperature\":" << std::fixed << std::setprecision(2) << temperature << ","
             << "\"humidity\":" << std::fixed << std::setprecision(2) << humidity
             << "}";
        first = false;
    }
}

void sendSpectralData(const std::string& host, uint16_t port) {
    std::ostringstream json;
    json << "[";

    for (uint16_t device_id = 1; device_id <= MSI_DEVICE_COUNT; ++device_id) {
        generateSpectralData(device_id, json);
    }

    json << "]";

    std::string response = httpPostJson(host, port, "/api/v1/ingest/spectral", json.str());
    if (!response.empty()) {
        std::cout << "[" << now_s() << "] Spectral data sent successfully" << std::endl;
    }
}

void sendMicrobialData(const std::string& host, uint16_t port) {
    std::ostringstream json;
    json << "[";

    for (uint16_t device_id = 101; device_id < 101 + MC_DEVICE_COUNT; ++device_id) {
        generateMicrobialData(device_id, json);
    }

    json << "]";

    std::string response = httpPostJson(host, port, "/api/v1/ingest/microbial", json.str());
    if (!response.empty()) {
        std::cout << "[" << now_s() << "] Microbial data sent successfully" << std::endl;
    }
}

int main(int argc, char* argv[]) {
    std::string server_host = "127.0.0.1";
    uint16_t server_port = 8080;
    uint32_t interval = 300;
    bool single_shot = false;

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--host" && i + 1 < argc) {
            server_host = argv[++i];
        } else if (arg == "--port" && i + 1 < argc) {
            server_port = std::stoi(argv[++i]);
        } else if (arg == "--interval" && i + 1 < argc) {
            interval = std::stoi(argv[++i]);
        } else if (arg == "--once") {
            single_shot = true;
        } else if (arg == "--help") {
            std::cout << "Usage: " << argv[0] << " [options]\n"
                      << "Options:\n"
                      << "  --host <host>      Server host (default: 127.0.0.1)\n"
                      << "  --port <port>      Server port (default: 8080)\n"
                      << "  --interval <s>     Report interval in seconds (default: 300)\n"
                      << "  --once             Send data once and exit\n"
                      << "  --help             Show this help\n";
            return 0;
        }
    }

    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    std::cout << "============================================\n";
    std::cout << "海昏侯简牍监测系统 - OPC UA 模拟器\n";
    std::cout << "============================================\n";
    std::cout << "Server: " << server_host << ":" << server_port << "\n";
    std::cout << "Interval: " << interval << "s\n";
    std::cout << "MSI Devices: " << MSI_DEVICE_COUNT << "\n";
    std::cout << "MC Devices: " << MC_DEVICE_COUNT << "\n";
    std::cout << "Total Slips: " << TOTAL_SLIPS << "\n";
    std::cout << "============================================\n\n";

    if (single_shot) {
        std::cout << "Sending single batch of data...\n";
        sendSpectralData(server_host, server_port);
        sendMicrobialData(server_host, server_port);
        std::cout << "Done.\n";
        return 0;
    }

    std::cout << "Simulator running. Press Ctrl+C to stop.\n\n";

    while (g_running) {
        try {
            sendSpectralData(server_host, server_port);
            sendMicrobialData(server_host, server_port);
        } catch (std::exception const& e) {
            std::cerr << "Error: " << e.what() << std::endl;
        }

        uint32_t sleep_interval = std::min(interval, 10u);
        for (uint32_t i = 0; i < interval && g_running; i += sleep_interval) {
            std::this_thread::sleep_for(std::chrono::seconds(sleep_interval));
        }
    }

    std::cout << "\nSimulator stopped.\n";
    return 0;
}
