#ifndef GAME_MWDIALOGUE_KEYWORDSEARCH_H
#define GAME_MWDIALOGUE_KEYWORDSEARCH_H

#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace Translation
{
    class Storage;
}

namespace MWDialogue
{

    class KeywordSearch
    {
        struct Entry
        {
            std::string mKeyword;
            std::string mTopicId;
            std::map<char, Entry> mChildren;
        };

        void buildTrie(std::string_view keyword, std::string topicId, size_t depth, Entry& entry);
        Entry mRoot;

    public:
        using Point = std::string::const_iterator;

        struct Match
        {
            Point mBeg;
            Point mEnd;
            std::string mTopicId;
            bool mExplicit{ false };

            std::string_view getDisplayName() const;
        };

        void seed(std::string_view keyword, std::string topicId);
        void clear();
        void highlightKeywords(Point beg, Point end, std::vector<Match>& out) const;

        /// 解析 @link# 超链接标记并做 keyword 高亮。
        /// 输入 text 是原始带 @link# 的文本，输出 processedText 是去除标记后的纯文本，
        /// 返回的 Match 迭代器指向 processedText。
        void parseHyperText(const std::string& text, const Translation::Storage& storage,
            std::string& processedText, std::vector<Match>& matches) const;

        static void removeUnusedPostfix(std::string& text, std::vector<Match>& matches);
    };

}

#endif
