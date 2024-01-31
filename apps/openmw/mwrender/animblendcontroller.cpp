#include "animblendcontroller.hpp"
#include <string>
#include <vector>

namespace MWRender
{
    using AnimStateData = AnimBlendController::AnimStateData;

    /// Animation Easing/Blending functions
    namespace Easings
    {
        float linear(float x)
        {
            return x;
        }
        float sineOut(float x)
        {
            return sin((x * 3.14) / 2);
        }
        float sineIn(float x)
        {
            return 1 - cos((x * 3.14) / 2);
        }
        float sineInOut(float x)
        {
            return -(cos(3.14 * x) - 1) / 2;
        }
        float cubicOut(float t)
        {
            return 1 - powf(1 - t, 3);
        }
        float cubicIn(float x)
        {
            return powf(x, 3);
        }
        float cubicInOut(float x)
        {
            return x < 0.5 ? 4 * x * x * x : 1 - powf(-2 * x + 2, 3) / 2;
        }
        float quartOut(float t)
        {
            return 1 - powf(1 - t, 4);
        }
        float quartIn(float t)
        {
            return powf(t, 4);
        }
        float quartInOut(float x)
        {
            return x < 0.5 ? 8 * x * x * x * x : 1 - powf(-2 * x + 2, 4) / 2;
        }
        float springOutGeneric(float x, float lambda, float w)
        {
            // Higher lambda = lower swing amplitude. 1 = 150% swing amplitude.
            // W corresponds to the amount of overswings, more = more. 4.71 = 1 overswing, 7.82 = 2
            return 1 - expf(-lambda * x) * cos(w * x);
        }
        float springOutWeak(float x)
        {
            return springOutGeneric(x, 4, 4.71);
        }
        float springOutMed(float x)
        {
            return springOutGeneric(x, 3, 4.71);
        }
        float springOutStrong(float x)
        {
            return springOutGeneric(x, 2, 4.71);
        }
        float springOutTooMuch(float x)
        {
            return springOutGeneric(x, 1, 4.71);
        }
        std::unordered_map<std::string, EasingFn> easingsMap = { { "linear", Easings::linear },
            { "sineOut", Easings::sineOut }, { "sineIn", Easings::sineIn }, { "sineInOut", Easings::sineInOut },
            { "cubicOut", Easings::cubicOut }, { "cubicIn", Easings::cubicIn }, { "cubicInOut", Easings::cubicInOut },
            { "quartOut", Easings::quartOut }, { "quartIn", Easings::quartIn }, { "quartInOut", Easings::quartInOut },
            { "springOutWeak", Easings::springOutWeak }, { "springOutMed", Easings::springOutMed },
            { "springOutStrong", Easings::springOutStrong }, { "springOutTooMuch", Easings::springOutTooMuch } };
    }

    namespace
    {
        // Helper methods
        osg::Vec3f vec3fLerp(float t, osg::Vec3f A, osg::Vec3f B)
        {
            return A + (B - A) * t;
        }
    }

    AnimBlendController::AnimBlendController(osg::ref_ptr<SceneUtil::KeyframeController> keyframeTrack,
        AnimStateData newState, osg::ref_ptr<const SceneUtil::AnimBlendRules> blendRules)
    {
        setKeyframeTrack(keyframeTrack, newState, blendRules);
    }

    void AnimBlendController::setKeyframeTrack(osg::ref_ptr<SceneUtil::KeyframeController> kft, AnimStateData newState,
        osg::ref_ptr<const SceneUtil::AnimBlendRules> blendRules)
    {
        if (newState.mGroupname != mAnimState.mGroupname || newState.mStartKey != mAnimState.mStartKey
            || kft != mKeyframeTrack)
        {
            // Animation have changed, start blending!
            // Log(Debug::Info) << "Animation change to: " << newState.mGroupname << ":" << newState.mStartKey;

            // Default blend settings
            mBlendDuration = 0;
            mEasingFn = &Easings::sineOut;

            if (blendRules)
            {
                // Finds a matching blend rule either in this or previous ruleset
                auto blendRule = blendRules->findBlendingRule(
                    mAnimState.mGroupname, mAnimState.mStartKey, newState.mGroupname, newState.mStartKey);
                // This will also check the previous ruleset, not sure it's a good idea though, commenting out
                // for now.
                /*if (!blendRule && mAnimBlendRules)
                    blendRule = mAnimBlendRules->findBlendingRule(
                        mAnimState.mGroupname, mAnimState.mStartKey, newState.mGroupname, newState.mStartKey);*/
                if (blendRule)
                {
                    if (Easings::easingsMap.contains(blendRule->mEasing))
                    {
                        mBlendDuration = blendRule->mDuration;
                        mEasingFn = Easings::easingsMap[blendRule->mEasing];
                    }
                    else
                    {
                        Log(Debug::Warning)
                            << "Warning: animation blending rule contains invalid easing type: " << blendRule->mEasing;
                    }
                }
            }

            mAnimBlendRules = blendRules;
            mKeyframeTrack = kft;
            mAnimState = newState;
            mBlendTrigger = true;
        }
    }

    void AnimBlendController::operator()(NifOsg::MatrixTransform* node, osg::NodeVisitor* nv)
    {
        // HOW THIS WORK: The actual retrieval of the bone transformation based on animation is done by the
        // KeyframeController (mKeyframeTrack). The KeyframeController retreives time data (playback position) every
        // frame from controller's input (getInputValue(nv)) which is bound to an appropriate AnimationState time value
        // in Animation.cpp. Animation.cpp ultimately manages animation playback via updating AnimationState objects and
        // determines when and what should be playing.
        // This controller exploits KeyframeController to get transformations and upon animation change blends from
        // the last known position to the new animated one.

        auto [translation, rotation, scale] = mKeyframeTrack->getCurrentTransformation(nv);

        // Probably might make sense to adjust this by animation speed?
        float time = nv->getFrameStamp()->getSimulationTime();

        if (mBlendTrigger)
        {
            mBlendTrigger = false;
            mBlendStartTime = time;
            // Nif mRotation is used here because it's unaffected by the side-effects of RotationController
            mBlendStartRot = node->mRotation.toOsgMatrix().getRotate();
            mBlendStartTrans = node->getMatrix().getTrans();
            mBlendStartScale = node->mScale;
        }

        if (mBlendDuration != 0)
            mInterpFactor = std::min((time - mBlendStartTime) / mBlendDuration, 1.0f);
        else
            mInterpFactor = 1;

        mInterpFactor = mEasingFn(mInterpFactor);

        // Interpolate node's rotation
        if (rotation)
        {
            osg::Quat lerpedRot;
            lerpedRot.slerp(mInterpFactor, mBlendStartRot, *rotation);
            node->setRotation(lerpedRot);
        }
        else
        {
            // This is necessary to prevent first person animation glitching out
            node->setRotation(node->mRotation);
        }

        // Update node's translation
        if (translation)
        {
            osg::Vec3f lerpedTrans = vec3fLerp(mInterpFactor, mBlendStartTrans, *translation);
            node->setTranslation(lerpedTrans);
        }

        // Update node's scale
        if (scale)
            // Scale is not lerped based on the idea that it is much more likely that scale animation will be used to
            // instantly hide/show objects in which case the scale interpolation is undesirable.
            node->setScale(*scale);

        traverse(node, nv);
    }

}
