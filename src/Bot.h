#pragma once
#include <string>
#include <vector>
#include <set>
#include <map>
#include <mutex>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

struct Settings {
    std::string prompt_language;
    bool use_google_ai;
    bool enable_read_message;
    int read_delay;
    int delay_interval;
    bool use_slow_mode;
    bool use_reply;
    int delete_bot_reply;
    bool delete_immediately;
};

class Bot {
public:
    Bot(std::string token, std::string api_key);
    void run(const std::string& channel_id, Settings settings);

private:
    std::string m_token;
    std::string m_bot_user_id;
    std::string m_google_api_key;
    
    std::set<std::string> m_processed_message_ids;
    std::string m_last_generated_text;
    std::map<std::string, int> m_bot_question_counts;
    
    std::mutex m_processed_ids_mutex;

    // Fungsi internal
    bool get_bot_info();
    void auto_reply(const std::string& channel_id, Settings settings);
    std::string generate_reply(const std::string& prompt, const std::string& lang, bool use_ai, const std::string& author_id, bool is_quiz);
    std::string get_random_message_from_file();
    void send_message(const std::string& channel_id, const std::string& text, const std::string& reply_to_id, const std::string& tag_user_id, const Settings& s);
    void delete_message(const std::string& channel_id, const std::string& message_id, int delay);
    json get_channel_info_raw(const std::string& channel_id);
    std::vector<std::string> get_channel_members(const std::string& channel_id);
};
