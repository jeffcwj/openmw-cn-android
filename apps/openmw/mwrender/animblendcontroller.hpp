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

using namespace SceneUtil;

namespace MWRender
{
    /// Animation Easing/Blending functions
    namespace Easings
    {
        inline float linear(float x)
        {
            return x;
        }
        inline float sineOut(float x)
        {
            return sin((x * 3.14) / 2);
        }
        inline float sineIn(float x)
        {
            return 1 - cos((x * 3.14) / 2);
        }
        inline float sineInOut(float x)
        {
            return -(cos(3.14 * x) - 1) / 2;
        }
        inline float cubicOut(float t)
        {
            return 1 - powf(1 - t, 3);
        }
        inline float cubicIn(float x)
        {
            return powf(x, 3);
        }
        inline float cubicInOut(float x)
        {
            return x < 0.5 ? 4 * x * x * x : 1 - powf(-2 * x + 2, 3) / 2;
        }
        inline float quartOut(float t)
        {
            return 1 - powf(1 - t, 4);
        }
        inline float quartIn(float t)
        {
            return powf(t, 4);
        }
        inline float quartInOut(float x)
        {
            return x < 0.5 ? 8 * x * x * x * x : 1 - powf(-2 * x + 2, 4) / 2;
        }
        inline float springOutGeneric(float x, float lambda, float w)
        {
            // Higher lambda = lower swing amplitude. 1 = 150% swing amplitude.
            // W corresponds to the amount of overswings, more = more. 4.71 = 1 overswing, 7.82 = 2
            return 1 - expf(-lambda * x) * cos(w * x);
        }
        inline float springOutWeak(float x)
        {
            return springOutGeneric(x, 4, 4.71);
        }
        inline float springOutMed(float x)
        {
            return springOutGeneric(x, 3, 4.71);
        }
        inline float springOutStrong(float x)
        {
            return springOutGeneric(x, 2, 4.71);
        }
        inline float springOutTooMuch(float x)
        {
            return springOutGeneric(x, 1, 4.71);
        }
    }

    namespace
    {
        // Helper methods
        osg::Vec3f vec3fLerp(float t, osg::Vec3f A, osg::Vec3f B)
        {
            return A + (B - A) * t;
        }
    }

    class AnimBlendController : public SceneUtil::NodeCallback<AnimBlendController, NifOsg::MatrixTransform*>,
                                public SceneUtil::Controller
    {
    public:
        typedef float (*EasingFn)(float);

        struct AnimStateData
        {
            std::string mGroupname = "INIT_STATE";
            std::string mStartKey;
        };

        AnimBlendController(osg::ref_ptr<KeyframeController> keyframeTrack, AnimStateData animState,
            osg::ref_ptr<const AnimBlendRules> blendRules);

        void operator()(NifOsg::MatrixTransform* node, osg::NodeVisitor* nv);

        void setKeyframeTrack(osg::ref_ptr<KeyframeController> kft, AnimStateData animState,
            osg::ref_ptr<const AnimBlendRules> blendRules);
        osg::Callback* getAsCallback() { return this; }

    protected:
        osg::ref_ptr<KeyframeController> mKeyframeTrack;

    private:
        EasingFn mEasingFn;
        float mBlendDuration;

        bool mBlendTrigger = false;
        float mBlendStartTime;
        osg::Quat mBlendStartRot;
        osg::Vec3f mBlendStartTrans;
        float mBlendStartScale;

        float mInterpFactor;

        AnimStateData mAnimState;
        osg::ref_ptr<const AnimBlendRules> mAnimBlendRules;

        std::unordered_map<std::string, EasingFn> mEasingFnMap = { { "linear", Easings::linear },
            { "sineOut", Easings::sineOut }, { "sineIn", Easings::sineIn }, { "sineInOut", Easings::sineInOut },
            { "cubicOut", Easings::cubicOut }, { "cubicIn", Easings::cubicIn }, { "cubicInOut", Easings::cubicInOut },
            { "quartOut", Easings::quartOut }, { "quartIn", Easings::quartIn }, { "quartInOut", Easings::quartInOut },
            { "springOutWeak", Easings::springOutWeak }, { "springOutMed", Easings::springOutMed },
            { "springOutStrong", Easings::springOutStrong }, { "springOutTooMuch", Easings::springOutTooMuch } };
    };

}

#endif
