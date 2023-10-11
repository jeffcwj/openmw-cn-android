#ifndef OPENMW_COMPONENTS_SCENEUTIL_ANIMBLENDRULES_HPP
#define OPENMW_COMPONENTS_SCENEUTIL_ANIMBLENDRULES_HPP

#include <map>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <osg/Object>

#include <components/debug/debuglog.hpp>
#include <components/files/configfileparser.hpp>
#include <components/files/conversion.hpp>
#include <components/sceneutil/controller.hpp>
#include <components/sceneutil/textkeymap.hpp>
#include <components/vfs/manager.hpp>
#include <iterator>
#include <yaml-cpp/yaml.h>

namespace SceneUtil
{
    class AnimBlendRules : osg::Object
    {
    public:
        AnimBlendRules();
        AnimBlendRules(const VFS::Manager* vfs, std::string configpath);
        AnimBlendRules(
            const VFS::Manager* vfs, std::shared_ptr<AnimBlendRules> const fallbackRules, std::string configpath);
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

        void init(const VFS::Manager* vfs, std::string configpath);

        std::vector<BlendRule> parseYaml(std::string rawYaml, std::string path);

        std::optional<BlendRule> findBlendingRule(
            std::string fromGroup, std::string fromKey, std::string toGroup, std::string toKey);

        std::vector<BlendRule> getRules() const { return mRules; }

        bool isValid() const { return mRules.size() > 0; }

    private:
        std::string mConfigPath;
        std::vector<BlendRule> mRules;
    };
}

#endif
