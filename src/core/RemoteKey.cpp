// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Patryk Strzelecki <patryk@strzelecki.info>
#include "RemoteKey.h"

namespace lgremote {

const char* remoteKeyLabel(RemoteKey key) {
    switch (key) {
        case RemoteKey::Power: return "POWER";
        case RemoteKey::PowerOn: return "POWER ON";
        case RemoteKey::PowerOff: return "POWER OFF";
        case RemoteKey::Up: return "UP";
        case RemoteKey::Down: return "DOWN";
        case RemoteKey::Left: return "LEFT";
        case RemoteKey::Right: return "RIGHT";
        case RemoteKey::Ok: return "OK";
        case RemoteKey::Back: return "BACK";
        case RemoteKey::Exit: return "EXIT";
        case RemoteKey::Home: return "HOME";
        case RemoteKey::Settings: return "SETTINGS";
        case RemoteKey::QuickMenu: return "QUICK MENU";
        case RemoteKey::Input: return "INPUT";
        case RemoteKey::Info: return "INFO";
        case RemoteKey::Guide: return "GUIDE";
        case RemoteKey::VolumeUp: return "VOL +";
        case RemoteKey::VolumeDown: return "VOL -";
        case RemoteKey::Mute: return "MUTE";
        case RemoteKey::ChannelUp: return "CH +";
        case RemoteKey::ChannelDown: return "CH -";
        case RemoteKey::Digit0: return "0";
        case RemoteKey::Digit1: return "1";
        case RemoteKey::Digit2: return "2";
        case RemoteKey::Digit3: return "3";
        case RemoteKey::Digit4: return "4";
        case RemoteKey::Digit5: return "5";
        case RemoteKey::Digit6: return "6";
        case RemoteKey::Digit7: return "7";
        case RemoteKey::Digit8: return "8";
        case RemoteKey::Digit9: return "9";
        case RemoteKey::PlayPause: return "PLAY/PAUSE";
        case RemoteKey::Play: return "PLAY";
        case RemoteKey::Pause: return "PAUSE";
        case RemoteKey::Stop: return "STOP";
        case RemoteKey::Rewind: return "REWIND";
        case RemoteKey::FastForward: return "FORWARD";
        case RemoteKey::Red: return "RED";
        case RemoteKey::Green: return "GREEN";
        case RemoteKey::Yellow: return "YELLOW";
        case RemoteKey::Blue: return "BLUE";
        case RemoteKey::Hdmi1: return "HDMI 1";
        case RemoteKey::Hdmi2: return "HDMI 2";
        case RemoteKey::Hdmi3: return "HDMI 3";
        case RemoteKey::Hdmi4: return "HDMI 4";
        case RemoteKey::EzAdjust: return "EZ ADJUST";
        case RemoteKey::InStart: return "IN START";
        case RemoteKey::None:
        case RemoteKey::Count:
            break;
    }
    return "";
}

}  // namespace lgremote
