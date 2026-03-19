#include "journalbooks.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwdialogue/quest.hpp"
#include "../mwdialogue/topic.hpp"

#include <components/misc/utf8stream.hpp>
#include <components/settings/values.hpp>

#include "textcolours.hpp"

namespace
{
    struct AddContent
    {
        std::shared_ptr<MWGui::BookTypesetter> mTypesetter;
        MWGui::BookTypesetter::Style* mBodyStyle;

        AddContent(std::shared_ptr<MWGui::BookTypesetter> typesetter, MWGui::BookTypesetter::Style* body_style)
            : mTypesetter(std::move(typesetter))
            , mBodyStyle(body_style)
        {
        }
    };

    struct AddSpan : AddContent
    {
        AddSpan(std::shared_ptr<MWGui::BookTypesetter> typesetter, MWGui::BookTypesetter::Style* body_style)
            : AddContent(std::move(typesetter), body_style)
        {
        }

        void operator()(const MWDialogue::Topic* topic, size_t begin, size_t end)
        {
            MWGui::BookTypesetter::Style* style = mBodyStyle;

            const MWGui::TextColours& textColours = MWBase::Environment::get().getWindowManager()->getTextColours();
            if (topic)
            {
                intptr_t id = reinterpret_cast<intptr_t>(topic);
                style = mTypesetter->createHotStyle(mBodyStyle, textColours.journalLink, textColours.journalLinkOver,
                    textColours.journalLinkPressed, id);
            }

            mTypesetter->write(style, begin, end);
        }
    };

    struct AddEntry
    {
        std::shared_ptr<MWGui::BookTypesetter> mTypesetter;
        MWGui::BookTypesetter::Style* mBodyStyle;

        AddEntry(std::shared_ptr<MWGui::BookTypesetter> typesetter, MWGui::BookTypesetter::Style* body_style)
            : mTypesetter(std::move(typesetter))
            , mBodyStyle(body_style)
        {
        }

        void operator()(MWGui::JournalViewModel::Entry const& entry)
        {
            mTypesetter->addContent(entry.body());

            entry.visitSpans(AddSpan(mTypesetter, mBodyStyle));
        }
    };

    struct AddJournalEntry : AddEntry
    {
        bool mAddHeader;
        MWGui::BookTypesetter::Style* mHeaderStyle;

        AddJournalEntry(std::shared_ptr<MWGui::BookTypesetter> typesetter, MWGui::BookTypesetter::Style* body_style,
            MWGui::BookTypesetter::Style* header_style, bool add_header)
            : AddEntry(std::move(typesetter), body_style)
            , mAddHeader(add_header)
            , mHeaderStyle(header_style)
        {
        }

        void operator()(MWGui::JournalViewModel::JournalEntry const& entry, const MWDialogue::Quest* quest)
        {
            if (mAddHeader)
            {
                mTypesetter->write(mHeaderStyle, entry.timestamp());
                mTypesetter->lineBreak();
            }

            if (quest)
            {
                auto questName = quest->getName();
                if (!questName.empty())
                {
                    intptr_t id = -reinterpret_cast<intptr_t>(quest);
                    auto style = mTypesetter->createHotStyle(mBodyStyle, MyGUI::Colour(0.60f, 0.00f, 0.00f),
                        MyGUI::Colour(0.70f, 0.10f, 0.10f), MyGUI::Colour(0.80f, 0.20f, 0.20f), id);
                    mTypesetter->write(style, questName);
                    mTypesetter->lineBreak();
                }
            }

            AddEntry::operator()(entry);

            mTypesetter->sectionBreak(10);
        }
    };

    struct AddTopicEntry : AddEntry
    {
        const MWGui::TypesetBook::Content* mContentHandle;
        MWGui::BookTypesetter::Style* mHeaderStyle;

        AddTopicEntry(std::shared_ptr<MWGui::BookTypesetter> typesetter, MWGui::BookTypesetter::Style* body_style,
            MWGui::BookTypesetter::Style* header_style, const MWGui::TypesetBook::Content* contentHandle)
            : AddEntry(std::move(typesetter), body_style)
            , mContentHandle(contentHandle)
            , mHeaderStyle(header_style)
        {
        }

        void operator()(MWGui::JournalViewModel::TopicEntry const& entry)
        {
            mTypesetter->write(mBodyStyle, entry.source());
            mTypesetter->write(mBodyStyle, 0, 3); // begin quote

            AddEntry::operator()(entry);

            mTypesetter->selectContent(mContentHandle);
            mTypesetter->write(mBodyStyle, 2, 3); // end quote

            mTypesetter->sectionBreak(10);
        }
    };

    struct AddTopicName : AddContent
    {
        AddTopicName(std::shared_ptr<MWGui::BookTypesetter> typesetter, MWGui::BookTypesetter::Style* style)
            : AddContent(std::move(typesetter), style)
        {
        }

        void operator()(std::string_view topicName)
        {
            mTypesetter->write(mBodyStyle, topicName);
            mTypesetter->sectionBreak(10);
        }
    };

    struct AddQuestName : AddContent
    {
        AddQuestName(std::shared_ptr<MWGui::BookTypesetter> typesetter, MWGui::BookTypesetter::Style* style)
            : AddContent(std::move(typesetter), style)
        {
        }

        void operator()(std::string_view questName)
        {
            mTypesetter->write(mBodyStyle, questName);
            mTypesetter->sectionBreak(10);
        }
    };
}

namespace MWGui
{

    JournalBooks::JournalBooks(JournalViewModel::Ptr model, ToUTF8::FromType encoding)
        : mModel(std::move(model))
        , mEncoding(encoding)
        , mIndexPagesCount(0)
    {
    }

    std::shared_ptr<TypesetBook> JournalBooks::createEmptyJournalBook()
    {
        std::shared_ptr<BookTypesetter> typesetter = createTypesetter();

        BookTypesetter::Style* header = typesetter->createStyle({}, journalHeaderColour);
        BookTypesetter::Style* body = typesetter->createStyle({}, MyGUI::Colour::Black);

        typesetter->write(header, "You have no journal entries!");
        typesetter->lineBreak();
        typesetter->write(body, "You should have gone though the starting quest and got an initial quest.");

        return typesetter->complete();
    }

    std::shared_ptr<TypesetBook> JournalBooks::createJournalBook()
    {
        std::shared_ptr<BookTypesetter> typesetter = createTypesetter();

        BookTypesetter::Style* header = typesetter->createStyle({}, journalHeaderColour);
        BookTypesetter::Style* body = typesetter->createStyle({}, MyGUI::Colour::Black);

        mModel->visitJournalEntries({}, AddJournalEntry(typesetter, body, header, true));

        return typesetter->complete();
    }

    std::shared_ptr<TypesetBook> JournalBooks::createTopicBook(const MWDialogue::Topic& topic)
    {
        std::shared_ptr<BookTypesetter> typesetter = createTypesetter();

        BookTypesetter::Style* header = typesetter->createStyle({}, journalHeaderColour);
        BookTypesetter::Style* body = typesetter->createStyle({}, MyGUI::Colour::Black);

        mModel->visitTopicName(topic, AddTopicName(typesetter, header));

        const TypesetBook::Content* contentHandle = typesetter->addContent(": \"");

        mModel->visitTopicEntries(topic, AddTopicEntry(typesetter, body, header, contentHandle));

        return typesetter->complete();
    }

    std::shared_ptr<TypesetBook> JournalBooks::createQuestBook(std::string_view questName)
    {
        std::shared_ptr<BookTypesetter> typesetter = createTypesetter();

        BookTypesetter::Style* header = typesetter->createStyle({}, journalHeaderColour);
        BookTypesetter::Style* body = typesetter->createStyle({}, MyGUI::Colour::Black);

        AddQuestName addName(typesetter, header);
        addName(questName);

        mModel->visitJournalEntries(questName, AddJournalEntry(typesetter, body, header, true));

        return typesetter->complete();
    }

    std::shared_ptr<TypesetBook> JournalBooks::createTopicIndexBook()
    {
        bool isRussian = (mEncoding == ToUTF8::WINDOWS_1251);

        std::shared_ptr<BookTypesetter> typesetter
            = isRussian ? createCyrillicJournalIndex() : createLatinJournalIndex();

        return typesetter->complete();
    }

    std::shared_ptr<BookTypesetter> JournalBooks::createLatinJournalIndex()
    {
        std::shared_ptr<BookTypesetter> typesetter = BookTypesetter::create(92, 260);

        typesetter->setSectionAlignment(BookTypesetter::AlignCenter);

        // Latin journal index always has two columns for now.
        mIndexPagesCount = 2;

        char ch = 'A';
        std::string buffer;

        BookTypesetter::Style* body = typesetter->createStyle({}, MyGUI::Colour::Black);
        for (int i = 0; i < 26; ++i)
        {
            buffer = "( ";
            buffer += ch;
            buffer += " )";

            const MWGui::TextColours& textColours = MWBase::Environment::get().getWindowManager()->getTextColours();
            BookTypesetter::Style* style = typesetter->createHotStyle(body, textColours.journalTopic,
                textColours.journalTopicOver, textColours.journalTopicPressed, (Utf8Stream::UnicodeChar)ch);

            if (i == 13)
                typesetter->sectionBreak();

            typesetter->write(style, buffer);
            typesetter->lineBreak();

            ch++;
        }

        return typesetter;
    }

    std::shared_ptr<BookTypesetter> JournalBooks::createCyrillicJournalIndex()
    {
        std::shared_ptr<BookTypesetter> typesetter = BookTypesetter::create(92, 260);

        typesetter->setSectionAlignment(BookTypesetter::AlignCenter);

        BookTypesetter::Style* body = typesetter->createStyle({}, MyGUI::Colour::Black);

        // for small font size split alphabet to two columns (2x15 characters), for big font size split it to three
        // columns (3x10 characters).
        int sectionBreak = 10;
        mIndexPagesCount = 3;
        if (Settings::gui().mFontSize < 18)
        {
            sectionBreak = 15;
            mIndexPagesCount = 2;
        }

        unsigned char ch[3] = { 0xd0, 0x90, 0x00 }; // CYRILLIC CAPITAL A is a 0xd090 in UTF-8

        std::string buffer;

        for (int i = 0; i < 32; ++i)
        {
            buffer = "( ";
            buffer += ch[0];
            buffer += ch[1];
            buffer += " )";

            Utf8Stream stream(ch, ch + 2);
            Utf8Stream::UnicodeChar first = stream.peek();

            const MWGui::TextColours& textColours = MWBase::Environment::get().getWindowManager()->getTextColours();
            BookTypesetter::Style* style = typesetter->createHotStyle(
                body, textColours.journalTopic, textColours.journalTopicOver, textColours.journalTopicPressed, first);

            ch[1]++;

            // Words can not be started with these characters
            if (i == 26 || i == 28)
                continue;

            if (i % sectionBreak == 0)
                typesetter->sectionBreak();

            typesetter->write(style, buffer);
            typesetter->lineBreak();
        }

        return typesetter;
    }

    std::shared_ptr<BookTypesetter> JournalBooks::createTypesetter()
    {
        // TODO: determine page size from layout...
        return BookTypesetter::create(240, 320);
    }

}
