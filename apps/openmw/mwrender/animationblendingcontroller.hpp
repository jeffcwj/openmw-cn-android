#ifndef OPENMW_MWRENDER_ANIMATIONBLENDINGCONTROLLER_H
#define OPENMW_MWRENDER_ANIMATIONBLENDINGCONTROLLER_H

#include <components/sceneutil/nodecallback.hpp>
#include <components/sceneutil/controller.hpp>
#include <components/sceneutil/keyframe.hpp>
#include <components/nifosg/matrixtransform.hpp>


using namespace SceneUtil;

namespace MWRender
{

    
    class AnimationBlendingController
        : public SceneUtil::NodeCallback<AnimationBlendingController, NifOsg::MatrixTransform*>,
          public SceneUtil::Controller
    {
    public:
        AnimationBlendingController() = default;
        AnimationBlendingController(osg::ref_ptr<KeyframeController> keyframeTrack);
        AnimationBlendingController(const AnimationBlendingController& copy, const osg::CopyOp& copyop);

        META_Object(NifOsg, AnimationBlendingController);
        
        void SetKeyframeTrack(KeyframeController* kft) { keyframeTrack = kft; };
        void operator()(NifOsg::MatrixTransform* node, osg::NodeVisitor* nv);
        osg::Callback* getAsCallback() { return this; } 

    protected:
        osg::ref_ptr<KeyframeController> keyframeTrack;

    private:
        float lastTimeStamp;
        float interpFactor;
       
    };

}

#endif
