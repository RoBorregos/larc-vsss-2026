#include "drawer.h"

#include <utility>

class PlotCmd final : public DrawCommand {
    cv::Point p; cv::Scalar col;
public:
    PlotCmd(cv::Point p, cv::Scalar c) : p(p), col(std::move(c)) {}
    void draw(cv::Mat& target) override { cv::circle(target, p, 1, col, -1); }
};

class LineCmd final : public DrawCommand {
    cv::Point p1, p2; cv::Scalar col; int thick;
public:
    LineCmd(cv::Point a, cv::Point b, int t, cv::Scalar c) : p1(a), p2(b), thick(t), col(std::move(c)) {}
    void draw(cv::Mat& target) override { cv::line(target, p1, p2, col, thick); }
};

class CircleCmd final : public DrawCommand {
    cv::Point center; int radius, thick; cv::Scalar col;
public:
    CircleCmd(cv::Point c, int r, int t, cv::Scalar col) : center(c), radius(r), thick(t), col(std::move(col)) {}
    void draw(cv::Mat& target) override { cv::circle(target, center, radius, col, thick); }
};

class PolylineCmd final : public DrawCommand {
    std::vector<cv::Point> pts; bool closed; int thick; cv::Scalar col;
public:
    PolylineCmd(std::vector<cv::Point> p, bool c, int t, cv::Scalar col) : pts(std::move(p)), closed(c), thick(t), col(std::move(col)) {}
    void draw(cv::Mat& target) override {
        std::vector<std::vector<cv::Point>> cnts = { pts };
        cv::polylines(target, cnts, closed, col, thick);
    }
};

class InversePolyCmd : public DrawCommand {
    std::vector<cv::Point> pts;
    cv::Scalar col;
    cv::Rect limit_roi;
public:
    InversePolyCmd(const std::vector<cv::Point>& p, cv::Scalar c, cv::Rect r)
        : pts(p), col(std::move(c)), limit_roi(r) {}

    void draw(cv::Mat& target) override {
        if (pts.size() <= 2) return;

        cv::Rect safe_roi = limit_roi & cv::Rect(0, 0, target.cols, target.rows);
        if (safe_roi.empty()) return;

        cv::Mat image_area = target(safe_roi);

        cv::Mat mask(image_area.size(), CV_8UC1, cv::Scalar(255));

        std::vector<cv::Point> offset_pts;
        for (const auto& p : pts) {
            offset_pts.push_back(p - safe_roi.tl());
        }

        std::vector<std::vector<cv::Point>> contours = { offset_pts };
        cv::fillPoly(mask, contours, cv::Scalar(0));

        image_area.setTo(col, mask);
    }
};

class TextCmd final : public DrawCommand {
    std::string txt; cv::Rect roi; float scale; int y_off; bool erase;
public:
    TextCmd(std::string t, cv::Rect r, float s, int y, bool e) : txt(std::move(t)), roi(r), scale(s), y_off(y), erase(e) {}
    void draw(cv::Mat& target) override {
        if (erase) cv::rectangle(target, roi & cv::Rect(0,0,target.cols,target.rows), cv::Scalar(0,0,0), -1);

        int baseline = 0;
        double cur_scale = scale;
        cv::Size sz = cv::getTextSize(txt, cv::FONT_HERSHEY_SIMPLEX, cur_scale, 1, &baseline);
        while (sz.width > roi.width && cur_scale > 0.3) {
            cur_scale -= 0.1;
            sz = cv::getTextSize(txt, cv::FONT_HERSHEY_SIMPLEX, cur_scale, 1, &baseline);
        }
        cv::Point org(roi.x + (roi.width - sz.width)/2, roi.y + (roi.height + sz.height)/2 + y_off);
        cv::putText(target, txt, org, cv::FONT_HERSHEY_SIMPLEX, cur_scale, cv::Scalar(255,255,255), 1);
    }
};

void Drawer::plot(const cv::Point& p, const cv::Scalar& color) {
    command_queue.push_back(std::make_unique<PlotCmd>(p, color));
}

void Drawer::line(const cv::Point& p1, const cv::Point& p2, int thickness, const cv::Scalar& color) {
    command_queue.push_back(std::make_unique<LineCmd>(p1, p2, thickness, color));
}

void Drawer::circle(const cv::Point& center, int radius, int thickness, const cv::Scalar& color) {
    command_queue.push_back(std::make_unique<CircleCmd>(center, radius, thickness, color));
}

void Drawer::polyline(const std::vector<cv::Point>& points, bool is_closed, int thickness, const cv::Scalar& color) {
    command_queue.push_back(std::make_unique<PolylineCmd>(points, is_closed, thickness, color));
}

void Drawer::inverse_polyline(const std::vector<cv::Point>& points, const cv::Scalar& color, const cv::Rect& restriction_roi) {
    command_queue.push_back(std::make_unique<InversePolyCmd>(points, color, restriction_roi));
}

void Drawer::text(const std::string& content, const cv::Rect& roi, float font_scale, int y_offset, bool erase_bg) {
    command_queue.push_back(std::make_unique<TextCmd>(content, roi, font_scale, y_offset, erase_bg));
}

void Drawer::render(cv::Mat& target) {
    for (const auto& cmd : command_queue) {
        cmd->draw(target);
    }
    command_queue.clear();
}

void Drawer::clear() {
    command_queue.clear();
}