#include "Difficulty.h"

#include <stdexcept>

GameConfig PresetConfig(Difficulty difficulty) {
    switch (difficulty) {
        case Difficulty::Beginner:
            return {9, 9, 14, Difficulty::Beginner};
        case Difficulty::Intermediate:
            return {16, 16, 45, Difficulty::Intermediate};
        case Difficulty::Expert:
            return {30, 30, 155, Difficulty::Expert};
        case Difficulty::Custom:
            throw std::invalid_argument("Use GameConfig directly for custom difficulty");
    }
    throw std::invalid_argument("Unknown difficulty");
}
