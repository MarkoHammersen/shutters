#pragma once

class Actuator
{
protected:
    string up, down, device;

public:
    Actuator(){};
    Actuator(string device, string up, string down)
    {
        up = up;
        down = down;
    }
    string getUp()
    {
        return up;
    }
    string getDown()
    {
        return down;
    }
};