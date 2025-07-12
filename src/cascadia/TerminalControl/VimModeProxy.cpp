#include "pch.h"
#include "VimModeProxy.h"
#include "../../cascadia/TerminalCore/Terminal.hpp"
#include "../../cascadia/TerminalCore/lib/VimMotions.hpp"

VimModeProxy::VimModeProxy(std::shared_ptr<Microsoft::Terminal::Core::Terminal> terminal, winrt::Microsoft::Terminal::Control::implementation::ControlCore *controlCore, Search* searcher)
{
    _terminal = terminal;
    _controlCore = controlCore;
    _searcher = searcher;
}

bool comparePointsLessThanOrEqual(const til::point_span& span, const til::point& pt) {
    return std::tie(span.start.y, span.start.x) <= std::tie(pt.y, pt.x);
}

bool comparePointsLess(const til::point_span& span, const til::point& pt) {
    return std::tie(span.start.y, span.start.x) < std::tie(pt.y, pt.x);
}

void VimModeProxy::_moveToNextSearchResult(bool moveForward, bool isVisual)
{
    if (_searcher->Results().size() > 0)
    {
        const auto point = _terminal->GetSelectionAnchors()->start;
        const auto& results = _searcher->Results();

        std::vector<til::point_span>::const_iterator toSelect;
        if (!moveForward)
        {
            const auto it = std::lower_bound(results.begin(), results.end(), point, comparePointsLessThanOrEqual);

            if (it != results.end())
            {
                toSelect = it;
            }
            else
            {
                toSelect = _searcher->Results().begin();
            }
        }
        else
        {
            auto it = std::lower_bound(results.begin(), results.end(), point, comparePointsLess);
            if (it != results.begin())
            {
                it = std::prev(it);
                toSelect = it;
            }
            else
            {
                toSelect = std::prev(_searcher->Results().end());
            }
        }

        _scrollIfNeeded(toSelect->start);
        _updateSelection(isVisual, toSelect->start);
    }
}

void VimModeProxy::_highlightClosestSearchResult(bool moveForward)
{
    std::vector<til::point_span> oldResults;
    if (_searcher->Results().size() > 0)
    {
        const auto point = _terminal->GetSelectionAnchors()->start;
        const auto& results = _searcher->Results();

        std::vector<til::point_span>::const_iterator toSelect;
        if (!moveForward)
        {
            const auto it = std::lower_bound(results.begin(), results.end(), point, comparePointsLess);

            if (it != results.end())
            {
                toSelect = it;
            }
            else
            {
                toSelect = _searcher->Results().begin();
            }
        }
        else
        {
            auto it = std::lower_bound(results.begin(), results.end(), point, comparePointsLessThanOrEqual);

            if (it != results.begin())
            {
                it = std::prev(it);
                toSelect = it;
            }
            else
            {
                toSelect = std::prev(_searcher->Results().end());
            }
        }

        const auto idx = std::distance(results.begin(), toSelect);
        _terminal->SetSearchHighlightFocused(idx);
        _scrollIfNeeded(toSelect->start);
    }
}

void VimModeProxy::_handleSearch(bool moveForward)
{
    std::vector<til::point_span> oldResults;
    _searcher->ResetIfStaleRegex(*_terminal, _searchString, moveForward, true, &oldResults);
    _terminal->SetSearchHighlights(_searcher->Results());
    const auto results = _searcher->Results();
    if (!results.empty())
    {
        _terminal->SetSearchHighlightFocused(results.size() - 1);
        _highlightClosestSearchResult(moveForward);
    }
    _controlCore->UpdateSelectionFromVim(oldResults);
}

void VimModeProxy::_tilChar(std::wstring_view vkey, bool isVisual)
{
    vim::motions::TilChar(*_terminal, vkey, isVisual);
}

void VimModeProxy::_findChar(std::wstring_view vkey, bool isVisual)
{
    vim::motions::FindChar(*_terminal, vkey, isVisual);
}

void VimModeProxy::_findCharBack(std::wstring_view vkey, bool isVisual)
{
    vim::motions::FindCharBackwards(*_terminal, vkey, isVisual);
}

void VimModeProxy::_tilCharBack(std::wstring_view vkey, bool isVisual)
{
    vim::motions::TilCharBackwards(*_terminal, vkey, isVisual);
}

void VimModeProxy::_matchingChar(std::wstring_view startDelimiter, std::wstring_view endDelimiter, bool onStartDelimiter, bool isVisual)
{
    vim::motions::MatchingChar(*_terminal, startDelimiter, endDelimiter, onStartDelimiter, isVisual);
}

void VimModeProxy::_matchingChar(til::point startPos, std::wstring_view startDelimiter, std::wstring_view endDelimiter, bool onStartDelimiter, bool inBlock)
{
    vim::motions::MatchingChar(*_terminal, startPos, startDelimiter, endDelimiter, onStartDelimiter, inBlock);
}

void VimModeProxy::_inDelimiterSameLine(std::wstring_view delimiter, bool includeDelimiter)
{
    vim::motions::InDelimiterSameLine(*_terminal, delimiter, includeDelimiter);
}

void VimModeProxy::_inDelimiter(std::wstring_view startDelimiter, std::wstring_view endDelimiter, bool includeDelimiter)
{
    vim::motions::InDelimiter(*_terminal, startDelimiter, endDelimiter, includeDelimiter);
}

void VimModeProxy::_selectWordRight(bool isVisual, bool isLargeWord)
{
    vim::motions::MoveWordRight(*_terminal, isLargeWord, isVisual);
}

void VimModeProxy::_selectWordLeft(bool isVisual, bool isLargeWord)
{
    vim::motions::MoveWordLeft(*_terminal, isLargeWord, isVisual);
}

void VimModeProxy::_selectWordStartRight(bool isVisual, bool isLargeWord)
{
    vim::motions::MoveWordStartRight(*_terminal, isLargeWord, isVisual);
}

void VimModeProxy::_selectInWord(bool largeWord)
{
    vim::motions::SelectInWord(*_terminal, largeWord);
}

void VimModeProxy::_selectLineRight(bool isVisual)
{
    vim::motions::MoveToEndOfLine(*_terminal, isVisual);
}

void VimModeProxy::_selectLineLeft(bool isVisual)
{
    vim::motions::MoveToStartOfLine(*_terminal, isVisual);
}

void VimModeProxy::_selectLineUp()
{
    vim::motions::SelectLineUp(*_terminal);
}

void VimModeProxy::_selectLineDown()
{
    vim::motions::SelectLineDown(*_terminal);
}

void VimModeProxy::_selectTop(bool isVisual, bool entireLine)
{
    vim::motions::SelectTop(*_terminal, isVisual, entireLine);
}

void VimModeProxy::_selectBottom(bool isVisual, bool entireLine)
{
    vim::motions::SelectBottom(*_terminal, isVisual, entireLine);
}

void VimModeProxy::_selectHalfPageUp(bool isVisual, bool entireLine)
{
    vim::motions::SelectHalfPageUp(*_terminal, isVisual, entireLine);
}

void VimModeProxy::_selectHalfPageDown(bool isVisual, bool entireLine)
{
    vim::motions::SelectHalfPageDown(*_terminal, isVisual, entireLine);
}

void VimModeProxy::_selectPageUp(bool /*isVisual*/)
{
    //vim::motions::SelectPageUp(*_terminal, isVisual);
}

void VimModeProxy::_selectPageDown(bool /*isVisual*/)
{
    //vim::motions::SelectPageDown(*_terminal, isVisual);
}

void VimModeProxy::_selectCharRight(bool isVisual)
{
    vim::motions::MoveRight(*_terminal, isVisual);
}

void VimModeProxy::_selectCharLeft(bool isVisual)
{
    vim::motions::MoveLeft(*_terminal, isVisual);
}

void VimModeProxy::_selectDown(bool isVisual, til::CoordType /*lastY*/)
{
    vim::motions::MoveDown(*_terminal, isVisual);
}

void VimModeProxy::_selectUp(bool isVisual)
{
    vim::motions::MoveUp(*_terminal, isVisual);
}

void VimModeProxy::_selectLineFirstNonBlankChar(bool isVisual)
{
    vim::motions::MoveToFirstNonBlankChar(*_terminal, isVisual);
}

bool VimModeProxy::_executeVimSelection(
    const VimActionType action,
    const VimTextObjectType textObject,
    const int times,
    const VimMotionType motion,
    const bool isVisual,
    const std::wstring /*searchString*/,
    std::wstring_view vkey)
{
    bool exitAfter = false;
    bool selectFromStart = isVisual || action == VimActionType::yank;
    const auto lastNonSpaceChar = _getLastNonSpaceChar();

    for (int i = 0; i < times; i++)
    {
        auto pairIsVisual = isVisual || _vimMode == VimMode::visualLine;
        switch (textObject)
        {
        case VimTextObjectType::startCurlyBracePair:
            _matchingChar(L"{", L"}", true, pairIsVisual);
            break;
        case VimTextObjectType::startRoundBracePair:
            _matchingChar(L"(", L")", true, pairIsVisual);
            break;
        case VimTextObjectType::startSquareBracePair:
            _matchingChar(L"[", L"]", true, pairIsVisual);
            break;
        case VimTextObjectType::endCurlyBracePair:
            _matchingChar(L"{", L"}", false, pairIsVisual);
            break;
        case VimTextObjectType::endRoundBracePair:
            _matchingChar(L"(", L")", false, pairIsVisual);
            break;
        case VimTextObjectType::endSquareBracePair:
            _matchingChar(L"[", L"]", false, pairIsVisual);
            break;
        case VimTextObjectType::inCurlyBracePair:
            _inDelimiter(L"{", L"}", false);
            break;
        case VimTextObjectType::inSquareBracePair:
            _inDelimiter(L"[", L"]", false);
            break;
        case VimTextObjectType::inRoundBracePair:
            _inDelimiter(L"(", L")", false);
            break;
        case VimTextObjectType::inSingleQuotePair:
            _inDelimiterSameLine(L"'", false);
            break;
        case VimTextObjectType::inDoubleQuotePair:
            _inDelimiterSameLine(L"\"", false);
            break;
        case VimTextObjectType::inAngleBracketPair:
            _inDelimiter(L"<", L">", false);
            break;
        case VimTextObjectType::aroundCurlyBracePair:
            _inDelimiter(L"{", L"}", true);
            break;
        case VimTextObjectType::aroundSquareBracePair:
            _inDelimiter(L"[", L"]", true);
            break;
        case VimTextObjectType::aroundRoundBracePair:
            _inDelimiter(L"(", L")", true);
            break;
        case VimTextObjectType::aroundSingleQuotePair:
            _inDelimiterSameLine(L"'", true);
            break;
        case VimTextObjectType::aroundDoubleQuotePair:
            _inDelimiterSameLine(L"\"", true);
            break;
        case VimTextObjectType::aroundAngleBracketPair:
            _inDelimiter(L"<", L">", true);
            break;
        case VimTextObjectType::findChar:
            if (motion == VimMotionType::forward)
            {
                _findChar(vkey, selectFromStart);
            }
            else
            {
                _findCharBack(vkey, selectFromStart);
            }
            break;
        case VimTextObjectType::tilChar:
            if (motion == VimMotionType::forward)
            {
                _tilChar(vkey, selectFromStart);
            }
            else
            {
                _tilCharBack(vkey, selectFromStart);
            }
            break;
        case VimTextObjectType::findCharReverse:
            if (motion == VimMotionType::forward)
            {
                _findCharBack(vkey, selectFromStart);
            }
            else
            {
                _findChar(vkey, selectFromStart);
            }
            break;
        case VimTextObjectType::tilCharReverse:
            if (motion == VimMotionType::forward)
            {
                _tilCharBack(vkey, selectFromStart);
            }
            else
            {
                _tilChar(vkey, selectFromStart);
            }
            break;
        case VimTextObjectType::largeWord:
        case VimTextObjectType::word:
            if (motion == VimMotionType::moveForwardToEnd)
            {
                _selectWordRight(selectFromStart, textObject == VimTextObjectType::largeWord);
            }
            else if (motion == VimMotionType::moveBackToBegining)
            {
                _selectWordLeft(selectFromStart, textObject == VimTextObjectType::largeWord);
            }
            else if (motion == VimMotionType::moveForwardToStart)
            {
                _selectWordStartRight(selectFromStart, textObject == VimTextObjectType::largeWord);
            }
            break;
        case VimTextObjectType::inLargeWord:
        case VimTextObjectType::inWord:
            _selectInWord(textObject == VimTextObjectType::inLargeWord);
            break;
        case VimTextObjectType::line:
            if (motion == VimMotionType::moveForwardToEnd)
            {
                _selectLineRight(selectFromStart);
            }
            else if (motion == VimMotionType::moveBackToBegining)
            {
                _selectLineLeft(selectFromStart);
            }
            else if (motion == VimMotionType::backToFirstNonSpaceChar)
            {
                _selectLineFirstNonBlankChar(selectFromStart);
            }
            break;
        case VimTextObjectType::entireLine:
            switch (motion)
            {
            case VimMotionType::moveUp:
                _selectLineUp();
                break;
            case VimMotionType::moveDown:
                _selectLineDown();
                break;
            case VimMotionType::moveToTopOfBuffer:
                _selectTop(true, true);
                break;
            case VimMotionType::moveToBottomOfBuffer:
                _selectBottom(true, true);
                break;
            case VimMotionType::halfPageUp:
                _selectHalfPageUp(true, true);
                break;
            case VimMotionType::halfPageDown:
                _selectHalfPageDown(true, true);
                break;
            case VimMotionType::pageUp:
                _selectPageUp(true);
                break;
            case VimMotionType::pageDown:
                _selectPageDown(true);
        case VimMotionType::selectCurrentLine:
                vim::motions::SelectEntireLine(*_terminal);
                break;
            }
            break;
        case VimTextObjectType::charTextObject:
            switch (motion)
            {
        case VimMotionType::none:
                vim::motions::SelectCurrentChar(*_terminal);
                break;
            case VimMotionType::moveLeft:
                _selectCharLeft(selectFromStart);
                break;
            case VimMotionType::moveDown:
                _selectDown(selectFromStart, lastNonSpaceChar.y);
                break;
            case VimMotionType::moveUp:
                _selectUp(selectFromStart);
                break;
            case VimMotionType::moveRight:
                _selectCharRight(selectFromStart);
                break;
            }
            break;
        case VimTextObjectType::none:
            switch (motion)
            {
            case VimMotionType::moveToTopOfBuffer:
                _selectTop(selectFromStart, false);
                break;
            case VimMotionType::moveToBottomOfBuffer:
                _selectBottom(selectFromStart, false);
                break;
            case VimMotionType::halfPageUp:
                _selectHalfPageUp(selectFromStart, false);
                break;
            case VimMotionType::halfPageDown:
                _selectHalfPageDown(selectFromStart, false);
                break;
            case VimMotionType::pageUp:
                _selectPageUp(selectFromStart);
                break;
            case VimMotionType::pageDown:
                _selectPageDown(selectFromStart);
                break;
            }
            break;
        }
    }

    switch (action)
    {
    case VimActionType::swapPivot:
    {
        auto selectionAnchors = _terminal->GetSelectionAnchors();
        const auto selection{ selectionAnchors.write() };
        selection->pivot = selection->pivot == selection->start ? selection->end : selection->start;
        _terminal->SetSelectionAnchors(selection);
        break;
    }
    case VimActionType::enterQuickCopyMode:
    case VimActionType::enterQuickSelectMode:
        ResetVimState();
        _controlCore->EnterQuickSelectMode(L"[\\w\\d\\S]+", action == VimActionType::enterQuickCopyMode);
        break;
    case VimActionType::toggleRowNumbers:
    {
        _showRowNumbers = !_showRowNumbers;
        _controlCore->ToggleRowNumberMode(_showRowNumbers);
        break;
    }
    case VimActionType::scroll:
        _vimScrollScreenPosition(_textObject);
        break;
    case VimActionType::fuzzyFind:
    {
        const auto bufferData = _terminal->RetrieveSelectedTextFromBuffer(false);
        auto searchString = bufferData.plainText;
        _controlCore->StartFuzzySearch(searchString);
        break;
    }
    case VimActionType::commitSearch:
    {
        auto results = _searcher->Results();
        if (results.size() > 0)
        {
            auto focused = _terminal->GetSearchHighlightFocused();
            _terminal->SelectChar(focused->start);
            _terminal->SetSearchHighlightFocused(-1);
            _controlCore->UpdateSelectionFromVim(results);
        }
        break;
    }
    case VimActionType::nextSearchResult:
    {
        auto moveForward = motion != VimMotionType::forward;
        _moveToNextSearchResult(moveForward, isVisual);
        break;
    }
    case VimActionType::search:
    {
        auto moveForward = motion != VimMotionType::forward;
        if (textObject == VimTextObjectType::word)
        {
            _selectInWord(false);
            const auto bufferData = _terminal->RetrieveSelectedTextFromBuffer(moveForward);
            _searchString = L"\\b" + bufferData.plainText + L"\\b";
            _handleSearch(moveForward);
            _moveToNextSearchResult(moveForward, isVisual);
            //_terminal->SetSearchHighlightFocused(-1);
        }
        else
        {
            _handleSearch(moveForward);
        }
        break;
    }
    case VimActionType::yank:
    {
        _terminal->SelectYankRegion();
        std::thread hideTimerThread([this]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(250));
            {
                ResetVimState();
            }
        });
        hideTimerThread.detach();
        _controlCore->CopySelectionToClipboard(false, false, nullptr);
        exitAfter = false;
        break;
    }
    case VimActionType::toggleVisualOn:
        //_terminal->SetPivot();
        break;
    case VimActionType::enterBlockSelectionMode:
        _vimMode = _terminal->IsBlockSelection() ? VimMode::normal : VimMode::visual;
        if (_terminal->IsBlockSelection())
        {
            auto cursor = vim::motions::GetVimCursor(*_terminal);
            _terminal->SetBlockSelection(false);
            vim::motions::SelectPoint(*_terminal, cursor.Span.start);
        }
        else
        {
            _controlCore->ToggleBlockSelection();
        }
        break;
    case VimActionType::exit:
        exitAfter = true;
        break;
    }

    return exitAfter;
}

bool VimModeProxy::TryVimModeKeyBinding(
    const WORD vkey,
    const ::Microsoft::Terminal::Core::ControlKeyStates mods)
{
    bool sequenceCompleted = false;
    bool hideMarkers = false;
    bool clearStateOnSequenceCompleted = true;
    bool skipExecute = false;

    if (vkey == 16 || vkey == 17 || vkey == 18)
    {
        return true;
    }

    wchar_t vkeyText[2] = { 0 };
    BYTE keyboardState[256];
    if (!GetKeyboardState(keyboardState))
    {
        return true;
    }
    if (mods.IsShiftPressed())
    {
        keyboardState[VK_SHIFT] = 0x80;
    }
    ToUnicode(vkey, MapVirtualKey(vkey, MAPVK_VK_TO_VSC), keyboardState, vkeyText, 2, 0);

    std::wstringstream timesStringStream(_timesString);
    if (!_timesString.empty())
    {
        timesStringStream >> _times;
    }
    else
    {
        _times = 1;
    }

    if (vkey == L'R' && mods.IsCtrlPressed())
    {
        _action = VimActionType::toggleRowNumbers;
        _textObject = VimTextObjectType::none;
        sequenceCompleted = true;
    }
    else if (vkey >= 0x30 && vkey <= 0x39 && !mods.IsShiftPressed() && _vimMode != VimMode::search && _textObject != VimTextObjectType::findChar && _textObject != VimTextObjectType::findCharReverse && _textObject != VimTextObjectType::tilChar && _textObject != VimTextObjectType::tilCharReverse)
    {
        _timesString += vkeyText;
        std::wstringstream timesStringStream(_timesString);
        int times;
        timesStringStream >> times;
    }
    else if (vkey == VK_ESCAPE && _terminal->IsBlockSelection())
    {
        _action = VimActionType::enterBlockSelectionMode;
        sequenceCompleted = true;
    }
    // '%'
    else if (vkeyText[0] == L'%')
    {
        const auto bufferData = _terminal->RetrieveSelectedTextFromBuffer(false);
        if (bufferData.plainText.size() > 0)
        {
            auto selection = _terminal->GetSelectionAnchors();
            auto pos = selection->start == selection->pivot ? selection->end : selection->start;
            auto firstChar = _terminal->GetTextBuffer().GetRowByOffset(pos.y).GlyphAt(pos.x)[0];
            if (firstChar == L'(')
            {
                _textObject = VimTextObjectType::startRoundBracePair;
                sequenceCompleted = true;
            }
            else if (firstChar == L')')
            {
                _textObject = VimTextObjectType::endRoundBracePair;
                sequenceCompleted = true;
            }
            else if (firstChar == L'{')
            {
                _textObject = VimTextObjectType::startCurlyBracePair;
                sequenceCompleted = true;
            }
            else if (firstChar == L'}')
            {
                _textObject = VimTextObjectType::endCurlyBracePair;
                sequenceCompleted = true;
            }
            else if (firstChar == L'[')
            {
                _textObject = VimTextObjectType::startSquareBracePair;
                sequenceCompleted = true;
            }
            else if (firstChar == L']')
            {
                _textObject = VimTextObjectType::endSquareBracePair;
                sequenceCompleted = true;
            }
        }
    }
    else if (_vimMode == VimMode::visualLine)
    {
        _textObject = VimTextObjectType::entireLine;
        sequenceCompleted = true;
        if ((vkey == L'U' && mods.IsCtrlPressed()) || vkey == VK_PRIOR)
        {
            _motion = VimMotionType::halfPageUp;
        }
        else if ((vkey == L'D' && mods.IsCtrlPressed()) || vkey == VK_NEXT)
        {
            _motion = VimMotionType::halfPageDown;
        }
        else if (vkey == L'G' && mods.IsShiftPressed() && _motion == VimMotionType::none)
        {
            _motion = VimMotionType::moveToBottomOfBuffer;
        }
        else if (vkey == L'G' && !mods.IsShiftPressed())
        {
            if (_motion == VimMotionType::g)
            {
                _motion = VimMotionType::moveToTopOfBuffer;
            }
            else
            {
                _motion = VimMotionType::g;
                sequenceCompleted = false;
            }
        }
        else if ((vkey == L'K' || vkey == VK_UP) && _motion == VimMotionType::none)
        {
            _motion = VimMotionType::moveUp;
        }
        else if ((vkey == L'J' || vkey == VK_DOWN || vkey == VK_RETURN) && _motion == VimMotionType::none)
        {
            _motion = VimMotionType::moveDown;
        }
        else if (vkey == L'Y' && _motion == VimMotionType::none)
        {
            _textObject = VimTextObjectType::none;
            _action = VimActionType::yank;
        }
        else if (vkey == L'F' && mods.IsCtrlPressed())
        {
            _motion = VimMotionType::pageDown;
        }
        else if (vkey == L'B' && mods.IsCtrlPressed())
        {
            _motion = VimMotionType::pageUp;
        }
        else if (vkey == VK_ESCAPE)
        {
            _vimMode = VimMode::normal;
            _textObject = VimTextObjectType::charTextObject;
        }
    }
    else if (_textObject == VimTextObjectType::tilChar || _textObject == VimTextObjectType::tilCharReverse || _textObject == VimTextObjectType::findChar || _textObject == VimTextObjectType::findCharReverse)
    {
        sequenceCompleted = true;
    }
    else if (_vimMode == VimMode::search)
    {
        if (vkey == VK_RETURN || vkey == VK_ESCAPE)
        {
            _motion = VimMotionType::none;
            _vimMode = VimMode::normal;
            if (vkey == VK_RETURN)
            {
                _action = VimActionType::commitSearch;
                sequenceCompleted = true;
            }
            else
            {
                _terminal->SetSearchHighlights({});
                _controlCore->UpdateSelectionFromVim(_searcher->Results());
            }
        }
        else
        {
            _action = VimActionType::search;
            _motion = _reverseSearch ? VimMotionType::back : VimMotionType::forward;
            sequenceCompleted = true;
            clearStateOnSequenceCompleted = false;
            hideMarkers = true;

            if (vkey == VK_BACK)
            {
                if (!_searchString.empty())
                {
                    _searchString.pop_back();
                }
            }
            else
            {
                if (vkey == L'V' && mods.IsCtrlPressed())
                {
                    auto pasteString = _getClipboardText();
                    _searchString += pasteString;
                }
                else
                {
                    _searchString += vkeyText;
                }
            }
        }
    }
    else if (vkey == L' ')
    {
        if (_leaderSequence)
        {
            _action = VimActionType::toggleRowNumbers;
            sequenceCompleted = true;
        }
        else
        {
            _leaderSequence = true;
        }
    }
    // * #
    else if ((vkey == 0x38 && mods.IsShiftPressed()) || (vkey == 0x33 && mods.IsShiftPressed()))
    {
        _action = VimActionType::search;
        _amount = VimTextAmount::in;
        _textObject = VimTextObjectType::word;

        _reverseSearch = vkey == 0x33;
        _motion = vkey == 0x38 ? VimMotionType::forward : VimMotionType::back;

        sequenceCompleted = true;
    }
    // / ?
    else if (vkey == 0xBF)
    {
        StartSearch(mods.IsShiftPressed());
        hideMarkers = true;
        _controlCore->StartVimSearch(_reverseSearch);
    }
    else if (vkey == L'O' && _vimMode == VimMode::visual)
    {
        _action = VimActionType::swapPivot;
        _textObject = VimTextObjectType::none;
        sequenceCompleted = true;
    }
    else if (vkey == L'Q' && mods.IsCtrlPressed())
    {
        _action = VimActionType::enterBlockSelectionMode;
        sequenceCompleted = true;
    }
    else if ((vkey == L'U' && mods.IsCtrlPressed()) || vkey == VK_PRIOR)
    {
        _motion = VimMotionType::halfPageUp;
        _textObject = VimTextObjectType::none;
        sequenceCompleted = true;
    }
    else if ((vkey == L'D' && mods.IsCtrlPressed()) || vkey == VK_NEXT)
    {
        _motion = VimMotionType::halfPageDown;
        _textObject = VimTextObjectType::none;
        sequenceCompleted = true;
    }
    else if (vkey == L'G' && mods.IsShiftPressed() && _motion == VimMotionType::none)
    {
        _motion = VimMotionType::moveToBottomOfBuffer;
        _textObject = VimTextObjectType::none;
        sequenceCompleted = true;
    }
    else if (vkey == L'G' && !mods.IsShiftPressed())
    {
        if (_motion == VimMotionType::g)
        {
            _motion = VimMotionType::moveToTopOfBuffer;
            _textObject = VimTextObjectType::none;
            sequenceCompleted = true;
        }
        else
        {
            _motion = VimMotionType::g;
        }
    }
    else if (vkey == L'N')
    {
        _action = VimActionType::nextSearchResult;
        if (_reverseSearch)
        {
            _motion = mods.IsShiftPressed() ? VimMotionType::forward : VimMotionType::back;
        }
        else
        {
            _motion = mods.IsShiftPressed() ? VimMotionType::back : VimMotionType::forward;
        }

        sequenceCompleted = true;
    }
    else if (vkey == L'I')
    {
        _amount = VimTextAmount::in;
    }
    else if (vkey == L'A')
    {
        _amount = VimTextAmount::around;
    }
    else if (vkey == L'F')
    {
        //Ooes this work?
        if (_leaderSequence)
        {
            _action = VimActionType::fuzzyFind;
        }
        else if (mods.IsShiftPressed())
        {
            _textObject = VimTextObjectType::findCharReverse;
            _motion = VimMotionType::forward;
        }
        else
        {
            if (mods.IsCtrlPressed())
            {
                _motion = VimMotionType::pageDown;
                _textObject = VimTextObjectType::none;
                sequenceCompleted = true;
            }
            else
            {
                _textObject = VimTextObjectType::findChar;
                _motion = VimMotionType::forward;
            }
        }
    }
    else if (vkey == L'T')
    {
        if (_action == VimActionType::scroll)
        {
            _textObject = VimTextObjectType::topOfScreen;
            sequenceCompleted = true;
        }
        else
        {
            _motion = VimMotionType::forward;
            if (mods.IsShiftPressed())
            {
                _textObject = VimTextObjectType::tilCharReverse;
            }
            else
            {
                _textObject = VimTextObjectType::tilChar;
            }
        }
    }
    else if (vkey == L'Y' && _motion == VimMotionType::none)
    {
        _action = VimActionType::yank;
        if (mods.IsShiftPressed())
        {
            _textObject = VimTextObjectType::line;
            _motion = VimMotionType::moveForwardToEnd;
            sequenceCompleted = true;
        }
        else if (_lastVkey[0] == L'y')
        {
            _motion = VimMotionType::selectCurrentLine;
            _textObject = VimTextObjectType::entireLine;
            sequenceCompleted = true;
        }
        else if (_vimMode == VimMode::visual)
        {
            _textObject = VimTextObjectType::none;
            sequenceCompleted = true;
        }
    }
    else if (vkey == L'E' && _motion == VimMotionType::none)
    {
        _textObject = mods.IsShiftPressed() ? VimTextObjectType::largeWord : VimTextObjectType::word;

        sequenceCompleted = true;
        _motion = VimMotionType::moveForwardToEnd;
    }
    else if (vkey == L'B' && (_motion == VimMotionType::none))
    {
        if (_action == VimActionType::scroll)
        {
            _textObject = VimTextObjectType::bottomOfScreen;
            sequenceCompleted = true;
        }
        else
        {
            if (mods.IsCtrlPressed())
            {
                _motion = VimMotionType::pageUp;
                _textObject = VimTextObjectType::none;
                sequenceCompleted = true;
            }
            else
            {
                _motion = VimMotionType::moveBackToBegining;
                _textObject = mods.IsShiftPressed() ? VimTextObjectType::largeWord : VimTextObjectType::word;
                sequenceCompleted = true;
            }
        }
    }
    // $
    else if (vkey == 0x34 && mods.IsShiftPressed())
    {
        _motion = VimMotionType::moveForwardToEnd;
        _textObject = VimTextObjectType::line;
        sequenceCompleted = true;
    }
    // ^
    else if (vkey == 0x36 && mods.IsShiftPressed())
    {
        _motion = VimMotionType::backToFirstNonSpaceChar;
        _textObject = VimTextObjectType::line;
        sequenceCompleted = true;
    }
    // |
    else if (vkey == VK_OEM_5 && mods.IsShiftPressed())
    {
        _motion = VimMotionType::moveBackToBegining;
        _textObject = VimTextObjectType::line;
        sequenceCompleted = true;
    }
    else if ((vkey == L'K' || vkey == VK_UP) && _motion == VimMotionType::none)
    {
        _motion = VimMotionType::moveUp;
        _textObject = _vimMode == VimMode::visualLine || (_times > 1 && _action == VimActionType::yank) ? VimTextObjectType::entireLine : VimTextObjectType::charTextObject;
        sequenceCompleted = true;
    }
    else if ((vkey == L'J' || vkey == VK_DOWN || vkey == VK_RETURN) && _motion == VimMotionType::none)
    {
        _textObject = _vimMode == VimMode::visualLine || (_times > 1 && _action == VimActionType::yank) ? VimTextObjectType::entireLine : VimTextObjectType::charTextObject;
        _motion = VimMotionType::moveDown;
        sequenceCompleted = true;
    }
    else if ((vkey == L'L' || vkey == VK_RIGHT) && _motion == VimMotionType::none)
    {
        _motion = VimMotionType::moveRight;
        _textObject = VimTextObjectType::charTextObject;
        sequenceCompleted = true;
    }
    else if ((vkey == L'H' || vkey == VK_LEFT) && _motion == VimMotionType::none)
    {
        _motion = VimMotionType::moveLeft;
        _textObject = VimTextObjectType::charTextObject;
        sequenceCompleted = true;
    }
    else if (vkey == L'W')
    {
        if (_leaderSequence)
        {
            _action = mods.IsShiftPressed() ? VimActionType::enterQuickCopyMode : VimActionType::enterQuickSelectMode;
            sequenceCompleted = true;
        }
        else if (_amount == VimTextAmount::in)
        {
            _textObject = mods.IsShiftPressed() ? VimTextObjectType::inLargeWord : VimTextObjectType::inWord;
        }
        else
        {
            _textObject = mods.IsShiftPressed() ? VimTextObjectType::largeWord : VimTextObjectType::word;
            _motion = VimMotionType::moveForwardToStart;
        }

        sequenceCompleted = true;
    }
    // {}
    else if ((vkey == VK_OEM_4 || vkey == VK_OEM_6) && mods.IsShiftPressed())
    {
        _textObject = _amount == VimTextAmount::in ? VimTextObjectType::inCurlyBracePair : _amount == VimTextAmount::around ? VimTextObjectType::aroundCurlyBracePair :
                                                                                                                               VimTextObjectType::charTextObject;
        sequenceCompleted = true;
    }
    // []
    else if (vkey == VK_OEM_4 || vkey == VK_OEM_6)
    {
        _textObject = _amount == VimTextAmount::in ? VimTextObjectType::inSquareBracePair : _amount == VimTextAmount::around ? VimTextObjectType::aroundSquareBracePair :
                                                                                                                               VimTextObjectType::charTextObject;
        sequenceCompleted = true;
    }
    // '
    else if (vkey == 0xDE && !mods.IsShiftPressed())
    {
        _textObject = _amount == VimTextAmount::in ? VimTextObjectType::inSingleQuotePair : _amount == VimTextAmount::around ? VimTextObjectType::aroundSingleQuotePair :
                                                                                                                               VimTextObjectType::charTextObject;
        sequenceCompleted = true;
    }
    // "
    else if (vkey == 0xDE && mods.IsShiftPressed())
    {
        _textObject = _amount == VimTextAmount::in ? VimTextObjectType::inDoubleQuotePair : _amount == VimTextAmount::around ? VimTextObjectType::aroundDoubleQuotePair :
                                                                                                                               VimTextObjectType::charTextObject;
        sequenceCompleted = true;
    }
    // ()
    else if (vkey == 0x39 && mods.IsShiftPressed() || vkey == 0x30 && mods.IsShiftPressed())
    {
        _textObject = _amount == VimTextAmount::in ? VimTextObjectType::inRoundBracePair : _amount == VimTextAmount::around ? VimTextObjectType::aroundRoundBracePair :
                                                                                                                              VimTextObjectType::charTextObject;
        sequenceCompleted = true;
    }
    // <>
    else if ((vkey == VK_OEM_COMMA && mods.IsShiftPressed()) || (vkey == VK_OEM_PERIOD && mods.IsShiftPressed()))
    {
        _textObject = _amount == VimTextAmount::in ? VimTextObjectType::inAngleBracketPair : _amount == VimTextAmount::around ? VimTextObjectType::aroundAngleBracketPair :
                                                                                                                                VimTextObjectType::charTextObject;
        sequenceCompleted = true;
    }
    // ; ,
    else if (vkey == 186 || vkey == 188)
    {
        _motion = vkey == 186 ? VimMotionType::forward : VimMotionType::back;

        _action = _lastAction;
        wcsncpy_s(vkeyText, _lastVkey, sizeof(vkeyText) / sizeof(vkeyText[0]));
        sequenceCompleted = true;
        _textObject = _lastTextObject;

        sequenceCompleted = true;
    }
    else if (vkey == L'V' && _vimMode == VimMode::normal && _motion == VimMotionType::none)
    {
        _vimMode = VimMode::visual;
        _action = VimActionType::toggleVisualOn;
        if (mods.IsShiftPressed())
        {
            _vimMode = VimMode::visualLine;
            _motion = VimMotionType::selectCurrentLine;
            _textObject = VimTextObjectType::entireLine;
            sequenceCompleted = true;
        }
        sequenceCompleted = true;
    }
    else if (vkey == L'S')
    {
        if (_leaderSequence && _action == VimActionType::fuzzyFind)
        {
            _textObject = VimTextObjectType::inWord;
            sequenceCompleted = true;
        }
    }
    else if (vkey == L'Z')
    {
        if (_action == VimActionType::scroll)
        {
            _textObject = VimTextObjectType::centerOfScreen;
            sequenceCompleted = true;
        }
        else
        {
            _action = VimActionType::scroll;
        }
    }
    else if (vkey == VK_ESCAPE)
    {
        if (_vimMode == VimMode::search || _vimMode == VimMode::visual)
        {
            _vimMode = VimMode::normal;
            _textObject = VimTextObjectType::charTextObject;
            _motion = VimMotionType::none;
            sequenceCompleted = true;
        }
        else if (_vimMode == VimMode::normal)
        {
            _action = VimActionType::exit;
            _textObject = VimTextObjectType::none;
            sequenceCompleted = true;
        }
    }
    else
    {
        sequenceCompleted = true;
        skipExecute = true;
    }

    auto shouldExit = false;

    if (sequenceCompleted && !skipExecute)
    {
        shouldExit = _executeVimSelection(_action, _textObject, _times, _motion, _vimMode == VimMode::visual, _searchString, vkeyText);
    }

    wcsncpy_s(_lastVkey, vkeyText, sizeof(_lastVkey) / sizeof(_lastVkey[0]));

    if (sequenceCompleted)
    {
        if (_action == VimActionType::yank)
        {
            _vimMode = VimMode::normal;
        }
        _setStateForCompletedSequence();

        if (shouldExit)
        {
            ResetVimState();
            return true;
        }
    }

    _controlCore->UpdateSelectionFromVim();

    return true;
}

bool VimModeProxy::ShowRowNumbers()
{
    return _showRowNumbers;
}

void VimModeProxy::ShowRowNumbers(bool val)
{
    _showRowNumbers = val;
}

int32_t VimModeProxy::ViewportRowToHighlight()
{
    auto lock = _terminal->LockForReading();
    const auto offset = _terminal->GetScrollOffset();
    const auto selection = _terminal->GetSelectionAnchors();
    if (!selection->active)
    {
        return _terminal->GetCursorPosition().y;
    }
    const auto pivotIsStart = selection->start == selection->pivot;
    const til::point point = pivotIsStart ? selection->end : selection->start;
    return point.y - offset;
}

void VimModeProxy::_setPosForResize(til::point pos, til::point& target) const
{
    const auto selection = _terminal->GetSelectionAnchors();
    auto lineNumber = 0;
    for (auto i = 0; i < pos.y; i++)
    {
        if (!_terminal->GetTextBuffer().GetRowByOffset(i).WasWrapForced())
        {
            lineNumber++;
        }
    }
    target.x = pos.x;
    auto y = pos.y - 1;
    const auto width = _terminal->GetTextBuffer().GetRowByOffset(y).size();
    while (_terminal->GetTextBuffer().GetRowByOffset(y).WasWrapForced())
    {
        target.x += width;
        y--;
    }

    target.y = lineNumber;
}

void VimModeProxy::StoreSelectionForResize()
{
    const auto selection = _terminal->GetSelectionAnchors();
    _setPosForResize(selection->start, _tempStart);
    _setPosForResize(selection->end, _tempEnd);
    _setPosForResize(selection->pivot, _tempPivot);
    const auto offSet = _terminal->GetScrollOffset();
    _tempTop = selection->start.y - offSet;
}

til::point VimModeProxy::_updateFromResize(til::point from) const
{
    auto selection = _terminal->GetSelectionAnchors();
    auto lineNumber = 0;
    auto i = 0;
    while (lineNumber < from.y)
    {
        if (!_terminal->GetTextBuffer().GetRowByOffset(i).WasWrapForced())
        {
            lineNumber++;
        }
        i++;
    }

    const auto width = _terminal->GetTextBuffer().GetRowByOffset(i).size();
    til::CoordType extraI = from.x / width;
    til::CoordType x = from.x % width;

    const auto point = til::point{ x, i + extraI };
    return point;
}

void VimModeProxy::UpdateSelectionFromResize() const
{
    auto selectionAnchors = _terminal->GetSelectionAnchors();
    const auto selection{ selectionAnchors.write() };
    selection->start = _updateFromResize(_tempStart);
    selection->pivot = _updateFromResize(_tempPivot);
    selection->end = _updateFromResize(_tempEnd);
    _terminal->SetSelectionAnchors(selection);
    _terminal->UserScrollViewport(selection->start.y - _tempTop);
}

void VimModeProxy::CommitSearch()
{
    _vimMode = VimMode::normal;
    _executeVimSelection(VimActionType::commitSearch, VimTextObjectType::none, 1, VimMotionType::none, false, _searchString, L"");
    _controlCore->UpdateSelectionFromVim(_searcher->Results());
    _setStateForCompletedSequence();
}

til::point VimModeProxy::_getLastNonSpaceChar() const
{
    auto &buffer = _terminal->GetTextBuffer();
    auto maybeLastChar = buffer.GetLastNonSpaceCharacter();
    auto maybeLastRowNumber = maybeLastChar.y;
    while (buffer.GetRowByOffset(maybeLastRowNumber).GetLastNonSpaceColumn() == 0)
    {
        maybeLastRowNumber--;
    }
    auto lastColumn = buffer.GetRowByOffset(maybeLastRowNumber).GetLastNonSpaceColumn();
    return til::point{ lastColumn, maybeLastRowNumber };
}

void VimModeProxy::_setStateForCompletedSequence()
{
    _lastTextObject = _textObject;
    _lastAction = _action;
    _lastMotion = _motion;
    _textObject = VimTextObjectType::charTextObject;
    _action = VimActionType::none;
    _motion = VimMotionType::none;
    _lastTimes = _times;
    _timesString = L"";
    _amount = VimTextAmount::none;
    _leaderSequence = false;
}

void VimModeProxy::ExitVimSearch()
{
    _vimMode = VimMode::normal;
    auto selection = _terminal->GetSelectionAnchors();
    _scrollIfNeeded(selection->start);
    _setStateForCompletedSequence();
    _controlCore->ClearSearch();
}

void VimModeProxy::SetSearchString(std::wstring_view searchString)
{
    _searchString = searchString;
    _executeVimSelection(VimActionType::search, VimTextObjectType::none, 1, _motion, false, _searchString, L"");
}

void VimModeProxy::StartSearch(bool isReverse)
{
        if (isReverse)
        {
            _motion = VimMotionType::back;
            _reverseSearch = true;
        }
        else
        {
            _motion = VimMotionType::forward;
            _reverseSearch = false;
        }
        _action = VimActionType::search;
        _vimMode = VimMode::search;
        _searchString = L"";

        _action = VimActionType::search;
}

void VimModeProxy::_vimScrollScreenPosition(VimTextObjectType textObjectType)
{
    static const int32_t paddingRows = 5;
    int offset;
    switch (textObjectType)
    {
    case VimTextObjectType::bottomOfScreen:
        offset = _terminal->GetViewport().Height() - paddingRows;
        break;
    case VimTextObjectType::centerOfScreen:
        offset = _terminal->GetViewport().Height() / 2;
        break;
    case VimTextObjectType::topOfScreen:
        offset = paddingRows;
        break;
    }
    _terminal->UserScrollViewport(_terminal->GetSelectionEnd().y - offset);
}

//This can probably be removed
void VimModeProxy::_updateSelection(bool isVisual, til::point adjusted)
{
    auto selectionAnchors = _terminal->GetSelectionAnchors();
    const auto selection{ selectionAnchors.write() };
    if (isVisual)
    {
        auto pivotIsStart = selection->start == til::point{ selection->pivot.x - 1, selection->pivot.y };
        //This means that the end is moving
        if (pivotIsStart)
        {
            if (adjusted < selection->pivot)
            {
                selection->start = adjusted;
                selection->end = til::point{ selection->pivot.x, selection->pivot.y };
            }
            else
            {
                selection->end = til::point{ adjusted.x + 1, adjusted.y };
            }
        }
        //This means that the start is moving
        else
        {
            if (adjusted > selection->pivot)
            {
                selection->start = selection->pivot;
                selection->end = til::point{ adjusted.x + 1, adjusted.y };
            }
            else
            {
                selection->start = til::point{ adjusted.x + 1, adjusted.y };
            }
        }
    }
    else
    {
        selection->start = adjusted;
        selection->end = til::point{adjusted.x + 1, adjusted.y};
        selection->pivot = selection->end;
    }
    _terminal->SetSelectionAnchors(selection);
}

void VimModeProxy::_scrollIfNeeded(const til::point& pos) noexcept
{
    const auto viewport = _terminal->GetViewport();
    const auto scrollOffset = _terminal->GetViewport().Top();

    if (pos.y >= scrollOffset && pos.y < scrollOffset + viewport.Height())
    {
        
    }
    else
    {
        auto newY = std::max(0, pos.y - 5);
        _terminal->UserScrollViewport(newY);
    }
}

void VimModeProxy::ResetVimState()
{
    const auto lock = _terminal->LockForWriting();
    ExitVimMode();
    _lastTextObject = VimTextObjectType::none;
    _lastAction = VimActionType::none;
    _lastMotion = VimMotionType::none;
    _textObject = VimTextObjectType::none;
    _lastTimes = 0;
    _searchString = L"";
    _lastVkey[0] = L'\0';
    _terminal->ClearYankRegion();
    _terminal->ClearSelection();
    _terminal->SetSearchHighlights({});
    _controlCore->UpdateSelectionFromVim(_searcher->Results());
    _controlCore->ExitVim();
}

VimModeProxy::VimMode VimModeProxy::_getVimMode()
{
    return _vimMode;
}

void VimModeProxy::ExitVimMode()
{
    _vimMode = VimMode::none;
}

void VimModeProxy::EnterVimMode(bool selectLastChar)
{
    _vimMode = VimMode::normal;
    ResetVimModeForSizeChange(selectLastChar);
}

bool VimModeProxy::IsInVimMode()
{
    return _getVimMode() != VimMode::none;
}

void VimModeProxy::ResetVimModeForSizeChange(bool selectLastChar)
{
    if (_vimMode != VimMode::none)
    {
        auto lock = _terminal->LockForWriting();

        if (!_terminal->IsSelectionActive())
        {
            _terminal->ToggleMarkMode();
            if (_terminal->SelectionMode() != ::Microsoft::Terminal::Core::Terminal::SelectionInteractionMode::Mark)
            {
                _terminal->ToggleMarkMode();
            }
            selectLastChar = true;
        }

        if (_terminal->SelectionMode() != ::Microsoft::Terminal::Core::Terminal::SelectionInteractionMode::Mark)
        {
            _terminal->ToggleMarkMode();
        }

        if (selectLastChar)
        {
            vim::motions::SelectLastNonSpaceChar(*_terminal);
        }

        if (selectLastChar)
        {
            _vimScrollScreenPosition(VimTextObjectType::centerOfScreen);
        }
        _controlCore->UpdateSelectionFromVim();
    }
}

void VimModeProxy::SelectRow(int32_t row, int32_t col)
{
    EnterVimMode(false);
    if (_terminal->SelectionMode() != ::Microsoft::Terminal::Core::Terminal::SelectionInteractionMode::Mark)
    {
        _terminal->ToggleMarkMode();
    }

    auto vp = _terminal->GetViewport();
    if (col > vp.Width())
    {
        int32_t rows = col / vp.Width();
        col %= vp.Width();
        row += rows;
    }

    _terminal->SelectChar(til::point{ col, row });
    _vimScrollScreenPosition(VimTextObjectType::centerOfScreen);
    _controlCore->UpdateSelectionFromVim();
}

std::wstring VimModeProxy::_getClipboardText()
{
    const auto clipboard = _openClipboard();
    if (!clipboard)
    {
        LOG_LAST_ERROR();
        return L"";
    }

    // This handles most cases of pasting text as the OS converts most formats to CF_UNICODETEXT automatically.
    if (const auto handle = GetClipboardData(CF_UNICODETEXT))
    {
        const wil::unique_hglobal_locked lock{ handle };
        const auto str = static_cast<const wchar_t*>(lock.get());
        if (!str)
        {
            return L"";
        }

        // As per: https://learn.microsoft.com/en-us/windows/win32/dataxchg/standard-clipboard-formats
        //   CF_UNICODETEXT: [...] A null character signals the end of the data.
        // --> Use wcsnlen() to determine the actual length.
        // NOTE: Some applications don't add a trailing null character. This includes past conhost versions.
        const auto maxLen = GlobalSize(handle) / sizeof(wchar_t);
        auto result = std::wstring(str, maxLen);
        if (!result.empty() && result.back() == '\0')
        {
            result.pop_back(); // Removes the last character if it's a null character
        }
        return result;
    }
    return L"";
}

wil::unique_close_clipboard_call VimModeProxy::_openClipboard()
{
    auto hwnd = GetConsoleWindow();

    bool success = false;

    // OpenClipboard may fail to acquire the internal lock --> retry.
    for (DWORD sleep = 10;; sleep *= 2)
    {
        if (OpenClipboard(hwnd))
        {
            success = true;
            break;
        }
        // 10 iterations
        if (sleep > 10000)
        {
            break;
        }
        Sleep(sleep);
    }

    return wil::unique_close_clipboard_call{ success };
}
