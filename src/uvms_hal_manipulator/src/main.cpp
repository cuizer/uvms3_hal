#include "rclcpp/rclcpp.hpp"
#include "uvms_hal_manipulator/manipulator_lifecycle_node.hpp"

int main(int argc, char** argv)
{
    rclcpp::init(argc, argv);

    auto node = std::make_shared<uvms_hal_manipulator::ManipulatorLifecycleNode>();

    rclcpp::executors::SingleThreadedExecutor executor;
    executor.add_node(node->get_node_base_interface());
    executor.spin();

    rclcpp::shutdown();
    return 0;
}