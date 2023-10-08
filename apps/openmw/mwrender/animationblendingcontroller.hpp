#ifndef OPENMW_MWRENDER_ANIMATIONBLENDINGCONTROLLER_H
#define OPENMW_MWRENDER_ANIMATIONBLENDINGCONTROLLER_H

#include <map>
#include <optional>
#include <unordered_map>

#include <components/debug/debuglog.hpp>
#include <components/nifosg/matrixtransform.hpp>
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

    class AnimationBlendingController
        : public SceneUtil::NodeCallback<AnimationBlendingController, NifOsg::MatrixTransform*>,
          public SceneUtil::Controller
    {
    public:
        struct AnimStateData
        {
            std::string groupname;
            std::string startKey;
            AnimStateData(std::string g, std::string k)
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

        struct AnimBlendRule
        {
            std::string fromGroup;
            std::string fromKey;
            std::string toGroup;
            std::string toKey;
            float duration;
            std::string easing;
        };

        std::optional<AnimBlendRule> FindBlendingRule(
            std::vector<AnimBlendRule> rules, AnimStateData fromState, AnimStateData toState);

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
        // float blendStartScale;

        AnimStateData animState;

        typedef float (*EasingFn)(float);

        std::unordered_map<std::string_view, EasingFn> easingFnMap = { { "linear", Easings::linear },
            { "sineOut", Easings::sineOut }, { "sineIn", Easings::sineIn }, { "sineInOut", Easings::sineInOut },
            { "cubicOut", Easings::cubicOut }, { "cubicIn", Easings::cubicIn }, { "cubicInOut", Easings::cubicInOut },
            { "quartOut", Easings::quartOut }, { "quartIn", Easings::quartIn }, { "quartInOut", Easings::quartInOut },
            { "springOutWeak", Easings::springOutWeak }, { "springOutMed", Easings::springOutMed },
            { "springOutStrong", Easings::springOutStrong }, { "springOutTooMuch", Easings::springOutTooMuch } };
    };

}

#endif
