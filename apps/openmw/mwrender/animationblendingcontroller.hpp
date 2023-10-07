#ifndef OPENMW_MWRENDER_ANIMATIONBLENDINGCONTROLLER_H
#define OPENMW_MWRENDER_ANIMATIONBLENDINGCONTROLLER_H

#include <components/sceneutil/nodecallback.hpp>
#include <components/sceneutil/controller.hpp>
#include <components/sceneutil/keyframe.hpp>
#include <components/nifosg/matrixtransform.hpp>
#include <components/debug/debuglog.hpp>

using namespace SceneUtil;


namespace MWRender
{

    
    class AnimationBlendingController
        : public SceneUtil::NodeCallback<AnimationBlendingController, NifOsg::MatrixTransform*>,
          public SceneUtil::Controller
    {
    public:

        struct AnimStateData
        {
            std::string_view groupname;
            std::string_view startKey;
            AnimStateData(std::string_view g, std::string_view k)
                : groupname(g)
                , startKey(k)
            {
            }

            AnimStateData() 
                : groupname("INIT_STATE")
                , startKey("")
            {
            }

        };
       
        AnimationBlendingController(osg::ref_ptr<KeyframeController> keyframeTrack, AnimStateData animState);

        void operator()(NifOsg::MatrixTransform* node, osg::NodeVisitor* nv);
        
        void SetKeyframeTrack(osg::ref_ptr<KeyframeController> kft, AnimStateData animState);
        osg::Vec3f Vec3fLerp(float t, osg::Vec3f A, osg::Vec3f B);
        osg::Callback* getAsCallback() { return this; } 

    protected:
        osg::ref_ptr<KeyframeController> keyframeTrack;

    private:
        float lastTimeStamp;
        float interpFactor;
        
        float blendStartTime;
        osg::Quat blendStartRot;
        osg::Vec3f blendStartTrans;
        float blendStartScale;

        AnimStateData animState;

        /// Easings
        float linear(float x) { return x; }
        float sineOut(float x) { return sin((x * 3.14) / 2); }
        float sineIn(float x) { return 1 - cos((x * 3.14) / 2); }
        float sineInOut(float x) { return -(cos(3.14 * x) - 1) / 2; }
        float cubicOut(float t) { return 1 - powf(1 - t, 3); }
        float cubicIn(float x) { return powf(x, 3); }
        float cubicInOut(float x) { return x < 0.5 ? 4 * x * x * x : 1 - powf(-2 * x + 2, 3) / 2; }
        float quartOut(float t) { return 1 - powf(1 - t, 4); }
        float quartIn(float t) { return powf(t, 4); }
        float quartInOut(float x) { return x < 0.5 ? 8 * x * x * x * x : 1 - powf(-2 * x + 2, 4) / 2; }
        float springOutGeneric(float x, float lambda, float w) {
            // Higher lambda = lower swing amplitude. 1 = 150% swing amplitude.
            // W corresponds to the amount of overswings, more = more. 4.71 = 1 overswing, 7.82 = 2
            return 1 - expf(-lambda * x) * cos(w * x);
        }
        float springOutWeak(float x) { return springOutGeneric(x, 4, 4.71);}
        float springOutMed(float x) { return springOutGeneric(x, 3, 4.71); }
        float springOutStrong(float x) { return springOutGeneric(x, 2, 4.71); }
        float springOutTooMuch(float x) { return springOutGeneric(x, 1, 4.71); }
        
    };

}

#endif
