#ifndef AUTH_H
#include <Arduino.h>
#include <string.h>

struct auth_state
{
private:
    const char token[42] = "lockerqyubitL0002L0004L0008L000264L000128"; // dummy auth
public:
    bool check_auth(String auth_h = "xxx");
};

#endif