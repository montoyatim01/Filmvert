
#include "state.h"
#include "imageMeta.h"
#include "imageParams.h"
#include "logger.h"

//--- Set Pointers ---//
/*
    Set the metadata and parameter pointers
    from the image object.

    NOTE: This has to be done after the image
    has been added to the roll, otherwise
    invalid pointers are assigned
*/
void userState::setPtrs(imageMetadata* meta, imageParams* params, bool* reRdnr) {
    // Set pointers
    imgMeta = meta;
    imgParam = params;
    reRender = reRdnr;

    // Set initial state
    updateState();
    return;
}

//--- Update State ---//
/*
    Add a copy of the current image
    state to the vectors.

    Cap the size of the state.

    Clear the end if our position is
    not already at the end
*/
void userState::updateState() {
    if (!imgMeta || !imgParam) {
        LOG_ERROR("Cannot update state, no meta or param!");
        return;
    }

    if (statePos < paramStates.size() - 1) {
        // Our state position is not the
        // last position in the state set
        if (statePos >= 0) {
            // We actually have a state
            metaStates.erase(metaStates.begin() + statePos + 1,
                            metaStates.end());

            paramStates.erase(paramStates.begin() + statePos + 1,
                            paramStates.end());
        }
    }
    // Add our copies to the states, track the pos
    imageMetadata tmpMeta = *imgMeta;
    metaStates.push_back(tmpMeta);

    imageParams tmpParam = *imgParam;
    paramStates.push_back(tmpParam);

    // Remove the front elements if larger than settings
    if (metaStates.size() > appPrefs.prefs.undoLevels)
        while (metaStates.size() > appPrefs.prefs.undoLevels)
            metaStates.pop_front();

    if (paramStates.size() > appPrefs.prefs.undoLevels)
        while (paramStates.size() > appPrefs.prefs.undoLevels)
            paramStates.pop_front();

    statePos = paramStates.size() - 1;
}

//--- Set State ---//
/*
    Seek to a specific state iterator
*/
void userState::setState(int pos) {

    // Check it's a valid state
    if (pos > metaStates.size() - 1 ||
        pos > paramStates.size() - 1 ||
        pos < 0) {
        LOG_ERROR("Cannot set state, invalid index!");
        return;
    }
    if (*imgParam != paramStates[pos])
        *reRender = true;
    *imgMeta = metaStates[pos];
    *imgParam = paramStates[pos];
    statePos = pos;

    return;
}

//--- Undo State ---//
/*
    Move back one in the state list
*/
void userState::undoState() {

    // Check we have state to go too
    if (statePos == 0) {
        LOG_WARN("Out of undo states!");
        return;
    } else if (statePos > metaStates.size() ||
        statePos > paramStates.size()) {
        LOG_WARN("State pos ahead of states!");
        return;
    }

    statePos--;

    // If values affecting image are
    // different flag for render
    if (*imgParam != paramStates[statePos])
        *reRender = true;
    *imgMeta = metaStates[statePos];
    *imgParam = paramStates[statePos];

    return;
}

//--- Redo State ---//
/*
    Move forward one in the state list
*/
void userState::redoState() {

    // Check we have a state to go to
    if (statePos == metaStates.size() - 1 ||
        statePos == paramStates.size() - 1) {
        LOG_WARN("Out of redo states!");
        return;
    }

    statePos++;

    // If values affecting image are
    // different flag for render
    if (*imgParam != paramStates[statePos])
        *reRender = true;
    *imgMeta = metaStates[statePos];
    *imgParam = paramStates[statePos];

    return;
}

//--- State Size ---//
/*
    Size of the state
*/
int userState::stateSize() {
    return std::min(metaStates.size(), paramStates.size());
}

//--- Undo Available ---//
/*
    Are there states left to
    move back to
*/
bool userState::undoAvail() {
    return statePos > 0;
}

//--- Redo Available ---//
/*
    Are there states left
    to move forward to
*/
bool userState::redoAvail() {
    return (statePos < metaStates.size() - 1 &&
            statePos < paramStates.size() - 1);
}
