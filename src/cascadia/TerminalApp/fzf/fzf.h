#pragma once

#include <vector>
#include <icu.h>

namespace fzf::matcher
{
    struct TextRun
    {
        size_t Start;
        size_t End;
    };

    struct MatchResult
    {
        int32_t Score = 0;
        std::vector<TextRun> Runs;
    };

    struct TextAndNameMatchResult
    {
        int32_t Score = 0;
        std::vector<TextRun> Runs;
        std::vector<TextRun> NameRuns;
    };

    struct Pattern
    {
        std::wstring text;
        std::vector<std::vector<UChar32>> terms;
    };

    Pattern ParsePattern(std::wstring_view patternStr);
    std::optional<MatchResult> Match(std::wstring_view text, const Pattern& pattern);
    std::optional<TextAndNameMatchResult> MatchTextAndName(std::wstring_view text, std::wstring_view name, const Pattern& pattern);
}
