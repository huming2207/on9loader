#include "main.hpp"
#include "hal/clock_setup.h"
#include "subghz.hpp"
#include <log.h>

int main()
{
    clock_init();
    ON9_LOGN("On9Loader init OK!");

    auto *lora= subghz::instance();
    if (!lora->init()) {
        ON9_LOGN("LoRa init failed");

    }


}