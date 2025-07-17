# pragma once


class Interface{
public:
    virtual ~Interface() = default;
    virtual bool init() = 0;
    virtual bool send_frame(const struct can_frame &frame) = 0;
    virtual bool receive_frame(struct can_frame &frame, int timeout_ms) = 0;
};