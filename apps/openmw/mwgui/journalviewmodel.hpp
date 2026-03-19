#ifndef MWGUI_JOURNALVIEWMODEL_HPP
#define MWGUI_JOURNALVIEWMODEL_HPP

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <string_view>

#include <components/misc/utf8stream.hpp>

#include "../mwdialogue/quest.hpp"

namespace MWDialogue
{
    class Topic;
}

namespace MWGui
{
    /// View-Model for the journal GUI
    ///
    /// This interface defines an abstract data model suited
    ///  specifically to the needs of the journal GUI. It isolates
    /// the journal GUI from the implementation details of the
    /// game data store.
    struct JournalViewModel
    {
        typedef std::shared_ptr<JournalViewModel> Ptr;

        /// The base interface for both journal entries and topics.
        struct Entry
        {
            /// returns the body text for the journal entry
            virtual std::string_view body() const = 0;

            /// Visits each subset of text in the body, delivering the beginning
            /// and end of the span relative to the body, and a valid Topic pointer if
            /// the span represents a keyword, or nullptr if not.
            virtual void visitSpans(
                std::function<void(const MWDialogue::Topic*, size_t, size_t)> visitor) const = 0;

            virtual ~Entry() = default;
        };

        /// An interface to topic data.
        struct TopicEntry : Entry
        {
            /// Returns the name of the NPC this portion of dialog was heard from.
            virtual std::string_view source() const = 0;

            virtual ~TopicEntry() = default;
        };

        /// An interface to journal data.
        struct JournalEntry : Entry
        {
            /// Returns the in-game date this entry was added to the journal.
            virtual std::string_view timestamp() const = 0;

            virtual ~JournalEntry() = default;
        };

        /// called prior to journal opening
        virtual void load() = 0;

        /// called prior to journal closing
        virtual void unload() = 0;

        /// returns true if their are no journal entries to display
        virtual bool isEmpty() const = 0;

        /// walks the active and optionally completed, quests providing the name and completed status
        virtual void visitQuestNames(bool active_only, std::function<void(std::string_view, bool)> visitor) const = 0;

        /// walks over the journal entries related to all quests with the given name
        /// If \a questName is empty, simply visits all journal entries
        virtual void visitJournalEntries(std::string_view questName,
            std::function<void(JournalEntry const&, const MWDialogue::Quest*)> visitor) const
            = 0;

        /// provides the name of the topic
        virtual void visitTopicName(
            const MWDialogue::Topic& topic, std::function<void(std::string_view)> visitor) const
            = 0;

        /// walks over the topics whose names start with the character
        virtual void visitTopicNamesStartingWith(
            Utf8Stream::UnicodeChar character, std::function<void(std::string_view)> visitor) const = 0;

        /// walks over the topic entries for the given topic
        virtual void visitTopicEntries(
            const MWDialogue::Topic& topic, std::function<void(TopicEntry const&)> visitor) const
            = 0;

        // create an instance of the default journal view model implementation
        static Ptr create();

        virtual ~JournalViewModel() = default;
    };
}

#endif // MWGUI_JOURNALVIEWMODEL_HPP
