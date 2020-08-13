#include "navigation.h"

VPADStatus vpadBuffer[1];
VPADReadError vpadError;

struct KPADWrapper {
    bool connected;
    KPADExtensionType type;
    KPADStatus status;
};

std::array<KPADWrapper, 4> KPADControllers {false, WPAD_EXT_CORE, KPADStatus{}};


// Technical functions

void initializeInputs() {
    VPADInit();
    KPADInit();
}

void updateInputs() {
    // Read VPAD (Gamepad) input
    VPADRead(VPADChan::VPAD_CHAN_0, vpadBuffer, 1, &vpadError);
    if (vpadError != VPAD_READ_SUCCESS) {
        WHBLogPrint("Couldn't read the VPAD controller input");
    }

    // Read WPAD (Pro Controller, Wiimote, Classic) input
    // Loop over each controller channel
    for (uint32_t i=0; i < KPADControllers.size(); i++) {
        // Test if it's connected
        if (WPADProbe((WPADChan)i, &KPADControllers[i].type) != 0) {
            KPADControllers[i].connected = false;
            continue;
        }
        
        KPADControllers[i].connected = true;

        // Read the input
        KPADRead((KPADChan)i, &KPADControllers[i].status, 1);
    }
}

// Navigation Actions

// Check whether the Gamepad is pressing the specified button
bool vpadButtonPressed(VPADButtons button) {
    if (vpadError == VPAD_READ_SUCCESS) {
        if (vpadBuffer[0].hold & button) return true;
    }
    return false;
}

// Check whether any KPAD controller is pressing the specified button
bool kpadButtonPressed(WPADButton button) {
    for (const auto& pad : KPADControllers) {
        if (pad.connected && pad.status.error == KPAD_ERROR_OK) {
            if (pad.status.hold & button) {
                return true;
            }
        }
    }
    return false;
}

// Check whether a stick value crosses the threshold
bool getStickDirection(float stickValue, float threshold) {
    return threshold < 0 ? (stickValue < threshold) : (stickValue > threshold);
}

// Checks for all sticks whether 
bool getKPADSticksDirection(bool XAxis, float threshold) {
    for (const auto& pad : KPADControllers) {
        if (!pad.connected) continue;
        
        if (pad.type == KPADExtensionType::WPAD_EXT_NUNCHUK || pad.type == KPADExtensionType::WPAD_EXT_MPLUS_NUNCHUK) {
            return getStickDirection(XAxis ? pad.status.nunchuck.stick.x : pad.status.nunchuck.stick.y, threshold);
        }
        else if (pad.type == KPADExtensionType::WPAD_EXT_CLASSIC || pad.type == KPADExtensionType::WPAD_EXT_MPLUS_CLASSIC) {
            return getStickDirection(XAxis ? pad.status.classic.leftStick.x : pad.status.classic.leftStick.y, threshold);
        }
        else if (pad.type == KPADExtensionType::WPAD_EXT_PRO_CONTROLLER) {
            return getStickDirection(XAxis ? pad.status.pro.leftStick.x : pad.status.pro.leftStick.y, threshold);
        }
    }
    return false;
}

bool navigatedUp() {
    return vpadButtonPressed(VPAD_BUTTON_UP) || kpadButtonPressed(WPAD_BUTTON_UP) || getStickDirection(vpadBuffer[0].leftStick.y, 0.7) || getKPADSticksDirection(false, 0.7);
}

bool navigatedDown() {
    return vpadButtonPressed(VPAD_BUTTON_DOWN) || kpadButtonPressed(WPAD_BUTTON_DOWN) || getStickDirection(vpadBuffer[0].leftStick.y, -0.7) || getKPADSticksDirection(false, -0.7);
}

bool navigatedLeft() {
    return vpadButtonPressed(VPAD_BUTTON_LEFT) || kpadButtonPressed(WPAD_BUTTON_LEFT) || getStickDirection(vpadBuffer[0].leftStick.x, 0.7) || getKPADSticksDirection(true, 0.7);
}

bool navigatedRight() {
    return vpadButtonPressed(VPAD_BUTTON_RIGHT) || kpadButtonPressed(WPAD_BUTTON_RIGHT) || getStickDirection(vpadBuffer[0].leftStick.x, -0.7) || getKPADSticksDirection(true, -0.7);
}

// Button Actions
bool pressedOk() {
    return vpadButtonPressed(VPAD_BUTTON_A) || kpadButtonPressed(WPAD_BUTTON_A);
}
bool pressedStart() {
    return vpadButtonPressed(VPAD_BUTTON_PLUS) || kpadButtonPressed(WPAD_BUTTON_PLUS);
}
bool pressedBack() {
    return vpadButtonPressed(VPAD_BUTTON_B) || kpadButtonPressed(WPAD_BUTTON_B);
}