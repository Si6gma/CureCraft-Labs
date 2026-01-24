#include "auth.h"
#include <iostream>

bool Authentication::validateLogin(const std::string& username, const std::string& password)
{
    bool valid = (username == VALID_USERNAME && password == VALID_PASSWORD);
    
    if (valid)
    {
        std::cout << "[Auth] Login successful for user: " << username << std::endl;
    }
    else
    {
        std::cout << "[Auth] Login failed for user: " << username << std::endl;
    }
    
    return valid;
}

std::string Authentication::hashPassword(const std::string& password)
{
    // Simple hash for demo purposes (NOT for production!)
    // In production, use bcrypt, argon2, or similar
    std::hash<std::string> hasher;
    size_t hash = hasher(password);
    return std::to_string(hash);
}
