#include <auth.h>
bool auth_state::check_auth(String auth_h)
{
    if (!auth_h)
    {
        return false;
    }

    String auth_str = auth_h;
    auth_str.trim();

    // Check if starts with "Bearer "
    if (auth_str.startsWith("Bearer "))
    {
        auth_str = auth_str.substring(7); // Remove "Bearer "
        auth_str.trim();
    }

    return (auth_str.equals(token));
}