#include "animationblendingcontroller.hpp"
#include <string>
#include <vector>

namespace MWRender
{
    using AnimStateData = AnimationBlendingController::AnimStateData;

    // AnimationBlendingController::AnimationBlendingController() {}

    AnimationBlendingController::AnimationBlendingController(osg::ref_ptr<KeyframeController> keyframeTrack,
        AnimStateData newState, std::shared_ptr<AnimBlendRules> blendRules)
    {
        setKeyframeTrack(keyframeTrack, newState, blendRules);
        // Log(Debug::Info) << "AnimationBlendingController created for: " << keyframeTrack-> getName();
    }

    /*AnimationBlendingController::AnimationBlendingController(
        const AnimationBlendingController& copy, const osg::CopyOp& copyop)
        : osg::Object(copy, copyop)
    {
    }*/

    void AnimationBlendingController::setKeyframeTrack(
        osg::ref_ptr<KeyframeController> kft, AnimStateData newState, std::shared_ptr<AnimBlendRules> blendRules)
    {

        if (newState.mGroupname == "INIT_STATE")
            return;
        if (newState.mGroupname != mAnimState.mGroupname || newState.mStartKey != mAnimState.mStartKey
            || kft != mKeyframeTrack)
        {
            // Animation have changed, start blending!
            // Log(Debug::Info) << "Animation change to: " << newState.mGroupname << " " << newState.mStartKey << " ";

            // Default blend settings
            // TO DO: Activate this only if a flag in Settings::game() is set to true
            mBlendDuration = 0.22;
            mEasingFn = &Easings::sineOut;

            if (blendRules)
            {
                auto blendRule = blendRules->findBlendingRule(
                    mAnimState.mGroupname, mAnimState.mStartKey, newState.mGroupname, newState.mStartKey);
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

    osg::Vec3f AnimationBlendingController::vec3fLerp(float t, osg::Vec3f A, osg::Vec3f B)
    {
        return A + (B - A) * t;
    }

    void AnimationBlendingController::operator()(NifOsg::MatrixTransform* mtx, osg::NodeVisitor* nv)
    {
        // HOW THIS WORK: The actual retrieval of the bone transformation based on animation is done by the
        // KeyframeController (mKeyframeTrack). The KeyframeController retreives time data (playback position) every
        // frame from controller's input (getInputValue(nv)) which is bound to an appropriate AnimationState time value
        // in Animation.cpp. Animation.cpp ultimately manages animation playback via updating AnimationState objects and
        // determines when and what should be playing.
        // This controller exploits KeyframeController to get transformations and upon animation change blends from
        // the last known position to the new animated one.
        auto [translation, rotation, scale] = mKeyframeTrack->GetCurrentTransformation(nv);

        // Log(Debug::Info) << "ABC Animating node: " << node->getName();
        // Log(Debug::Info) << "Last TS: " << lastTimeStamp;

        // Probably might make sence to adjust this by animation speed?
        float time = nv->getFrameStamp()->getSimulationTime();

        if (mBlendTrigger)
        {
            mBlendTrigger = false;
            mBlendStartTime = time;
            mBlendStartRot = mtx->getmRotation();
            mBlendStartTrans = mtx->getTranslation();
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
            osg::Vec3f lerpedTrans = vec3fLerp(mInterpFactor, mBlendStartTrans, *translation);
            mtx->setTranslation(lerpedTrans);
        }

        // Update node's scale
        if (scale)
            mtx->setScale(*scale);

        traverse(mtx, nv);
    }

}
