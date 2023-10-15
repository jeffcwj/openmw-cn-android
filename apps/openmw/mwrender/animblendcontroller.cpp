#include "animblendcontroller.hpp"
#include <string>
#include <vector>

namespace MWRender
{
    using AnimStateData = AnimBlendController::AnimStateData;

    AnimBlendController::AnimBlendController(osg::ref_ptr<KeyframeController> keyframeTrack, AnimStateData newState,
        osg::ref_ptr<const AnimBlendRules> blendRules)
    {
        setKeyframeTrack(keyframeTrack, newState, blendRules);
    }

    void AnimBlendController::setKeyframeTrack(
        osg::ref_ptr<KeyframeController> kft, AnimStateData newState, osg::ref_ptr<const AnimBlendRules> blendRules)
    {

        if (newState.mGroupname == "INIT_STATE")
            return;
        if (newState.mGroupname != mAnimState.mGroupname || newState.mStartKey != mAnimState.mStartKey
            || kft != mKeyframeTrack)
        {
            // Animation have changed, start blending!
            // Log(Debug::Info) << "Animation change to: " << newState.mGroupname << ":" << newState.mStartKey;

            // Default blend settings
            mBlendDuration = 0.22;
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
                    if (mEasingFnMap.contains(blendRule->mEasing))
                    {
                        mBlendDuration = blendRule->mDuration;
                        mEasingFn = mEasingFnMap[blendRule->mEasing];
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
            // Nif mNifRotation is used here because it's unaffected by the side-effects of RotationController
            mBlendStartRot = node->mNifRotation.toOsgMatrix().getRotate();
            mBlendStartTrans = node->getTranslation();
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
            node->setRotation(node->mNifRotation);
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
