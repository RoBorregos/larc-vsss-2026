#pragma once
#include <array>

namespace Color_ID {
    constexpr unsigned char NONE    = 0;
    constexpr unsigned char BLUE    = 1;
    constexpr unsigned char YELLOW  = 2;
    constexpr unsigned char CYAN    = 3;
    constexpr unsigned char GREEN   = 4;
    constexpr unsigned char MAGENTA = 5;
    constexpr unsigned char RED     = 6;
    constexpr unsigned char ORANGE  = 7;
}

enum class ObjectType { Yellow, Blue, Ball, None };

struct Patch {
    int id;
    int color;
    cv::Point2d centroid;
    bool parent;
    cv::Rect bounding_box{};
    bool valid = true;
};

struct BallPatch {
    int id = 20; // Constant for ball (see documentation)
    std::shared_ptr<Patch> patch;
    cv::Point2d center{};
};

struct RobotPatch {
    int id = -1;
    std::vector<std::shared_ptr<Patch>> patches;

    std::shared_ptr<Patch> parent;
    std::vector<std::shared_ptr<Patch>> children;
    cv::Point2d center{};
    double facing{};
};

struct RobotIdentity {
    int id;
    int aruco_id;
};

namespace RobotIdentities {
    constexpr size_t TOTAL_ROBOTS = 21;

    constexpr std::array<RobotIdentity, TOTAL_ROBOTS> DATABASE = {{
        {0, 955},
        {1, 771},
        {2, 939},

        {10, 256},
        {11, 272},
        {12, 273},

        {20, -1}
    }};

    inline int get_robot_id_by_aruco(int aruco_id) {
        for (const auto& entry : DATABASE) {
            if (entry.aruco_id == aruco_id) {
                return entry.id;
            }
        }
        return -1;
    }
}