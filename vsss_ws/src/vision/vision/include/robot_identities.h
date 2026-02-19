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

enum class Team { Yellow, Blue, Ball, None };

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

struct RobotSignature {
    unsigned char parent;
    unsigned char child_1;
    unsigned char child_2;

    constexpr bool operator==(const RobotSignature& other) const {
        return parent == other.parent &&
               child_1 == other.child_1 &&
               child_2 == other.child_2;
    }
};

struct RobotIdentity {
    int id;
    RobotSignature signature;
};

namespace RobotIdentities {
    constexpr size_t TOTAL_ROBOTS = 21;

    constexpr std::array<RobotIdentity, TOTAL_ROBOTS> DATABASE = {{
        {0, {Color_ID::YELLOW, Color_ID::RED,      Color_ID::GREEN}},
        {1, {Color_ID::YELLOW, Color_ID::RED,      Color_ID::CYAN}},
        {2, {Color_ID::YELLOW, Color_ID::GREEN,    Color_ID::RED}},
        {3, {Color_ID::YELLOW, Color_ID::GREEN,    Color_ID::CYAN}},
        {4, {Color_ID::YELLOW, Color_ID::GREEN,    Color_ID::MAGENTA}},
        {5, {Color_ID::YELLOW, Color_ID::CYAN,     Color_ID::RED}},
        {6, {Color_ID::YELLOW, Color_ID::CYAN,     Color_ID::GREEN}},
        {7, {Color_ID::YELLOW, Color_ID::CYAN,     Color_ID::MAGENTA}},
        {8, {Color_ID::YELLOW, Color_ID::MAGENTA,  Color_ID::GREEN}},
        {9, {Color_ID::YELLOW, Color_ID::MAGENTA,  Color_ID::CYAN}},

        {10, {Color_ID::BLUE, Color_ID::RED,      Color_ID::GREEN}},
        {11, {Color_ID::BLUE, Color_ID::RED,      Color_ID::CYAN}},
        {12, {Color_ID::BLUE, Color_ID::GREEN,    Color_ID::RED}},
        {13, {Color_ID::BLUE, Color_ID::GREEN,    Color_ID::CYAN}},
        {14, {Color_ID::BLUE, Color_ID::GREEN,    Color_ID::MAGENTA}},
        {15, {Color_ID::BLUE, Color_ID::CYAN,     Color_ID::RED}},
        {16, {Color_ID::BLUE, Color_ID::CYAN,     Color_ID::GREEN}},
        {17, {Color_ID::BLUE, Color_ID::CYAN,     Color_ID::MAGENTA}},
        {18, {Color_ID::BLUE, Color_ID::MAGENTA,  Color_ID::GREEN}},
        {19, {Color_ID::BLUE, Color_ID::MAGENTA,  Color_ID::CYAN}},

        {20, {Color_ID::ORANGE, Color_ID::NONE, Color_ID::NONE}}
    }};

    inline int get_id(unsigned char p, unsigned char c1, unsigned char c2) {
        RobotSignature query = {p, c1, c2};
        for (const auto& entry : DATABASE) {
            if (entry.signature == query) return entry.id;
        }
        return -1;
    }
}