#ifndef OPENMW_MWRENDER_ANIMBLENDCONTROLLER_H
#define OPENMW_MWRENDER_ANIMBLENDCONTROLLER_H

#include <map>
#include <optional>
#include <string>
#include <unordered_map>

#include <components/debug/debuglog.hpp>
#include <components/nifosg/matrixtransform.hpp>
#include <components/sceneutil/animblendrules.hpp>
#include <components/sceneutil/controller.hpp>
#include <components/sceneutil/keyframe.hpp>
#include <components/sceneutil/nodecallback.hpp>

namespace MWRender
{
    namespace Easings
    {
        typedef float (*EasingFn)(float);
    }

    class AnimBlendController : public SceneUtil::NodeCallback<AnimBlendController, NifOsg::MatrixTransform*>,
                                public SceneUtil::Controller
    {
    public:
        struct AnimStateData
        {
            std::string mGroupname;
            std::string mStartKey;
        };

        AnimBlendController(osg::ref_ptr<SceneUtil::KeyframeController> keyframeTrack, AnimStateData animState,
            osg::ref_ptr<const SceneUtil::AnimBlendRules> blendRules);

        void operator()(NifOsg::MatrixTransform* node, osg::NodeVisitor* nv);

        void setKeyframeTrack(osg::ref_ptr<SceneUtil::KeyframeController> kft, AnimStateData animState,
            osg::ref_ptr<const SceneUtil::AnimBlendRules> blendRules);
        osg::Callback* getAsCallback() { return this; }

    protected:
        osg::ref_ptr<SceneUtil::KeyframeController> mKeyframeTrack;

    private:
        Easings::EasingFn mEasingFn;
        float mBlendDuration;

        bool mBlendTrigger = false;
        float mBlendStartTime;
        osg::Quat mBlendStartRot;
        osg::Vec3f mBlendStartTrans;
        float mBlendStartScale;

        float mInterpFactor;

        AnimStateData mAnimState;
        osg::ref_ptr<const SceneUtil::AnimBlendRules> mAnimBlendRules;
    };

}

#endif
