#include "journalviewmodel.hpp"

#include <sstream>
#include <unordered_map>

#include <MyGUI_LanguageManager.h>

#include <components/misc/strings/algorithm.hpp>
#include <components/translation/translation.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/journal.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/world.hpp"

#include "../mwdialogue/keywordsearch.hpp"
#include "../mwworld/datetimemanager.hpp"

namespace MWGui
{

    struct JournalViewModelImpl;

    struct JournalViewModelImpl : JournalViewModel
    {
        mutable bool mKeywordSearchLoaded;
        mutable MWDialogue::KeywordSearch mKeywordSearch;
        mutable std::unordered_map<std::string, const MWDialogue::Topic*> mTopics;

        JournalViewModelImpl()
            : mKeywordSearchLoaded(false)
        {
        }

        virtual ~JournalViewModelImpl() = default;

        void load() override {}

        void unload() override
        {
            mKeywordSearch.clear();
            mTopics.clear();
            mKeywordSearchLoaded = false;
        }

        void ensureKeyWordSearchLoaded() const
        {
            if (!mKeywordSearchLoaded)
            {
                MWBase::Journal* journal = MWBase::Environment::get().getJournal();

                for (MWBase::Journal::TTopicIter i = journal->topicBegin(); i != journal->topicEnd(); ++i)
                {
                    const MWDialogue::Topic& topic = i->second;
                    const std::string topicId = Misc::StringUtils::lowerCase(topic.getName());
                    mTopics[topicId] = &topic;
                    mKeywordSearch.seed(topic.getName(), topicId);
                }

                mKeywordSearchLoaded = true;
            }
        }

        bool isEmpty() const override
        {
            MWBase::Journal* journal = MWBase::Environment::get().getJournal();
            return journal->begin() == journal->end();
        }

        template <typename EntryType, typename Interface>
        struct BaseEntry : Interface
        {
            const EntryType* mEntry;
            JournalViewModelImpl const* mModel;

            BaseEntry(JournalViewModelImpl const* model, const EntryType& entry)
                : mEntry(&entry)
                , mModel(model)
            {
            }

            virtual ~BaseEntry() = default;

            mutable bool mLoaded{ false };
            mutable std::string mText;

            struct Token
            {
                size_t mStart;
                size_t mEnd;
                const MWDialogue::Topic* mTopic;
            };

            mutable std::vector<Token> mTokens;

            void ensureLoaded() const
            {
                if (!mLoaded)
                {
                    mModel->ensureKeyWordSearchLoaded();

                    const Translation::Storage& storage
                        = MWBase::Environment::get().getWindowManager()->getTranslationDataStorage();

                    const std::string& text = mEntry->getText();

                    std::string processedText;
                    std::vector<MWDialogue::KeywordSearch::Match> rawMatches;
                    mModel->mKeywordSearch.parseHyperText(text, storage, processedText, rawMatches);
                    MWDialogue::KeywordSearch::removeUnusedPostfix(processedText, rawMatches);

                    mText = std::move(processedText);
                    mTokens.reserve(rawMatches.size());

                    for (const auto& match : rawMatches)
                    {
                        size_t start = static_cast<size_t>(match.mBeg - mText.cbegin());
                        size_t end = static_cast<size_t>(match.mEnd - mText.cbegin());
                        const MWDialogue::Topic* topic = nullptr;
                        auto it = mModel->mTopics.find(match.mTopicId);
                        if (it != mModel->mTopics.end())
                            topic = it->second;
                        if (topic)
                            mTokens.push_back({ start, end, topic });
                    }

                    mLoaded = true;
                }
            }

            std::string_view body() const override
            {
                ensureLoaded();
                return mText;
            }

            void visitSpans(
                std::function<void(const MWDialogue::Topic*, size_t, size_t)> visitor) const override
            {
                ensureLoaded();

                size_t i = 0;
                for (const Token& token : mTokens)
                {
                    if (i < token.mStart)
                        visitor(nullptr, i, token.mStart);
                    visitor(token.mTopic, token.mStart, token.mEnd);
                    i = token.mEnd;
                }
                if (i < mText.size())
                    visitor(nullptr, i, mText.size());
            }
        };

        void visitQuestNames(bool active_only, std::function<void(std::string_view, bool)> visitor) const override
        {
            MWBase::Journal* journal = MWBase::Environment::get().getJournal();

            std::set<std::string, std::less<>> visitedQuests;

            for (MWBase::Journal::TQuestIter i = journal->questBegin(); i != journal->questEnd(); ++i)
            {
                const MWDialogue::Quest& quest = i->second;

                bool isFinished = false;
                for (MWBase::Journal::TQuestIter j = journal->questBegin(); j != journal->questEnd(); ++j)
                {
                    if (quest.getName() == j->second.getName() && j->second.isFinished())
                        isFinished = true;
                }

                if (active_only && isFinished)
                    continue;

                if (!quest.getName().empty())
                {
                    if (visitedQuests.find(quest.getName()) != visitedQuests.end())
                        continue;

                    visitor(quest.getName(), isFinished);

                    visitedQuests.emplace(quest.getName());
                }
            }
        }

        struct JournalEntryImpl : BaseEntry<MWDialogue::StampedJournalEntry, JournalEntry>
        {
            mutable std::string mTimestamp;

            JournalEntryImpl(JournalViewModelImpl const* model, const MWDialogue::StampedJournalEntry& entry)
                : BaseEntry(model, entry)
            {
            }

            std::string_view timestamp() const override
            {
                if (mTimestamp.empty())
                {
                    std::ostringstream os;

                    os << MWBase::Environment::get().getWorld()->getTimeManager()->getMonthName(mEntry->mMonth)
                       << " " << mEntry->mDayOfMonth
                       << "\xe6\x97\xa5 (\xe7\xac\xac" << mEntry->mDay << "\xe5\xa4\xa9)";

                    mTimestamp = os.str();
                }

                return mTimestamp;
            }
        };

        void visitJournalEntries(std::string_view questName,
            std::function<void(JournalEntry const&, const MWDialogue::Quest*)> visitor) const override
        {
            MWBase::Journal* journal = MWBase::Environment::get().getJournal();

            std::vector<MWDialogue::Quest const*> quests;
            for (MWBase::Journal::TQuestIter questIt = journal->questBegin(); questIt != journal->questEnd();
                 ++questIt)
            {
                if (questName.empty() || Misc::StringUtils::ciEqual(questIt->second.getName(), questName))
                    quests.push_back(&questIt->second);
            }

            for (MWBase::Journal::TEntryIter i = journal->begin(); i != journal->end(); ++i)
            {
                bool visited = false;
                for (MWDialogue::Quest const* quest : quests)
                {
                    if (quest->getTopic() != i->mTopic)
                        continue;
                    for (MWDialogue::Topic::TEntryIter j = quest->begin(); j != quest->end(); ++j)
                    {
                        if (i->mInfoId == j->mInfoId)
                        {
                            visitor(JournalEntryImpl(this, *i), questName.empty() ? quest : nullptr);
                            visited = true;
                            break;
                        }
                    }
                }
                if (!visited && questName.empty())
                    visitor(JournalEntryImpl(this, *i), nullptr);
            }
        }

        void visitTopicName(
            const MWDialogue::Topic& topic, std::function<void(std::string_view)> visitor) const override
        {
            visitor(topic.getName());
        }

        void visitTopicNamesStartingWith(
            Utf8Stream::UnicodeChar character, std::function<void(std::string_view)> visitor) const override
        {
            MWBase::Journal* journal = MWBase::Environment::get().getJournal();

            character = Utf8Stream::toLowerUtf8(character);
            for (MWBase::Journal::TTopicIter i = journal->topicBegin(); i != journal->topicEnd(); ++i)
            {
                Utf8Stream stream(i->second.getName());
                Utf8Stream::UnicodeChar first = Utf8Stream::toLowerUtf8(stream.peek());

                if (Translation::isFirstChar(first, (char)character))
                    visitor(i->second.getName());
            }
        }

        struct TopicEntryImpl : BaseEntry<MWDialogue::Entry, TopicEntry>
        {
            MWDialogue::Topic const& mTopic;

            TopicEntryImpl(
                JournalViewModelImpl const* model, MWDialogue::Topic const& topic, const MWDialogue::Entry& entry)
                : BaseEntry(model, entry)
                , mTopic(topic)
            {
            }

            std::string_view source() const override { return mEntry->mActorName; }
        };

        void visitTopicEntries(
            const MWDialogue::Topic& topic, std::function<void(TopicEntry const&)> visitor) const override
        {
            for (MWDialogue::Topic::TEntryIter i = topic.begin(); i != topic.end(); ++i)
                visitor(TopicEntryImpl(this, topic, *i));
        }
    };

    JournalViewModel::Ptr JournalViewModel::create()
    {
        return std::make_shared<JournalViewModelImpl>();
    }

}
