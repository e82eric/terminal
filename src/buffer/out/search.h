/*++
Copyright (c) Microsoft Corporation
Licensed under the MIT license.

Module Name:
- search.h

Abstract:
- This module is used for searching through the screen for a substring

Author(s):
- Michael Niksa (MiNiksa) 20-Apr-2018

Revision History:
- From components of find.c by Jerry Shea (jerrysh) 1-May-1997
--*/

#pragma once

#include "textBuffer.hpp"
#include "../renderer/inc/IRenderData.hpp"
#include "../fzf/fzf.h"

struct FuzzySearchResultRow
{
    til::CoordType startRowNumber;
    std::vector<int32_t> positions;
};

class Search final
{
public:
    Search() = default;

    bool ResetIfStale(Microsoft::Console::Render::IRenderData& renderData, const std::wstring_view& needle, bool reverse, bool caseInsensitive);
    std::vector<FuzzySearchResultRow> FuzzySearch(Microsoft::Console::Render::IRenderData& renderData, const std::wstring_view& needle) const;

    void MoveToCurrentSelection();
    void MoveToPoint(til::point anchor) noexcept;
    void MovePastPoint(til::point anchor) noexcept;
    void FindNext() noexcept;

    const til::point_span* GetCurrent() const noexcept;
    void HighlightResults() const;
    bool SelectCurrent() const;

    const std::vector<til::point_span>& Results() const noexcept;
    ptrdiff_t CurrentMatch() const noexcept;

private:
    // _renderData is a pointer so that Search() is constexpr default constructable.
    Microsoft::Console::Render::IRenderData* _renderData = nullptr;
    std::wstring _needle;
    bool _caseInsensitive = false;
    uint64_t _lastMutationId = 0;

    std::vector<til::point_span> _results;
    ptrdiff_t _index = 0;
    ptrdiff_t _step = 0;
    fzf_slab_t* _fzfSlab = fzf_make_default_slab();
};
