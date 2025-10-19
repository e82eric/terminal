// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include "pch.h"
#include "StreamingSuggestions.h"
#include "StreamingSuggestions.g.cpp"
#include "FuzzyHighlightText.g.cpp"
#include "fzf/fzf.h"
#include <algorithm>

using namespace winrt;
using namespace winrt::Windows::UI::Xaml;

namespace winrt::TerminalApp::implementation
{
    FuzzyHighlightText::FuzzyHighlightText(
        const winrt::hstring& nameText) :
        _NameText(nameText)
    {
    }
    FuzzyHighlightText::FuzzyHighlightText(
        const winrt::Windows::Foundation::Collections::IVector<winrt::TerminalApp::HighlightedRun>& nameHighlights, const winrt::hstring& nameText) :
        _NameHighlights(nameHighlights), _NameText(nameText)
    {
    }

    StreamingSuggestions::StreamingSuggestions()
    {
        InitializeComponent();
        _filteredActions = winrt::single_threaded_observable_vector<TerminalApp::FuzzyHighlightText>();
        
        _sizeChangedRevoker = ListViewElement().SizeChanged(winrt::auto_revoke, [this](auto /*s*/, auto /*e*/) {
            if (Visibility() == Visibility::Visible)
            {
                this->_recalculateTopMargin();
            }
        });

        ListViewElement().SelectionChanged({ this, &StreamingSuggestions::_selectedCommandChanged });
    }

    void StreamingSuggestions::SelectNextItem(const bool moveDown)
    {
        auto selected = ListViewElement().SelectedIndex();
        const auto numItems = ::base::saturated_cast<int>(ListViewElement().Items().Size());

        if (numItems != 0 && (selected != -1 || moveDown))
        {
            const auto newIndex = ((numItems + selected + (moveDown ? 1 : -1)) % numItems);
            ListViewElement().SelectedIndex(newIndex);
            ListViewElement().ScrollIntoView(ListViewElement().SelectedItem());
        }
    }
    
    void StreamingSuggestions::Open(
        Microsoft::Terminal::Control::TermControl const& termControl,
        const winrt::hstring& needle,
        Windows::Foundation::Point anchor,
        Windows::Foundation::Size space,
        float characterHeight,
        const winrt::hstring& currentWord)
    {
        _characterHeight = characterHeight;
        _termControl = termControl; // Store the TermControl
        _currentWord = currentWord;
        _currentSearchTerm = currentWord;
        _filteredActions.Clear();
        
        _anchor = anchor;
        _space = space;
        _searchVersion = 0;
        
        Visibility(Visibility::Visible);
        ListViewElement().ItemsSource(_filteredActions);
        FocusSearchBox();
        SearchBox().Text(_currentWord);
        SearchBox().Select(currentWord.size(), 0);

        const auto proposedX = gsl::narrow_cast<int>(_anchor.X - 40);
        // If the control is too wide to fit in the window, clamp it fit inside
        // the window.
        const auto maxX = gsl::narrow_cast<int>(space.Width - ActualWidth());
        const auto clampedX = std::clamp(proposedX, 0, maxX);

        // Create a thickness for the new margins. This will set the left, then
        // we'll go update the top separately
        Margin(Windows::UI::Xaml::ThicknessHelper::FromLengths(clampedX, 0, 0, 0));
        
        _recalculateTopMargin();

        {
            std::lock_guard<std::mutex> lock(_batchesMutex);
            _batches.clear();
        }

        auto op = termControl.SuggestionScrollBackSearchAsync(
            needle,
            Microsoft::Terminal::Control::SuggestionBatchHandler{
                [weakThis = get_weak()](Microsoft::Terminal::Control::SuggestionBatch const& batch) {
                    if (auto self = weakThis.get())
                    {
                        {
                            std::lock_guard<std::mutex> lock(self->_batchesMutex);
                            self->_batches.push_back(batch);
                        }
                        self->_triggerSearch();
                    }
                } });
    }

    void StreamingSuggestions::SearchBox_TextChanged(Windows::Foundation::IInspectable const&, Windows::UI::Xaml::Controls::TextChangedEventArgs const&)
    {
        {
            const std::wstring searchTerm{ SearchBox().Text() };
            std::lock_guard lock(_searchTermMutex);
            _currentSearchTerm = searchTerm;
        }

        _triggerSearch();
    }
    
    void StreamingSuggestions::FocusSearchBox()
    {
        SearchBox().Focus(Windows::UI::Xaml::FocusState::Keyboard);
    }
    
    void StreamingSuggestions::UserControl_KeyDown(Windows::Foundation::IInspectable const&, Windows::UI::Xaml::Input::KeyRoutedEventArgs const& e)
    {
        if (e.Key() == Windows::System::VirtualKey::Escape)
        {
            _filteredActions.Clear();
            _termControl.PreviewInput(L"");
            SearchBox().Text(L"");
            Visibility(Windows::UI::Xaml::Visibility::Collapsed);
            e.Handled(true);
        }
        else if (e.Key() == Windows::System::VirtualKey::Up)
        {
            SelectNextItem(false);
            e.Handled(true);
        }
        else if (e.Key() == Windows::System::VirtualKey::Down)
        {
            SelectNextItem(true);
            e.Handled(true);
        }
        else if (e.Key() == Windows::System::VirtualKey::Enter ||
                 e.Key() == Windows::System::VirtualKey::Tab)
        {
            _termControl.PreviewInput(L"");
            _dispatchSelectedCommand();
            e.Handled(true);
        }
    }
    
    void StreamingSuggestions::_triggerSearch()
    {
        std::wstring term;
        {
            std::lock_guard lock(_searchTermMutex);
            term = _currentSearchTerm;
        }
        const std::uint64_t myVersion = ++_searchVersion;

        _performFuzzySearch(term, myVersion);
    }

    winrt::Windows::Foundation::IAsyncAction StreamingSuggestions::_performFuzzySearch(std::wstring searchTerm, uint64_t version)
    {
        co_await winrt::resume_background();
        using namespace std::chrono_literals;
        if (_searchVersion > 1)
        {
            co_await winrt::resume_after(10ms);
            if (version != _searchVersion)
            {
                co_return;
            }
        }

        std::vector<Microsoft::Terminal::Control::SuggestionBatch> batchesSnapshot;
        {
            std::lock_guard<std::mutex> lock(_batchesMutex);
            batchesSnapshot.assign(_batches.begin(), _batches.end());
        }

        if (searchTerm.empty())
        {
            std::vector<TerminalApp::FuzzyHighlightText> allItems;
            {
                for (const auto& batch : batchesSnapshot)
                {
                    if (version != _searchVersion)
                    {
                        co_return;
                    }
                    for (const auto& item : batch.Items())
                    {
                        auto highlight = winrt::make<FuzzyHighlightText>(item.Text);
                        allItems.push_back(highlight);
                        if (allItems.size() >= 250)
                        {
                            break;
                        }
                    }
                    if (allItems.size() >= 250)
                    {
                        break;
                    }
                }
            }

            co_await winrt::resume_foreground(Dispatcher(), Windows::UI::Core::CoreDispatcherPriority::Normal);
            if (version == _searchVersion)
            {
                // Reverse items if opening upward
                if (_isOpenedUpward)
                {
                    std::ranges::reverse(allItems);
                }

                _filteredActions.ReplaceAll(allItems);
                if (!allItems.empty() && ListViewElement().SelectedIndex() == -1)
                {
                    if (_isOpenedUpward)
                    {
                        ListViewElement().SelectedIndex(gsl::narrow_cast<int32_t>(allItems.size() - 1));
                        _scrollToIndex(gsl::narrow_cast<int32_t>(allItems.size() - 1));
                    }
                    else
                    {
                        ListViewElement().SelectedIndex(0);
                    }
                }
            }

            co_return;
        }

        auto pattern = fzf::matcher::ParsePattern(searchTerm);

        struct ScoredItem
        {
            Microsoft::Terminal::Control::SuggestionSearchItem item;
            int32_t score;
            std::optional<std::vector<fzf::matcher::TextRun>> runs;
            int32_t ordinal;
        };

        std::vector<ScoredItem> scoredItems;

        for (const auto& batch : batchesSnapshot)
        {
            if (version != _searchVersion)
            {
                co_return;
            }
            for (const auto& item : batch.Items())
            {
                auto text = item.Text;
                auto matchResult = fzf::matcher::Match(text, pattern);
                if (matchResult)
                {
                    scoredItems.push_back({ item, matchResult->Score, matchResult->Runs, item.Ordinal });
                }
            }
        }

        auto MaxResults = 250;
        if (scoredItems.size() > MaxResults)
        {
            std::ranges::partial_sort(scoredItems, scoredItems.begin() + MaxResults, [](const ScoredItem& a, const ScoredItem& b) {
                if (a.score == b.score)
                {
                    return a.ordinal < b.ordinal;
                }
                return a.score > b.score;
            });
            scoredItems.resize(MaxResults);
        }
        else
        {
            std::ranges::sort(scoredItems, [](const ScoredItem& a, const ScoredItem& b) {
                return a.score > b.score;
            });
        }

        co_await winrt::resume_foreground(Dispatcher(), Windows::UI::Core::CoreDispatcherPriority::Normal);

        if (_isOpenedUpward)
        {
            std::ranges::reverse(scoredItems);
        }

        _filteredActions.Clear();
        for (const auto& scoredItem : scoredItems)
        {
            std::vector<winrt::TerminalApp::HighlightedRun> segments;
            if (scoredItem.runs)
            {
                segments.resize(scoredItem.runs.value().size());
                std::transform(scoredItem.runs.value().begin(), scoredItem.runs.value().end(), segments.begin(), [](auto&& run) -> winrt::TerminalApp::HighlightedRun {
                    return { run.Start, run.End };
                });
            }
            auto highlight = winrt::make<FuzzyHighlightText>(
                winrt::single_threaded_vector(std::move(segments)),
                scoredItem.item.Text);
            _filteredActions.Append(highlight);
        }

        if (!scoredItems.empty() && ListViewElement().SelectedIndex() == -1)
        {
            if (_isOpenedUpward)
            {
                ListViewElement().SelectedIndex(gsl::narrow_cast<int32_t>(scoredItems.size() - 1));
                _scrollToIndex(gsl::narrow_cast<int32_t>(scoredItems.size() - 1));
            }
            else
            {
                ListViewElement().SelectedIndex(0);
            }
        }

        co_return;
    }
    
    void StreamingSuggestions::_dispatchSelectedCommand()
    {
        const auto selectedIndex = ListViewElement().SelectedIndex();
        if (selectedIndex >= 0 && selectedIndex < static_cast<int32_t>(_filteredActions.Size()))
        {
            const auto selectedItem = _filteredActions.GetAt(selectedIndex);
            const auto commandText = selectedItem.NameText();
            
            if (!commandText.empty())
            {
                auto cmd = Microsoft::Terminal::Settings::Model::Command::ScrollBackSuggestionToCommand(commandText, _currentWord, L"");
                
                Visibility(Windows::UI::Xaml::Visibility::Collapsed);
                _filteredActions.Clear();
                SearchBox().Text(L"");
                
                DispatchCommandRequested.raise(*this, cmd);
            }
        }
    }

    void StreamingSuggestions::_selectedCommandChanged(const Windows::Foundation::IInspectable& /*sender*/, const Windows::UI::Xaml::RoutedEventArgs& /*args*/)
    {
        if (Visibility() == Visibility::Visible)
        {
            const auto selectedIndex = ListViewElement().SelectedIndex();
            uint32_t newSelectedIndex = 0;

            if (selectedIndex >= 0 && selectedIndex < static_cast<int32_t>(_filteredActions.Size()))
            {
                const auto selectedItem = _filteredActions.GetAt(selectedIndex);
                
                auto highlightVector = winrt::single_threaded_vector<Microsoft::Terminal::Control::SuggestionSearchItem>();
                if (SearchBox().Text().size() < 2)
                {
                        auto suggestionItem = Microsoft::Terminal::Control::SuggestionSearchItem{};
                        suggestionItem.Text = selectedItem.NameText();
                        highlightVector.Append({ suggestionItem });
                }
                else
                {
                    std::vector<std::pair<Microsoft::Terminal::Control::SuggestionSearchItem, uint32_t>> itemsWithIndex;
                    for (uint32_t i = 0; i < _filteredActions.Size(); ++i)
                    {
                        const auto item = _filteredActions.GetAt(i);
                        auto suggestionItem = Microsoft::Terminal::Control::SuggestionSearchItem{};
                        suggestionItem.Text = item.NameText();
                        itemsWithIndex.push_back({ suggestionItem, i });
                    }

                    std::sort(itemsWithIndex.begin(), itemsWithIndex.end(), [](const auto& a, const auto& b) {
                        return a.first.Ordinal < b.first.Ordinal;
                    });

                    for (uint32_t i = 0; i < itemsWithIndex.size(); ++i)
                    {
                        highlightVector.Append(itemsWithIndex[i].first);
                        if (itemsWithIndex[i].second == static_cast<uint32_t>(selectedIndex))
                        {
                            newSelectedIndex = i;
                        }
                    }
                }

                auto backspaces = std::wstring(_currentWord.size(), L'\x7f');
                auto previewText = winrt::hstring{ fmt::format(FMT_COMPILE(L"{}{}"), backspaces, selectedItem.NameText()) };
                _termControl.PreviewInput(previewText);
            }
        }
    }

    void StreamingSuggestions::_setDirection(bool openUpward)
    {
        // Call Measure() on the descriptions backdrop, so that it gets it's new
        // DesiredSize for this new description text.
        //
        // If you forget this, then we _probably_ weren't laid out since
        // updating that text, and the ActualHeight will be the _last_
        // description's height.
        RootGrid().Measure({
            static_cast<float>(ActualWidth()),
            static_cast<float>(ActualHeight()),
        });

        auto currentMargin = Margin();
        _isOpenedUpward = openUpward;

        if (openUpward)
        {
            Windows::UI::Xaml::Controls::Grid::SetRow(SearchBox(), 2);
            // Bottom Up.

            // This is wackier, because we need to calculate the offset upwards
            // from our anchor. So we need to get the size of our elements:
            const auto backdropHeight = RootGrid().ActualHeight();

            const auto marginTop = (_anchor.Y - backdropHeight);

            currentMargin.Top = marginTop;
        }
        else
        {
            Windows::UI::Xaml::Controls::Grid::SetRow(SearchBox(), 0);
            currentMargin.Top = (_anchor.Y);
        }

        Margin(currentMargin);
    }

    void StreamingSuggestions::_scrollToIndex(uint32_t index)
    {
        auto numItems = ListViewElement().Items().Size();

        if (numItems == 0)
        {
            return;
        }

        auto clampedIndex = std::clamp<int32_t>(index, 0, numItems - 1);
        ListViewElement().SelectedIndex(clampedIndex);
        ListViewElement().ScrollIntoView(ListViewElement().SelectedItem());
    }

    void StreamingSuggestions::_recalculateTopMargin()
    {
        const auto controlHeight = 250;
        const auto spaceBelow = _space.Height - _anchor.Y;

        auto openUpward = true;
        if (spaceBelow >= controlHeight)
        {
            openUpward = false;
        }
        _setDirection(openUpward);
    }
}
