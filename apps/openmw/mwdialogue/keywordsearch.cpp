#include "keywordsearch.hpp"

#include <algorithm>
#include <set>
#include <stdexcept>

#include <components/misc/strings/algorithm.hpp>
#include <components/misc/strings/lower.hpp>
#include <components/translation/translation.hpp>

namespace
{
    std::string_view removePseudoAsterisks(std::string_view phrase)
    {
        std::size_t lastNonAsteriskPos = phrase.find_last_not_of('\x7F');
        if (lastNonAsteriskPos == std::string_view::npos)
            return {};
        return phrase.substr(0, lastNonAsteriskPos + 1);
    }
}

namespace MWDialogue
{

    void KeywordSearch::seed(std::string_view keyword, std::string topicId)
    {
        if (keyword.empty())
            return;
        buildTrie(keyword, std::move(topicId), 0, mRoot);
    }

    void KeywordSearch::buildTrie(
        std::string_view keyword, std::string topicId, size_t depth, Entry& entry)
    {
        if (depth == keyword.size())
        {
            entry.mKeyword.assign(keyword);
            entry.mTopicId = std::move(topicId);
            return;
        }
        char c = Misc::StringUtils::toLower(keyword[depth]);
        auto it = entry.mChildren.find(c);
        if (it == entry.mChildren.end())
            it = entry.mChildren.emplace(c, Entry{}).first;
        buildTrie(keyword, std::move(topicId), depth + 1, it->second);
    }

    void KeywordSearch::clear()
    {
        mRoot = Entry{};
    }

    void KeywordSearch::highlightKeywords(Point beg, Point end, std::vector<Match>& out) const
    {
        static const std::string wordSeparators = " \t\n\r.,;:!?\"'()[]{}/-+*=#$%&^~<>|\\@";

        out.clear();

        for (Point i = beg; i != end;)
        {
            const Entry* node = &mRoot;
            const Entry* lastMatch = nullptr;
            Point matchEnd = i;

            for (Point j = i; j != end;)
            {
                unsigned char ch = static_cast<unsigned char>(*j);
                // 3/4 字节 UTF-8 首字节视为字边界（中日韩等字符）
                if (ch >= 0xE0)
                {
                    if (!node->mKeyword.empty())
                    {
                        lastMatch = node;
                        matchEnd = j;
                    }
                    break;
                }
                char c = Misc::StringUtils::toLower(static_cast<char>(ch));
                auto it = node->mChildren.find(c);
                if (it == node->mChildren.end())
                    break;
                node = &it->second;
                ++j;
                if (!node->mKeyword.empty())
                {
                    lastMatch = node;
                    matchEnd = j;
                }
            }

            if (lastMatch)
            {
                bool leftBoundary = (i == beg);
                if (!leftBoundary)
                {
                    unsigned char prev = static_cast<unsigned char>(*(i - 1));
                    if (prev >= 0xE0
                        || wordSeparators.find(static_cast<char>(prev)) != std::string::npos)
                        leftBoundary = true;
                }

                bool rightBoundary = (matchEnd == end);
                if (!rightBoundary)
                {
                    unsigned char next = static_cast<unsigned char>(*matchEnd);
                    if (next >= 0xE0
                        || wordSeparators.find(static_cast<char>(next)) != std::string::npos)
                        rightBoundary = true;
                }

                if (leftBoundary && rightBoundary)
                {
                    Match m;
                    m.mBeg = i;
                    m.mEnd = matchEnd;
                    m.mTopicId = lastMatch->mTopicId;
                    m.mExplicit = false;
                    out.push_back(std::move(m));
                    i = matchEnd;
                    continue;
                }
            }

            // 跳过当前字符（处理多字节 UTF-8）
            unsigned char lead = static_cast<unsigned char>(*i);
            if (lead < 0x80)
                ++i;
            else if (lead < 0xC0)
                ++i;
            else if (lead < 0xE0)
                i += std::min<ptrdiff_t>(2, end - i);
            else if (lead < 0xF0)
                i += std::min<ptrdiff_t>(3, end - i);
            else
                i += std::min<ptrdiff_t>(4, end - i);
        }
    }

    void KeywordSearch::parseHyperText(const std::string& text,
        const Translation::Storage& storage, std::string& processedText,
        std::vector<Match>& matches) const
    {
        matches.clear();
        processedText.clear();
        processedText.reserve(text.size());

        // 第一遍：解析 @link# 标记，生成 processedText 和显式链接偏移量
        struct ExplicitLink
        {
            size_t mStart;
            size_t mEnd;
            std::string mTopicId;
        };
        std::vector<ExplicitLink> explicitLinks;

        size_t i = 0;
        while (i < text.size())
        {
            if (text[i] == '@')
            {
                size_t hashPos = text.find('#', i + 1);
                if (hashPos != std::string::npos)
                {
                    std::string link = text.substr(i + 1, hashPos - i - 1);
                    const char pseudoAsterisk = 127;
                    std::replace(link.begin(), link.end(), pseudoAsterisk, '*');

                    std::string_view topicName = storage.topicStandardForm(link);
                    std::string_view displayName = removePseudoAsterisks(link);

                    size_t start = processedText.size();
                    processedText.append(displayName);
                    size_t stop = processedText.size();

                    // 在 trie 中查找此 topic 的 topicId
                    std::string topicId;
                    {
                        const Entry* node = &mRoot;
                        const Entry* lastFound = nullptr;
                        for (size_t k = 0; k < topicName.size(); ++k)
                        {
                            char c = Misc::StringUtils::toLower(topicName[k]);
                            auto it = node->mChildren.find(c);
                            if (it == node->mChildren.end())
                                break;
                            node = &it->second;
                            if (!node->mTopicId.empty())
                                lastFound = node;
                        }
                        if (lastFound)
                            topicId = lastFound->mTopicId;
                    }

                    if (!topicId.empty())
                    {
                        ExplicitLink el;
                        el.mStart = start;
                        el.mEnd = stop;
                        el.mTopicId = std::move(topicId);
                        explicitLinks.push_back(std::move(el));
                    }

                    i = hashPos + 1;
                    continue;
                }
            }
            processedText.push_back(text[i]);
            ++i;
        }

        // 第二遍：对 processedText 做 keyword 高亮
        std::vector<Match> keywordMatches;
        highlightKeywords(processedText.cbegin(), processedText.cend(), keywordMatches);

        // 合并：显式链接优先，keyword 匹配不与显式链接重叠
        for (auto& km : keywordMatches)
        {
            size_t kmStart = static_cast<size_t>(km.mBeg - processedText.cbegin());
            size_t kmEnd = static_cast<size_t>(km.mEnd - processedText.cbegin());
            bool overlaps = false;
            for (const auto& el : explicitLinks)
            {
                if (kmStart < el.mEnd && kmEnd > el.mStart)
                {
                    overlaps = true;
                    break;
                }
            }
            if (!overlaps)
                matches.push_back(std::move(km));
        }

        // 添加显式链接到 matches
        for (auto& el : explicitLinks)
        {
            Match m;
            m.mBeg = processedText.cbegin() + static_cast<ptrdiff_t>(el.mStart);
            m.mEnd = processedText.cbegin() + static_cast<ptrdiff_t>(el.mEnd);
            m.mTopicId = std::move(el.mTopicId);
            m.mExplicit = true;
            matches.push_back(std::move(m));
        }

        // 按位置排序
        std::sort(matches.begin(), matches.end(),
            [&processedText](const Match& a, const Match& b)
            { return (a.mBeg - processedText.cbegin()) < (b.mBeg - processedText.cbegin()); });
    }

    std::string_view KeywordSearch::Match::getDisplayName() const
    {
        return std::string_view(&*mBeg, mEnd - mBeg);
    }

    void KeywordSearch::removeUnusedPostfix(std::string& text, std::vector<Match>& matches)
    {
        // 从后往前处理，避免偏移量错乱（string::erase 会使迭代器失效）
        // 先转换为偏移量数组
        struct MatchOffset
        {
            size_t mBeg;
            size_t mEnd;
        };
        std::vector<MatchOffset> offsets(matches.size());
        for (size_t idx = 0; idx < matches.size(); ++idx)
        {
            offsets[idx].mBeg = static_cast<size_t>(matches[idx].mBeg - text.cbegin());
            offsets[idx].mEnd = static_cast<size_t>(matches[idx].mEnd - text.cbegin());
        }

        for (int idx = static_cast<int>(offsets.size()) - 1; idx >= 0; --idx)
        {
            size_t begOff = offsets[idx].mBeg;
            size_t endOff = offsets[idx].mEnd;
            size_t len = endOff - begOff;
            if (len == 0)
                continue;

            // 查找最后一个非 '*' 字符
            size_t lastNonAsterisk = std::string::npos;
            for (size_t k = endOff; k > begOff; --k)
            {
                if (text[k - 1] != '*')
                {
                    lastNonAsterisk = k - 1;
                    break;
                }
            }
            if (lastNonAsterisk == std::string::npos || lastNonAsterisk < begOff)
                continue;

            size_t newEnd = lastNonAsterisk + 1;
            if (newEnd < endOff)
            {
                size_t removeCount = endOff - newEnd;
                text.erase(newEnd, removeCount);
                offsets[idx].mEnd = newEnd;

                // 调整前面（索引更小）的 match 偏移量
                // 不需要——从后往前删除，前面的偏移量不受影响
            }
        }

        // 重建所有 match 的迭代器
        for (size_t idx = 0; idx < matches.size(); ++idx)
        {
            matches[idx].mBeg = text.cbegin() + static_cast<ptrdiff_t>(offsets[idx].mBeg);
            matches[idx].mEnd = text.cbegin() + static_cast<ptrdiff_t>(offsets[idx].mEnd);
        }
    }

}
