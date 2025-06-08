#pragma once

#include <cstdint>


class on9ldr
{
public:
    static on9ldr &instance()
    {
        static on9ldr _instance;
        return _instance;
    }

    void operator=(on9ldr const &) =delete;
    on9ldr(on9ldr const &) = delete;

private:
    on9ldr() = default;

public:
    void init();

};
