#ifndef SESSION_H
#define SESSION_H

#include <string>
#include <memory>

class Session {
public:
    Session(int sessionId);
    void processMessage(const std::string& message);
    void sendResponse(const std::string& response);
    int getSessionId() const;

private:
    int sessionId;
    // Additional private members for managing session state can be added here
};

#endif // SESSION_H