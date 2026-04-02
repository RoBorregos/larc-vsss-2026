#pragma once
#include <opencv2/core.hpp>
#include <opencv2/core/cuda.hpp>
#include <optional>

#include "app_settings.h"
#include "coordinates.h"
#include "gui.h"
#include "robot_identities.h"

#define DRAW_RADIUS 10
#define DRAW_THICKNESS 2
#define DRAW_FONT_SCALE 0.4
#define DRAW_TEXT_X_OFFSET (-10)
#define DRAW_TEXT_Y_OFFSET (-15)

class Tracker {
private:
    Coordinates* coordinates;
    GUI* gui;
    Drawer* drawer;
    AppData* app_data;
    std::set<int> infield_objects;

    std::chrono::steady_clock::time_point last_prediction_time;
    cv::Point2f fixed_future_ball_pos;
    bool has_prediction = false;

    int MAX_BLIND_FRAMES = 120;
public:
    struct Entity {
        int id = -1;
        cv::Point2f pos{0, 0};
        cv::Point2f vel{0, 0};
        double facing = 0.0;
        bool visible = false;

        const double FACING_ALPHA = 0.6;

        int blind_frames = 0;
        bool initialized = false;

        long long total_frames = 0;
        long long seen_frames = 0;
        float accuracy = 0.0f;

        cv::KalmanFilter kf;
        cv::Mat measurement;

        void init(cv::Point2f start_pos, double start_facing);
        void update(std::optional<cv::Point2f> observed_pos, std::optional<double> observed_facing);
        [[nodiscard]] ObjectType team() const;
    };

    Tracker(Coordinates* coordinates, GUI* gui, Drawer* drawer, AppData* app_data);
    void upload_infield_objects(const std::set<int>& infield_objects);
    Entity ball;
    std::map<int, Entity> robots;


    void update(const std::vector<RobotPatch>& detected_robots, const std::optional<BallPatch>& detected_ball);
    void display_debug_image(int width, int height);
private:
};
