#pragma once

namespace FieldSizes {
	constexpr double LINE_THICKNESS = 0.01;
	constexpr double HALF_LENGTH = 0.75;
	constexpr double HALF_WIDTH  = 0.65;

	constexpr double GOAL_AREA_DEPTH = 0.15;
	constexpr double GOAL_AREA_X     = HALF_LENGTH - GOAL_AREA_DEPTH;
	constexpr double GOAL_AREA_HALF_WIDTH = 0.35;

	constexpr double CROSS_X_POS = 0.375;
	constexpr double CROSS_Y_POS = 0.4;
	constexpr double CENTER_CROSS_X = 0.375;
	constexpr double CROSS_SIZE = 0.02;

	constexpr double CENTER_CIRCLE_RADIUS = 0.2;
	constexpr double PENALTY_ARC_RADIUS = 0.125;
	constexpr double DOT_RADIUS = 0.02;
	constexpr double PENALTY_MARK_OFFSET = 0.4;
	constexpr double DOT_X_OFFSET_INNER = 0.175;
	constexpr double DOT_X_OFFSET_OUTER = 0.575;
}