#include <ros/ros.h>
#include <std_msgs/String.h>

#include <cctype>
#include <iostream>
#include <optional>
#include <string>

#include <unitree/idl/ros2/String_.hpp>
#include <unitree/robot/channel/channel_subscriber.hpp>

namespace {

  constexpr const char* kDefaultDdsTopic = "rt/audio_msg";

  ros::Publisher* g_asr_pub = nullptr;

  std::string TrimAsciiWhitespace(const std::string& input) {
    size_t begin = 0;
    size_t end = input.size();

    while (begin < end && std::isspace(static_cast<unsigned char>(input[begin]))) {
      ++begin;
    }
    while (end > begin && std::isspace(static_cast<unsigned char>(input[end - 1]))) {
      --end;
    }
    return input.substr(begin, end - begin);
  }

  bool EndsWith(const std::string& s, const std::string& suffix) {
    return s.size() >= suffix.size() && s.compare(s.size() - suffix.size(), suffix.size(), suffix) == 0;
  }

  std::string NormalizeAsrText(std::string s) {
    s = TrimAsciiWhitespace(s);
    static const std::string kSuffixes[] = {
      "。", "！", "？", "，", "、", "；", "：",
      ".", "!", "?", ",", ";", ":"
    };
    bool changed = true;
    while (changed) {
      changed = false;
      s = TrimAsciiWhitespace(s);
      for (const auto& suf : kSuffixes) {
        if (EndsWith(s, suf)) {
          s.erase(s.size() - suf.size());
          changed = true;
          break;
        }
      }
    }
    return TrimAsciiWhitespace(s);
  }

  // Minimal JSON string field extractor for payloads like:
  // {"text":"继续导航。", ...}
  // It does not aim to be a full JSON parser; it only extracts a quoted string value.
  std::optional<std::string> ExtractJsonStringField(const std::string& json, const std::string& field) {
    const std::string key = "\"" + field + "\"";
    size_t pos = json.find(key);
    if (pos == std::string::npos) {
      return std::nullopt;
    }
    pos = json.find(':', pos + key.size());
    if (pos == std::string::npos) {
      return std::nullopt;
    }
    ++pos;
    while (pos < json.size() && std::isspace(static_cast<unsigned char>(json[pos]))) {
      ++pos;
    }
    if (pos >= json.size() || json[pos] != '"') {
      return std::nullopt;
    }
    ++pos;
    std::string out;
    out.reserve(32);
    bool escape = false;
    for (; pos < json.size(); ++pos) {
      const char c = json[pos];
      if (escape) {
        switch (c) {
        case '"': out.push_back('"'); break;
        case '\\': out.push_back('\\'); break;
        case '/': out.push_back('/'); break;
        case 'b': out.push_back('\b'); break;
        case 'f': out.push_back('\f'); break;
        case 'n': out.push_back('\n'); break;
        case 'r': out.push_back('\r'); break;
        case 't': out.push_back('\t'); break;
        // NOTE: intentionally not implementing \uXXXX here.
        default: out.push_back(c); break;
        }
        escape = false;
        continue;
      }
      if (c == '\\') {
        escape = true;
        continue;
      }
      if (c == '"') {
        return out;
      }
      out.push_back(c);
    }
    return std::nullopt;
  }

  void AsrHandler(const void* msg) {
    if (!msg || !g_asr_pub) {
      return;
    }

    const auto* dds_msg = static_cast<const std_msgs::msg::dds_::String_*>(msg);
    const std::string payload = dds_msg->data();
    std::cout << "Original Topic:\"rt/audio_msg\" recv: " << payload << std::endl;
    // Original Topic : "rt/audio_msg" recv : {"index":133, "timestamp" : 1779762202365, "type" : 0, "text" : "导航。", "angle" : 0, "speaker_id" : 0, "emotion" : "<|NEUTRAL|>", "confidence" : 0.500000, "language" : "<|zh|>", "is_final" : false}
    const auto extracted_text = ExtractJsonStringField(payload, "text");
    const std::string text_raw = extracted_text ? *extracted_text : payload;
    const std::string text = NormalizeAsrText(text_raw);

    std_msgs::String ros_msg;

    if (text == "继续导航") {
      ROS_INFO("[unitree_asr_voice] ASR command: continue navigation");
    }
    else {
      if (extracted_text) {
        ROS_INFO("[unitree_asr_voice] ASR text: %s", text.c_str());
      }
      else {
        ROS_INFO("[unitree_asr_voice] ASR payload (no text field): %s", text.c_str());
      }
    }
    // ros_msg.data = text;
    // g_asr_pub->publish(ros_msg);

    // ROS_INFO("[unitree_asr_voice] ASR recv: %s" , text.c_str( ));
  }

} // namespace

int main(int argc, char** argv) {
  ros::init(argc, argv, "unitree_asr_voice_node");
  ros::NodeHandle nh;
  ros::NodeHandle pnh("~");
  setlocale(LC_ALL, ""); // For proper UTF-8 handling in console output.
  std::string network_interface = "eth0";
  std::string dds_topic = kDefaultDdsTopic;
  std::string asr_topic = "/unitree/asr";
  int queue_size = 10;

  pnh.param<std::string>("network_interface", network_interface,
    network_interface);
  pnh.param<std::string>("dds_topic", dds_topic, dds_topic);
  pnh.param<std::string>("asr_topic", asr_topic, asr_topic);
  pnh.param<int>("queue_size", queue_size, queue_size);

  ROS_INFO("[unitree_asr_voice] network_interface=%s", network_interface.c_str());
  ROS_INFO("[unitree_asr_voice] dds_topic=%s", dds_topic.c_str());
  ROS_INFO("[unitree_asr_voice] asr_topic=%s", asr_topic.c_str());

  unitree::robot::ChannelFactory::Instance()->Init(0, network_interface.c_str());

  ros::Publisher asr_pub = nh.advertise<std_msgs::String>(asr_topic, queue_size, false);
  g_asr_pub = &asr_pub;

  unitree::robot::ChannelSubscriber<std_msgs::msg::dds_::String_> subscriber(dds_topic);
  subscriber.InitChannel(AsrHandler);

  ROS_INFO("[unitree_asr_voice] node ready.");
  ros::spin();
  return 0;
}
