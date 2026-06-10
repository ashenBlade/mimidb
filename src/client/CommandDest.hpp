#pragma once

#include <string>
namespace mi::client {

class ICommandDest {
    public:
        virtual void Write(const std::string &value) = 0;
        virtual ~ICommandDest() = default;
};

class FileCommandDest : public ICommandDest {
    private:
        // File descriptor of file
        int _fd;
    public:
        FileCommandDest(int fd);
        void Write(const std::string &value) override;
        ~FileCommandDest() = default;
};

}
