#ifndef OPENMW_COMPONENTS_SCENEUTIL_ANIMBLENDRULES_HPP
#define OPENMW_COMPONENTS_SCENEUTIL_ANIMBLENDRULES_HPP

#include <optional>
#include <string>
#include <vector>

#include <osg/Object>

#include <components/vfs/manager.hpp>

namespace SceneUtil
{
    class AnimBlendRules : public osg::Object
    {
    public:
        AnimBlendRules() = default;
        AnimBlendRules(const VFS::Manager* vfs, const std::string& yamlpath);
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
        };

        void addOverrideRules(const AnimBlendRules& overrideRules);

        std::optional<BlendRule> findBlendingRule(
            std::string fromGroup, std::string fromKey, std::string toGroup, std::string toKey) const;

        const std::vector<BlendRule>& getRules() const { return mRules; }

    private:
        std::string mConfigPath;
        std::vector<BlendRule> mRules;

        void init(const VFS::Manager* vfs, std::string yamlpath);

        std::vector<BlendRule> parseYaml(const std::string& rawYaml, const std::string& path);

        inline bool fitsRuleString(const std::string& str, const std::string& ruleStr) const;
    };
}

#endif
