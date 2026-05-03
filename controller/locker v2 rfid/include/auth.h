#ifndef AUTH_H
#include <Arduino.h>
#include <string.h>
#include <sec_tkn.h>
struct auth_state
{
private:
    const char token[42] = token_sck; 
public:
    bool check_auth(String auth_h = "xxx");
};

#endif