#ifndef AUTH_H
#define AUTH_H

#include <string>

/**
 * @brief Simple authentication system for patient monitor
 * 
 * Provides basic login functionality with hardcoded credentials.
 * Session management is handled via simple boolean flag.
 */
class Authentication
{
public:
    /**
     * @brief Validate login credentials
     * @param username Username
     * @param password Password
     * @return true if credentials are valid
     */
    static bool validateLogin(const std::string& username, const std::string& password);

    /**
     * @brief Get hashed password (for security)
     * @param password Plain text password
     * @return Hashed password (simple hash for demo)
     */
    static std::string hashPassword(const std::string& password);

private:
    // Hardcoded credentials from spec
    static constexpr const char* VALID_USERNAME = "prog6";
    static constexpr const char* VALID_PASSWORD = "WeLikeAChallenge2025!";
};

#endif // AUTH_H
