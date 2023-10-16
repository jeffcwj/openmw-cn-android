#ifndef OPENMW_COMPONENTS_SCENEUTIL_ANIMBLENDRULES_HPP
#define OPENMW_COMPONENTS_SCENEUTIL_ANIMBLENDRULES_HPP

#include <iterator>
#include <map>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <yaml-cpp/yaml.h>

#include <osg/Object>

#include <components/debug/debuglog.hpp>
#include <components/files/configfileparser.hpp>
#include <components/files/conversion.hpp>
#include <components/misc/strings/algorithm.hpp>
#include <components/misc/strings/lower.hpp>
#include <components/sceneutil/controller.hpp>
#include <components/sceneutil/textkeymap.hpp>
#include <components/vfs/manager.hpp>

using namespace Misc::StringUtils;

namespace SceneUtil
{
    class AnimBlendRules : public osg::Object
    {
    public:
        AnimBlendRules();
        AnimBlendRules(const VFS::Manager* vfs, std::string kfpath);
        AnimBlendRules(const AnimBlendRules& copy, const osg::CopyOp& copyop);

        META_Object(SceneUtil, AnimBlendRules)

        struct BlendRule
        {
            std::string mFromGroup;
            std::string mFromKey;
            std::string mToGroup;
            std::string mToKey;
            float mDuration;
            std::string mEasing;

            static std::pair<std::string, std::string> parseFullName(std::string fullName);
        };

        void init(const VFS::Manager* vfs, std::string kfpath);

        void addOverrideRules(const AnimBlendRules& overrideRules);

        std::vector<BlendRule> parseYaml(const std::string& rawYaml, const std::string& path);

        inline bool fitsRuleString(const std::string& str, const std::string& ruleStr) const;

        std::optional<BlendRule> findBlendingRule(
            std::string fromGroup, std::string fromKey, std::string toGroup, std::string toKey) const;

        std::vector<BlendRule> getRules() const { return mRules; }

        bool isValid() const { return mRules.size() > 0; }

    private:
        std::string mConfigPath;
        std::vector<BlendRule> mRules;
    };
}

#endif
