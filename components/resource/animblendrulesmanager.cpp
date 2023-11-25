#include "animblendrulesmanager.hpp"

#include <array>

#include <components/vfs/manager.hpp>

#include <osg/Stats>
#include <osgAnimation/Animation>
#include <osgAnimation/BasicAnimationManager>
#include <osgAnimation/Channel>

#include <components/debug/debuglog.hpp>
#include <components/misc/pathhelpers.hpp>

#include <components/sceneutil/osgacontroller.hpp>
#include <components/vfs/pathutil.hpp>

#include "objectcache.hpp"
#include "scenemanager.hpp"

namespace Resource
{
    using AnimBlendRules = SceneUtil::AnimBlendRules;

    AnimBlendRulesManager::AnimBlendRulesManager(const VFS::Manager* vfs, double expiryDelay)
        : ResourceManager(vfs, expiryDelay)
        , mVfs(vfs)
    {
    }

    osg::ref_ptr<const AnimBlendRules> AnimBlendRulesManager::get(
        const std::string& path, const std::string& overridePath)
    {
        osg::ref_ptr<SceneUtil::AnimBlendRules> blendRules(new AnimBlendRules(*get(path), osg::CopyOp::SHALLOW_COPY));
        auto blendRuleOverrides = get(overridePath);

        blendRules->addOverrideRules(*blendRuleOverrides);

        return blendRules;
    }

    osg::ref_ptr<const AnimBlendRules> AnimBlendRulesManager::get(const std::string& path)
    {
        const std::string normalized = VFS::Path::normalizeFilename(path);

        osg::ref_ptr<osg::Object> obj = mCache->getRefFromObjectCache(normalized);
        if (obj)
            return osg::ref_ptr<AnimBlendRules>(static_cast<AnimBlendRules*>(obj.get()));
        else
        {
            osg::ref_ptr<AnimBlendRules> blendRules(new AnimBlendRules(mVfs, path));
            mCache->addEntryToObjectCache(normalized, blendRules);

            return blendRules;
        }
    }

    void AnimBlendRulesManager::reportStats(unsigned int frameNumber, osg::Stats* stats) const
    {
        stats->setAttribute(frameNumber, "Blending Rules", mCache->getCacheSize());
    }

}
