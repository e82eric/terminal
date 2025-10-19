// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#pragma once

#include "FuzzyHighlightText.g.h"
#include "StreamingSuggestions.g.h"

namespace winrt::TerminalApp::implementation
{
    struct FuzzyHighlightText : FuzzyHighlightTextT<FuzzyHighlightText>
    {
        FuzzyHighlightText() = default;
        FuzzyHighlightText(const winrt::hstring& nameText);
        FuzzyHighlightText(
            const winrt::Windows::Foundation::Collections::IVector<winrt::TerminalApp::HighlightedRun>& nameHighlights, const winrt::hstring& nameText);
        til::property_changed_event PropertyChanged;
        WINRT_OBSERVABLE_PROPERTY(winrt::Windows::Foundation::Collections::IVector<winrt::TerminalApp::HighlightedRun>, NameHighlights, PropertyChanged.raise);
        WINRT_OBSERVABLE_PROPERTY(winrt::hstring, NameText, PropertyChanged.raise);
    };

    struct StreamingSuggestions : StreamingSuggestionsT<StreamingSuggestions>
    {
        StreamingSuggestions();
        void SelectNextItem(bool moveDown);

        void Open(
            Microsoft::Terminal::Control::TermControl const& termControl,
            const winrt::hstring& needle, Windows::Foundation::Point anchor,
            Windows::Foundation::Size space,
            float characterHeight,
            const winrt::hstring& currentWord);
        void SearchBox_TextChanged(Windows::Foundation::IInspectable const& sender, Windows::UI::Xaml::Controls::TextChangedEventArgs const& e);
        void FocusSearchBox();
        void UserControl_KeyDown(Windows::Foundation::IInspectable const& sender, Windows::UI::Xaml::Input::KeyRoutedEventArgs const& e);
        til::property_changed_event PropertyChanged;
        til::typed_event<winrt::TerminalApp::StreamingSuggestions, Microsoft::Terminal::Settings::Model::Command> DispatchCommandRequested;
        float _characterHeight;

    private:
        Microsoft::Terminal::Control::TermControl _termControl{ nullptr };
        std::vector<Microsoft::Terminal::Control::SuggestionBatch> _batches;
        Windows::Foundation::Collections::IObservableVector<TerminalApp::FuzzyHighlightText> _filteredActions{ nullptr };
        std::mutex _batchesMutex;
        std::atomic<std::uint64_t> _searchVersion{ 0 };
        std::wstring _currentSearchTerm;
        std::mutex _searchTermMutex;
        Windows::Foundation::Point _anchor{};
        Windows::Foundation::Size _space{};
        winrt::Windows::UI::Xaml::Controls::ListView::SizeChanged_revoker _sizeChangedRevoker;
        winrt::hstring _currentWord;
        bool _isOpenedUpward{ false };

        winrt::Windows::Foundation::IAsyncAction _performFuzzySearch(std::wstring searchTerm, uint64_t version);
        void _triggerSearch();
        void _recalculateTopMargin();
        void _setDirection(bool openUpward);
        void _scrollToIndex(uint32_t index);
        void _dispatchSelectedCommand();
        void _selectedCommandChanged(const Windows::Foundation::IInspectable& sender, const Windows::UI::Xaml::RoutedEventArgs& args);
    };
}

namespace winrt::TerminalApp::factory_implementation
{
    struct StreamingSuggestions : StreamingSuggestionsT<StreamingSuggestions, implementation::StreamingSuggestions>
    {
    };
    struct FuzzyHighlightText : FuzzyHighlightTextT<FuzzyHighlightText, implementation::FuzzyHighlightText> { };
}
