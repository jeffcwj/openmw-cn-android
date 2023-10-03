#include "animationblendingcontroller.hpp"

namespace MWRender
{

    AnimationBlendingController::AnimationBlendingController(osg::ref_ptr<KeyframeController> keyframeTrack)
        : keyframeTrack(keyframeTrack) 
        , interpFactor(0.0f)
        , lastTimeStamp(0.0f)
    {
        
    }

    AnimationBlendingController::AnimationBlendingController(const AnimationBlendingController& copy, const osg::CopyOp& copyop)
        : keyframeTrack(nullptr)
        , interpFactor(0.0f)
        , lastTimeStamp(0.0f)
    {
    }

    

    void AnimationBlendingController::operator()(NifOsg::MatrixTransform* node, osg::NodeVisitor* nv)
    {
        auto [translation, rotation, scale] = keyframeTrack->GetCurrentTransformation(nv);

        float time = nv->getFrameStamp()->getSimulationTime();
        if (lastTimeStamp == 0.0f)
            lastTimeStamp = time;
        float dTime = time - lastTimeStamp;

        interpFactor = std::min(interpFactor + dTime, 1.0f);

        // Interpolate node's rotation
        if (rotation)
        {
            osg::Quat lerpedRot;
            //lerpedRot.slerp(interpFactor, node->getmRotation(), *rotation);
            //node->setRotation(lerpedRot);
            node->setRotation(*rotation);
        }
        else
        {
            // This is necessary to prevent first person animation glitching out
            node->setRotation(node->mRotationScale);
        }

        // Update node's translation
        if (translation)
            node->setTranslation(*translation);

        // Update node's scale
        if (scale)
            node->setScale(*scale);    

        lastTimeStamp = time;
    }

   

}
