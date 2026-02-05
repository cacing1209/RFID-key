#ifndef AUTH_H
#include <Arduino.h>

struct auth_state
{
private:
    const char token[42] = "lockerqyubitL0002L0004L0008L000264L000128";
    // modif youre custom token

public:
    bool check_auth(String auth_h = "xxx");
};

#endif