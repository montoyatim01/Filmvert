#include "roll.h"


void filmRoll::rollUpState() {
    for (auto &img : images)
        img.imgState.updateState();
}

void filmRoll::rollUndo() {
    if (rollUAvail()) {
        for (auto &img : images)
            img.imgState.undoState();
    }
}

void filmRoll::rollRedo() {
    if (rollRAvil()) {
        for (auto &img : images)
            img.imgState.redoState();
    }
}

bool filmRoll::rollUAvail() {
    bool avail = true;
    for (auto &img : images)
        avail &= img.imgState.undoAvail();
    return avail;
}

bool filmRoll::rollRAvil() {
    bool avail = true;
    for (auto &img : images)
        avail &= img.imgState.redoAvail();
    return avail;
}
