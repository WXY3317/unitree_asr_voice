#include <ros/ros.h>
#include <std_msgs/String.h>

#include <cctype>
#include <iostream>
#include <optional>
#include <string>

#include <nlohmann/json.hpp>

#include <unitree/idl/ros2/String_.hpp>
#include <unitree/robot/channel/channel_subscriber.hpp>

namespace {

  constexpr const char* kDefaultDdsTopic = "rt/audio_msg";

  // ros::Publisher* g_asr_pub = nullptr;

  std::string TrimAsciiWhitespace(const std::string& input) {
    size_t begin = 0;
    size_t end = input.size( );

    while ( begin < end && std::isspace(static_cast<unsigned char>(input[begin])) ) {
      ++begin;
    }
    while ( end > begin && std::isspace(static_cast<unsigned char>(input[end - 1])) ) {
      --end;
    }
    return input.substr(begin , end - begin);
  }

  bool EndsWith(const std::string& s , const std::string& suffix) {
    return s.size( ) >= suffix.size( ) && s.compare(s.size( ) - suffix.size( ) , suffix.size( ) , suffix) == 0;
  }


  /**
   * NormalizeAsrText
   * ---------------------------------
   * 功能：对从 ASR（语音识别）得到的文本做简单规范化，主要用于去除
   *      - 首尾的 ASCII 空白（空格、制表、换行等），
   *      - 末尾重复出现的句末标点（中/英文标点，如 '。', '.', '!' 等）。
   */
  std::string NormalizeAsrText(std::string s) {
    // 移除首尾 ASCII 空白
    s = TrimAsciiWhitespace(s);
    static const std::string kSuffixes [ ] = {
      "。", "！", "？", "，", "、", "；", "：",
      ".", "!", "?", ",", ";", ":"
    };

    while ( true ) {
      bool removed = false;
      for ( const auto& suf : kSuffixes ) {
        if ( EndsWith(s , suf) ) {
          s.erase(s.size( ) - suf.size( ));
          removed = true;
          break;
        }
      }
      if ( !removed ) break; // 没有删除，结束循环
      // 删除后可能出现尾部空格，重新去除
      s = TrimAsciiWhitespace(s);
    }
    return s;
  }

  // Use nlohmann::json to extract the "text" field when possible.

  void AsrHandler(const void* msg) {
    if ( !msg ) {
      return;
    }

    const auto* dds_msg = static_cast<const std_msgs::msg::dds_::String_*>(msg);
    const std::string payload = dds_msg->data( );
    std::cout << "Original Topic:\"rt/audio_msg\" recv: " << payload << std::endl;
    // Original Topic : "rt/audio_msg" recv : {"index":133, "timestamp" : 1779762202365, "type" : 0, "text" : "导航。", "angle" : 0, "speaker_id" : 0, "emotion" : "<|NEUTRAL|>", "confidence" : 0.500000, "language" : "<|zh|>", "is_final" : false}

    //提取"text"字段，如果解析失败或"text"字段不存在，则使用原始payload进行处理。
    std::optional<std::string> extracted_text;
    try {
      auto jsonPayload = nlohmann::json::parse(payload);
      if ( jsonPayload.contains("text") && jsonPayload["text"].is_string( ) ) {
        extracted_text = jsonPayload["text"].get<std::string>( );
      }
    }
    catch ( const std::exception& e ) {
   // parse failed; leave extracted_text empty and fall back to payload
    }

    std::string text;
    if ( extracted_text ) {
      text = NormalizeAsrText(*extracted_text);
    }
    else {
      text = TrimAsciiWhitespace(payload);
    }

    std_msgs::String ros_msg;

    if ( text == "继续导航" ) {
      ROS_INFO("[unitree_asr_voice] ASR command: continue navigation");
    }
    else {
      if ( extracted_text ) {
        ROS_INFO("[unitree_asr_voice] ASR text: %s" , text.c_str( ));
      }
      else {
        ROS_INFO("[unitree_asr_voice] ASR payload (no text field): %s" , text.c_str( ));
      }
    }
  }

} // namespace

int main(int argc , char** argv) {
  ros::init(argc , argv , "unitree_asr_voice_node");
  ros::NodeHandle nh;
  ros::NodeHandle pnh("~");
  setlocale(LC_ALL , ""); // For proper UTF-8 handling in console output.
  std::string network_interface = "eth0";
  std::string dds_topic = kDefaultDdsTopic;
  std::string asr_topic = "/unitree/asr";
  int queue_size = 10;

  pnh.param<std::string>("network_interface" , network_interface ,
    network_interface);
  pnh.param<std::string>("dds_topic" , dds_topic , dds_topic);
  pnh.param<std::string>("asr_topic" , asr_topic , asr_topic);
  pnh.param<int>("queue_size" , queue_size , queue_size);

  ROS_INFO("[unitree_asr_voice] network_interface=%s" , network_interface.c_str( ));
  ROS_INFO("[unitree_asr_voice] dds_topic=%s" , dds_topic.c_str( ));
  ROS_INFO("[unitree_asr_voice] asr_topic=%s" , asr_topic.c_str( ));

  unitree::robot::ChannelFactory::Instance( )->Init(0 , network_interface.c_str( ));

  // ros::Publisher asr_pub = nh.advertise<std_msgs::String>(asr_topic , queue_size , false);
  // g_asr_pub = &asr_pub;

  unitree::robot::ChannelSubscriber<std_msgs::msg::dds_::String_> subscriber(dds_topic);
  subscriber.InitChannel(AsrHandler);

  ROS_INFO("[unitree_asr_voice] node ready.");
  ros::spin( );
  return 0;
}
