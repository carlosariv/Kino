#include <windows.h>
#include <iostream>

int main() {
    wchar_t character = L'A'; // The character you want to convert

    // 1. Get the keyboard layout handle for the current thread

    // 2. Translate the character to its VK and modifier status

    if (result == -1) {
        std::cout << "Character cannot be mapped to a virtual key." << std::endl;
        return 1;
    }

    // 3. Extract the Virtual-Key code (low byte)

    // 4. Extract the shift state flags (high byte)
    BYTE shiftState = HIBYTE(result);

    // Print the results
    std::wcout << L"Character: " << character << std::endl;
    std::cout << "Virtual-Key Code: 0x" << std::hex << (int)vkCode << std::endl;
    std::cout << "Shift State Flags: " << (int)shiftState << std::endl;

    // Interpret shiftState flags:
    // 1 = SHIFT is pressed
    // 2 = CTRL is pressed
    // 4 = ALT (AltGr) is pressed
    if (shiftState & 1) std::cout << "-> Requires SHIFT" << std::endl;
    if (shiftState & 2) std::cout << "-> Requires CTRL" << std::endl;
    if (shiftState & 4) std::cout << "-> Requires ALT" << std::endl;

    return 0;
}
