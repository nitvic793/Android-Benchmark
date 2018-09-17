#pragma once

//--------------------------------------------------------------------------------
// Constants
//--------------------------------------------------------------------------------
const int32_t DOUBLE_TAP_TIMEOUT = 300 * 1000000;
const int32_t TAP_TIMEOUT = 180 * 1000000;
const int32_t DOUBLE_TAP_SLOP = 100;
const int32_t TOUCH_SLOP = 8;

enum {
	GESTURE_STATE_NONE = 0,
	GESTURE_STATE_START = 1,
	GESTURE_STATE_MOVE = 2,
	GESTURE_STATE_END = 4,
	GESTURE_STATE_ACTION = (GESTURE_STATE_START | GESTURE_STATE_END),
};

typedef int32_t GESTURE_STATE;

class AGestureDetector
{
	float dp_factor_ = 1.f;
	int32_t down_pointer_id_;
	float down_x_ = 0;
	float down_y_ = 0;
	
public:
	GESTURE_STATE DetectTap(const AInputEvent* motion_event);
	AGestureDetector();
	~AGestureDetector();
};
