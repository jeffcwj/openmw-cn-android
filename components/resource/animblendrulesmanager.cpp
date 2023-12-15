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

#include <components/resource/scenemanager.hpp>

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

    osg::ref_ptr<const AnimBlendRules> AnimBlendRulesManager::getInstance(
        const std::string& path, const std::string& overridePath)
    {
        // Note: Providing a non-existing path but an existing overridePath is not supported!
        auto tmpl = get(path);
        if (!tmpl)
            return nullptr;

        // Create an instance based on template and store template reference inside so the template will not be removed
        // from cache
        osg::ref_ptr<SceneUtil::AnimBlendRules> blendRules(new AnimBlendRules(*tmpl, osg::CopyOp::SHALLOW_COPY));
        blendRules->getOrCreateUserDataContainer()->addUserObject(new Resource::TemplateRef(tmpl));

        if (!overridePath.empty())
        {
            auto blendRuleOverrides = get(overridePath);
            if (blendRuleOverrides)
            {
                blendRules->addOverrideRules(*blendRuleOverrides);
            }
            blendRules->getOrCreateUserDataContainer()->addUserObject(new Resource::TemplateRef(blendRuleOverrides));
        }

        return blendRules;
    }

    osg::ref_ptr<const AnimBlendRules> AnimBlendRulesManager::get(const std::string& path)
    {
        const std::string normalized = VFS::Path::normalizeFilename(path);

        std::optional<osg::ref_ptr<osg::Object>> obj = mCache->getRefFromObjectCacheOrNone(normalized);
        if (obj && obj.value() && obj.value().get())
            // Found cached rules for this VFS path!
            return osg::ref_ptr<AnimBlendRules>(static_cast<AnimBlendRules*>(obj.value().get()));
        else if (obj && !obj.value())
        {
            // Found nullptr. This VFS path was checked before and it was empty.
            return osg::ref_ptr<AnimBlendRules>(static_cast<AnimBlendRules*>(obj.value().get()));
        }
        else if (!obj)
        {
            osg::ref_ptr<AnimBlendRules> blendRules(new AnimBlendRules(mVfs, path));
            if (blendRules->getRules().size() == 0)
            {
                // No blend rules were found in VFS, cache a nullptr.
                osg::ref_ptr<AnimBlendRules> nullRules = nullptr;
                mCache->addEntryToObjectCache(normalized, nullRules);
                // To avoid confusion - never return blend rules with 0 rules
                return nullRules;
            }
            else
            {
                // Blend rules were found in VFS, cache them.
                mCache->addEntryToObjectCache(normalized, blendRules);
                return blendRules;
            }
        }
    }

    void AnimBlendRulesManager::reportStats(unsigned int frameNumber, osg::Stats* stats) const
    {
        stats->setAttribute(frameNumber, "Blending Rules", mCache->getCacheSize());
    }

}
