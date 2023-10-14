#include "animblendrules.hpp"

namespace SceneUtil
{
    using BlendRule = AnimBlendRules::BlendRule;

    // String utilities
    std::string trimSpaces(std::string s)
    {
        std::string::size_type n, n2;
        n = s.find_first_not_of(" \t\r\n");
        if (n == std::string::npos)
            return std::string();
        else
        {
            n2 = s.find_last_not_of(" \t\r\n");
            return s.substr(n, n2 - n + 1);
        }
    }

    std::string toLower(std::string s)
    {
        std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });
        return s;
    }
    //

    AnimBlendRules::AnimBlendRules() {}

    AnimBlendRules::AnimBlendRules(const AnimBlendRules& copy, const osg::CopyOp& copyop)
        : mConfigPath(copy.mConfigPath)
        , mRules(copy.mRules)
    {
    }

    AnimBlendRules::AnimBlendRules(const VFS::Manager* vfs, std::string configpath)
        : mConfigPath(configpath)
    {
        init(vfs, configpath);
    }

    void AnimBlendRules::init(const VFS::Manager* vfs, std::string configpath)
    {
        if (configpath.find_first_of(".yaml") == std::string::npos || !vfs->exists(configpath))
            return;

        // Retrieving animation rules
        Log(Debug::Info) << "Loading animation blending YAML '" << configpath << "'.";
        std::string rawYaml(std::istreambuf_iterator<char>(*vfs->get(configpath)), {});
        auto rules = parseYaml(rawYaml, configpath);

        // Concat rules together, local rules should come last since the first rule matching from the end takes
        // priority
        mRules = rules;
    }

    void AnimBlendRules::addOverrideRules(const AnimBlendRules& overrideRules)
    {
        auto rules = overrideRules.getRules();
        mRules.insert(mRules.end(), rules.begin(), rules.end());
    }

    std::vector<BlendRule> AnimBlendRules::parseYaml(std::string rawYaml, std::string path)
    {

        std::vector<BlendRule> rules;

        YAML::Node root = YAML::Load(rawYaml);

        if (!root.IsDefined() || root.IsNull() || root.IsScalar())
        {
            Log(Debug::Warning) << "Warning: Can't parse YAML file '" << path
                                << "'. Check that it's a valid YAML file.";
            return rules;
        }

        if (root["blending_rules"])
        {
            for (const auto& it : root["blending_rules"])
            {
                if (it["from"] && it["to"] && it["duration"] && it["easing"])
                {
                    auto fromNames = BlendRule::parseFullName(it["from"].as<std::string>());
                    auto toNames = BlendRule::parseFullName(it["to"].as<std::string>());

                    BlendRule ruleObj = {
                        .mFromGroup = fromNames.first,
                        .mFromKey = fromNames.second,
                        .mToGroup = toNames.first,
                        .mToKey = toNames.second,
                        .mDuration = it["duration"].as<float>(),
                        .mEasing = it["easing"].as<std::string>(),
                    };

                    rules.emplace_back(ruleObj);
                }
                else
                {
                    Log(Debug::Warning) << "Warning: Blending rule '"
                                        << (it["from"] ? it["from"].as<std::string>() : "undefined") << "->"
                                        << (it["to"] ? it["to"].as<std::string>() : "undefined")
                                        << "' is missing some properties. File: '" << path << "'.";
                }
            }
        }
        else
        {
            Log(Debug::Warning) << "Warning: 'blending_rules' object not found in '" << path << "' file!";
        }

        return rules;
    }

    inline bool AnimBlendRules::fitsRuleString(std::string str, std::string ruleStr) const
    {
        // A wildcard only supported in the beginning or the end of the rule string in hopes that this will be more
        // performant. And most likely this kind of support is enough.
        return ruleStr == "*" || str == ruleStr || (ruleStr.starts_with("*") && str.ends_with(ruleStr.substr(1)))
            || (ruleStr.ends_with("*") && str.starts_with(ruleStr.substr(0, ruleStr.length() - 1)));
    }

    std::optional<BlendRule> AnimBlendRules::findBlendingRule(
        std::string fromGroup, std::string fromKey, std::string toGroup, std::string toKey) const
    {
        fromGroup = toLower(fromGroup);
        fromKey = toLower(fromKey);
        toGroup = toLower(toGroup);
        toKey = toLower(toKey);
        for (auto rule = mRules.rbegin(); rule != mRules.rend(); ++rule)
        {
            // TO DO: Also allow for partial wildcards at the end of groups and keys via std::string startswith method
            bool fromMatch = false;
            bool toMatch = false;

            // Pseudocode:
            // If not a wildcard and found a wildcard
            // starts with substr(0,wildcard)

            if (fitsRuleString(fromGroup, rule->mFromGroup)
                && (fitsRuleString(fromKey, rule->mFromKey) || rule->mFromKey == ""))
            {
                fromMatch = true;
            }

            if ((fitsRuleString(toGroup, rule->mToGroup) || (rule->mToGroup == "$" && toGroup == fromGroup))
                && (fitsRuleString(toKey, rule->mToKey) || rule->mToKey == ""))
            {
                toMatch = true;
            }

            if (fromMatch && toMatch)
                return std::make_optional<BlendRule>(*rule);
        }

        return std::nullopt;
    }

    std::pair<std::string, std::string> BlendRule::parseFullName(std::string full)
    {
        std::string group = "";
        std::string key = "";
        size_t delimiterInd = full.find(":");

        if (delimiterInd == std::string::npos)

            group = toLower(trimSpaces(full));
        else
        {
            group = toLower(trimSpaces(full.substr(0, delimiterInd)));
            key = toLower(trimSpaces(full.substr(delimiterInd + 1)));
        }
        return std::make_pair(group, key);
    }

}
