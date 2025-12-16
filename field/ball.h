#ifndef BALL_H
#define BALL_H

#include "field_object.h"

class Ball final : public FieldObject {
public:
	Ball();
	~Ball() override = default;
};

#endif //BALL_H
