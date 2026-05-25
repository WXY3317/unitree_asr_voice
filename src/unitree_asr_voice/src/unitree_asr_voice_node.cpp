#include <ros/ros.h>
#include <std_msgs/String.h>

#include <unitree/idl/ros2/String_.hpp>
#include <unitree/robot/channel/channel_subscriber.hpp>

namespace {

  constexpr const char* kDefaultDdsTopic = "rt/audio_msg";

  ros::Publisher* g_asr_pub = nullptr;

  void AsrHandler(const void* msg) {
    if ( !msg || !g_asr_pub ) {
      return;
    }

    const auto* dds_msg = static_cast<const std_msgs::msg::dds_::String_*>(msg);
    const std::string text = dds_msg->data( );

    std_msgs::String ros_msg;
    ros_msg.data = text;
    g_asr_pub->publish(ros_msg);

    ROS_INFO("[unitree_asr_voice] ASR recv: %s" , text.c_str( ));
  }

} // namespace

int main(int argc , char** argv) {
  ros::init(argc , argv , "unitree_asr_voice_node");
  ros::NodeHandle nh;
  ros::NodeHandle pnh("~");
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

  ros::Publisher asr_pub = nh.advertise<std_msgs::String>(asr_topic , queue_size , false);
  g_asr_pub = &asr_pub;

  unitree::robot::ChannelSubscriber<std_msgs::msg::dds_::String_> subscriber(dds_topic);
  subscriber.InitChannel(AsrHandler);

  ROS_INFO("[unitree_asr_voice] node ready.");

  ros::spin( );
  return 0;
}
