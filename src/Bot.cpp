#include "Bot.h"
#include "Logger.h"
#include <cpr/cpr.h>
#include <thread>
#include <chrono>
#include <random>
#include <fstream>
#include <regex>

Bot::Bot(std::string token, std::string api_key)
    : m_token(std::move(token)), m_google_api_key(std::move(api_key)) {
    get_bot_info();
}

// Implementasi fungsi lainnya dari Bot.h sama seperti sebelumnya,
// dengan pengecualian utama pada generate_reply dan penghapusan get_random_api_key.
// Berikut adalah versi lengkap dan terkoreksi.

bool Bot::get_bot_info() {
    cpr::Response r = cpr::Get(cpr::Url{"https://discord.com/api/v9/users/@me"},
                               cpr::Header{{"Authorization", m_token}});
    if (r.status_code == 200) {
        json data = json::parse(r.text);
        m_bot_user_id = data["id"];
        std::string username = data["username"];
        std::string discriminator = data["discriminator"];
        log_message("Bot Account Loaded: " + username + "#" + discriminator + " (ID: " + m_bot_user_id + ")", LogLevel::SUCCESS);
        return true;
    }
    log_message("Failed to get bot info. Token might be invalid.", LogLevel::ERROR);
    return false;
}

void Bot::run(const std::string& channel_id, Settings settings) {
    if (m_bot_user_id.empty()) {
        log_message("Bot cannot run without a valid user ID.", LogLevel::ERROR);
        return;
    }
    auto_reply(channel_id, settings);
}

void Bot::auto_reply(const std::string& channel_id, Settings settings) {
    while (true) {
        if (settings.use_google_ai) {
            log_message("[Channel " + channel_id + "] Waiting " + std::to_string(settings.read_delay) + "s before reading...", LogLevel::WAIT);
            std::this_thread::sleep_for(std::chrono::seconds(settings.read_delay));

            cpr::Response r = cpr::Get(cpr::Url{"https://discord.com/api/v9/channels/" + channel_id + "/messages?limit=2"},
                                       cpr::Header{{"Authorization", m_token}});

            if (r.status_code == 200) {
                json messages = json::parse(r.text);
                if (!messages.empty()) {
                    json latest_msg = messages[0];
                    std::string msg_id = latest_msg["id"];
                    
                    std::lock_guard<std::mutex> lock(m_processed_ids_mutex);
                    if (m_processed_message_ids.count(msg_id)) {
                        // Skip if message already processed to avoid spamming
                    } else {
                         std::string author_id = latest_msg["author"]["id"];
                        if (author_id != m_bot_user_id) {
                            std::string content = latest_msg.value("content", "");
                            std::regex quiz_pattern(R"(^\s*[\U0001F300-\U0001F5FF\U0001F600-\U0001F64F\U0001F680-\U0001F6FF\U0001F900-\U0001F9FF].*(which animal|what animal|guess the animal|what is this).*)", std::regex_constants::icase);
                            bool is_quiz = std::regex_search(content, quiz_pattern);
                            
                            log_message("[Channel " + channel_id + "] Received: " + content, LogLevel::INFO);
                            std::string reply_text = generate_reply(content, settings.prompt_language, true, author_id, is_quiz);

                            if (!reply_text.empty()) {
                                 std::vector<std::string> members = get_channel_members(channel_id);
                                 std::string tag_user_id = "";
                                 if(!members.empty()){
                                     std::random_device rd;
                                     std::mt19937 gen(rd());
                                     std::uniform_int_distribution<> distrib(0, members.size() - 1);
                                     tag_user_id = members[distrib(gen)];
                                 }
                                send_message(channel_id, reply_text, settings.use_reply ? msg_id : "", tag_user_id, settings);
                                m_processed_message_ids.insert(msg_id);
                            }
                        }
                    }
                }
            } else {
                log_message("[Channel " + channel_id + "] Failed to fetch messages: " + std::to_string(r.status_code), LogLevel::ERROR);
            }
        } else {
            log_message("[Channel " + channel_id + "] Waiting " + std::to_string(settings.delay_interval) + "s before sending from file...", LogLevel::WAIT);
            std::this_thread::sleep_for(std::chrono::seconds(settings.delay_interval));
            std::string message_text = get_random_message_from_file();
            send_message(channel_id, message_text, "", "", settings);
        }
        log_message("[Channel " + channel_id + "] Waiting " + std::to_string(settings.delay_interval) + "s for next cycle...", LogLevel::WAIT);
        std::this_thread::sleep_for(std::chrono::seconds(settings.delay_interval));
    }
}

std::string Bot::generate_reply(const std::string& prompt, const std::string& lang, bool use_ai, const std::string& author_id, bool is_quiz) {
    std::regex bot_pattern(R"(\b(bot|ai|artificial intelligence|gemini|robot)\b)", std::regex_constants::icase);
    if (std::regex_search(prompt, bot_pattern)) {
        m_bot_question_counts[author_id]++;
        if (m_bot_question_counts[author_id] == 1) return "Haha, I'm just here chilling like anyone else! What's good? 😎";
        if (m_bot_question_counts[author_id] <= 3) return "Yo, I'm just a regular guy vibing, let's talk something fun! 😏";
        return "Bruh, stop it already! I'm no bot, just a human having a blast! 😤🙄 #Chill";
    }
    if (is_quiz) { return "Yo, that's an animal! 🫶 Nailed it! What's the next one? 😄"; }
    if (!use_ai) return get_random_message_from_file();

    if (m_google_api_key.empty()) {
        log_message("No API key provided for this bot instance.", LogLevel::ERROR);
        return "";
    }

    std::string lang_prompt = (lang == "in") ? "Reply to the following message in Indian English: " + prompt : "Reply to the following message in English: " + prompt;
    json payload = { {"contents", {{ {"parts", {{ {"text", lang_prompt + "\n\nMake it one sentence using casual human language."} }}} }}} };
    std::string url = "https://generativelanguage.googleapis.com/v1beta/models/gemini-1.5-flash-latest:generateContent?key=" + m_google_api_key;
    
    cpr::Response r = cpr::Post(cpr::Url{url}, cpr::Header{{"Content-Type", "application/json"}}, cpr::Body{payload.dump()});

    if (r.status_code == 200) {
        json response_json = json::parse(r.text);
        if (response_json.contains("candidates") && !response_json["candidates"].empty()) {
             return response_json["candidates"][0]["content"]["parts"][0]["text"];
        }
        return "AI response format is unexpected.";
    } else if (r.status_code == 429) {
        log_message("API key for this account has hit the rate limit. Waiting for the next cycle.", LogLevel::WARNING);
        return "";
    }
    log_message("Failed to generate reply from AI: " + r.text, LogLevel::ERROR);
    return "";
}

std::string Bot::get_random_message_from_file() { /* ... sama seperti sebelumnya ... */ }
void Bot::send_message(const std::string& channel_id, const std::string& text, const std::string& reply_to_id, const std::string& tag_user_id, const Settings& s) { /* ... sama seperti sebelumnya ... */ }
void Bot::delete_message(const std::string& channel_id, const std::string& message_id, int delay) { /* ... sama seperti sebelumnya ... */ }
json Bot::get_channel_info_raw(const std::string& channel_id){ /* ... sama seperti sebelumnya ... */ }
std::vector<std::string> Bot::get_channel_members(const std::string& channel_id){ /* ... sama seperti sebelumnya ... */ }
