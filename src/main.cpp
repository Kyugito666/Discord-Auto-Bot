#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <dotenv.h>
#include <limits>
#include "Bot.h"
#include "Logger.h"

std::vector<std::string> split(const std::string& s, char delimiter) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(s);
    while (std::getline(tokenStream, token, delimiter)) {
        if (!token.empty()) tokens.push_back(token);
    }
    return tokens;
}

void print_banner() {
    std::cout << Color::CYAN << "╔════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║          DISCORD AI AUTO REPLIER BOT 🤖 (C++)        ║" << std::endl;
    std::cout << "║     Automate replies using Google AI & Discord     ║" << std::endl;
    std::cout << "║    Developed by: Kyugito666                          ║" << std::endl;
    std::cout << "║    Based on the Python bot by: Kazuha787           ║" << std::endl;
    std::cout << "╚════════════════════════════════════════════════════╝" << Color::RESET << std::endl;
}

Settings get_user_settings(const std::string& channel_id) {
    Settings s; char choice;
    std::cout << "\nEnter settings for channel " << channel_id << ":" << std::endl;
    std::cout << "  Use Google Gemini AI? (y/n): "; std::cin >> choice;
    s.use_google_ai = (choice == 'y' || choice == 'Y');

    if (s.use_google_ai) {
        std::cout << "  Choose prompt language (en/in): "; std::cin >> s.prompt_language;
        s.enable_read_message = true;
        std::cout << "  Enter message read delay (seconds): "; std::cin >> s.read_delay;
    } else { s.prompt_language = "in"; s.enable_read_message = false; s.read_delay = 0; }
    
    std::cout << "  Enter interval (seconds) for each iteration: "; std::cin >> s.delay_interval;
    std::cout << "  Send messages as replies? (y/n): "; std::cin >> choice;
    s.use_reply = (choice == 'y' || choice == 'Y');
    std::cout << "  Delete bot replies? (y/n): "; std::cin >> choice;
    if (choice == 'y' || choice == 'Y') {
        std::cout << "  After how many seconds? (0 to delete immediately): "; std::cin >> s.delete_bot_reply;
        s.delete_immediately = (s.delete_bot_reply == 0);
    } else { s.delete_bot_reply = -1; s.delete_immediately = false; }
    s.use_slow_mode = false;
    
    // PENTING: Membersihkan buffer input untuk mencegah masalah dengan getline berikutnya
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    return s;
}

int main() {
    dotenv::init();
    print_banner();

    std::string discord_tokens_str = std::getenv("DISCORD_TOKENS") ? std::getenv("DISCORD_TOKENS") : "";
    std::string google_keys_str = std::getenv("GOOGLE_API_KEYS") ? std::getenv("GOOGLE_API_KEYS") : "";

    auto discord_tokens = split(discord_tokens_str, ',');
    auto google_api_keys = split(google_keys_str, ',');

    if (discord_tokens.empty()) { /* ... validasi sama seperti sebelumnya ... */ return 1; }
    if (google_api_keys.empty()) { /* ... validasi sama seperti sebelumnya ... */ return 1; }
    if (discord_tokens.size() != google_api_keys.size()) {
        log_message("Error: The number of Discord tokens must match the number of Google API keys!", LogLevel::ERROR);
        return 1;
    }
    log_message(std::to_string(discord_tokens.size()) + " Discord token(s) and matching API key(s) loaded.", LogLevel::SUCCESS);

    std::cout << "Enter channel IDs (separate with commas): ";
    std::string channel_ids_str;
    std::getline(std::cin, channel_ids_str);
    auto channel_ids = split(channel_ids_str, ',');

    std::vector<std::thread> threads;
    std::map<std::string, Settings> all_settings;
    for (const auto& id : channel_ids) {
        all_settings[id] = get_user_settings(id);
    }

    int token_index = 0;
    for (const auto& id : channel_ids) {
        if (token_index >= discord_tokens.size()) {
            log_message("Warning: More channels than available accounts. Re-using accounts.", LogLevel::WARNING);
        }
        std::string token = discord_tokens[token_index % discord_tokens.size()];
        std::string api_key = google_api_keys[token_index % google_api_keys.size()];
        Bot bot_instance(token, api_key);
        
        threads.emplace_back([bot = std::move(bot_instance), id, settings = all_settings[id]]() mutable {
            bot.run(id, settings);
        });
        token_index++;
    }

    log_message("All bots are running... Press CTRL+C to stop.", LogLevel::INFO);
    for (auto& t : threads) { if (t.joinable()) { t.join(); } }
    return 0;
}
