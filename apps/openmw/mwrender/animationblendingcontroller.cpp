#include "animationblendingcontroller.hpp"
#include <string>
#include <vector>

namespace MWRender
{
    using AnimBlendRule = AnimationBlendingController::AnimBlendRule;
    using AnimStateData = AnimationBlendingController::AnimStateData;

    // AnimationBlendingController::AnimationBlendingController() {}

    AnimationBlendingController::AnimationBlendingController(
        osg::ref_ptr<KeyframeController> keyframeTrack, AnimStateData newState, std::vector<AnimBlendRule> blendRules)
    {
        SetKeyframeTrack(keyframeTrack, newState, blendRules);
        // Log(Debug::Info) << "AnimationBlendingController created for: " << keyframeTrack-> getName();
    }

    /*AnimationBlendingController::AnimationBlendingController(
        const AnimationBlendingController& copy, const osg::CopyOp& copyop)
        : osg::Object(copy, copyop)
    {
    }*/

    void AnimationBlendingController::SetKeyframeTrack(
        osg::ref_ptr<KeyframeController> kft, AnimStateData newState, std::vector<AnimBlendRule> blendRules)
    {
        // Default blend settings
        blendDuration = 0.22;
        easingFn = &Easings::sineOut;

        if (newState.groupname == "INIT_STATE")
            return;
        if (newState.groupname != animState.groupname || newState.startKey != animState.startKey
            || kft != keyframeTrack)
        {
            // Animation have changed, start blending!
            Log(Debug::Info) << "Animation change to: " << newState.groupname << " " << newState.startKey << " ";
            auto blendRule = FindBlendingRule(blendRules, animState, newState);
            if (blendRule)
            {
                blendDuration = blendRule->duration;
                easingFn = easingFnMap[blendRule->easing];
            }
            keyframeTrack = kft;
            animState = newState;
            interpFactor = 0.0f;
            lastTimeStamp = 0.0f;
        }
    }

    osg::Vec3f AnimationBlendingController::Vec3fLerp(float t, osg::Vec3f A, osg::Vec3f B)
    {
        return A + (B - A) * t;
    }

    void AnimationBlendingController::operator()(osg::Node* node, osg::NodeVisitor* nv)
    {
        // HOW ANIMATIONS WORK: The actual retrieval of the bone transformation based on animation is done by the
        // KeyframeController. KeyframeController retreives time data (playback position) every frame from a controller
        // source which is bound to an appropriate AnimationState in Animation.cpp. Animation.cpp ultimately manages
        // animation playback via updating AnimationState objects and determines when and what should be playing.
        auto [translation, rotation, scale] = keyframeTrack->GetCurrentTransformation(nv);

        // To do - don't cast every frame, cache.
        auto mtx = dynamic_cast<NifOsg::MatrixTransform*>(node);

        // Im not sure what to do here, do we need a fallback that will still somehow apply the animation if its not a
        // NifOsg::MatrixTransform?
        if (!mtx)
            return;

        // Log(Debug::Info) << "ABC Animating node: " << node->getName();
        // Log(Debug::Info) << "Last TS: " << lastTimeStamp;

        float time = nv->getFrameStamp()->getSimulationTime();
        if (lastTimeStamp == 0.0f)
        {
            blendStartTime = time;
            blendStartRot = mtx->getmRotation();
            blendStartTrans = mtx->getTranslation();
            lastTimeStamp = time;
        }

        interpFactor = std::min((time - blendStartTime) / blendDuration, 1.0f);
        interpFactor = easingFn(interpFactor);

        // Interpolate node's rotation
        if (rotation)
        {
            osg::Quat lerpedRot;
            lerpedRot.slerp(interpFactor, blendStartRot, *rotation);
            mtx->setRotation(lerpedRot);
            // mtx->setRotation(*rotation);
        }
        else
        {
            // This is necessary to prevent first person animation glitching out
            mtx->setRotation(mtx->mRotationScale);
        }

        // Update node's translation
        if (translation)
        {
            osg::Vec3f lerpedTrans = Vec3fLerp(interpFactor, blendStartTrans, *translation);
            mtx->setTranslation(lerpedTrans);
        }

        // Update node's scale
        if (scale)
            mtx->setScale(*scale);

        lastTimeStamp = time;

        traverse(node, nv);
    }

    std::optional<AnimBlendRule> AnimationBlendingController::FindBlendingRule(
        std::vector<AnimBlendRule>& rules, AnimStateData& fromState, AnimStateData& toState)
    {
        for (auto rule = rules.rbegin(); rule != rules.rend(); ++rule)
        {
            bool fromMatch = false;
            bool toMatch = false;

            if ((fromState.groupname == rule->fromGroup || rule->fromGroup == "*")
                && (fromState.startKey == rule->fromKey || rule->fromKey == "*" || rule->fromKey == ""))
            {
                fromMatch = true;
            }

            if ((toState.groupname == rule->toGroup || rule->toGroup == "*"
                    || (rule->toGroup == "$" && toState.groupname == fromState.groupname))
                && (toState.startKey == rule->toKey || rule->toKey == "*" || rule->toKey == ""))
            {
                toMatch = true;
            }

            if (fromMatch && toMatch)
                return std::make_optional<AnimBlendRule>(*rule);
        }

        return std::nullopt;
    }

    std::pair<std::string, std::string> AnimationBlendingController::AnimBlendRule::ParseFullName(std::string full)
    {
        std::string group = "";
        std::string key = "";
        size_t delimiterInd = full.find(":");
        if (delimiterInd == std::string::npos)
            group = full;
        else
        {
            group = full.substr(0, delimiterInd);
            key = full.substr(delimiterInd);
        }
        return std::make_pair(group, key);
    }

}
