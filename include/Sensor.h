#pragma once

class Sensor
{
protected:
    string up, down, device;

public:
    Sensor(){};
    Sensor(string device, string up, string down)
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

void initializeSensors();