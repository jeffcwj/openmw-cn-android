#ifndef SCENEUTIL_NODECALLBACK_H
#define SCENEUTIL_NODECALLBACK_H

#include <osg/Callback>

namespace osg
{
    class Node;
    class NodeVisitor;
}

namespace SceneUtil
{
    // IMPORTANT: While inheriting your controller from this class you have to be ABSOLUTELY SURE that when
    // you bind you controller to a node via node->addUpdateCallback() an actual type of node will match (or be a child
    // of) NodeType provided to this class. Otherwise an unsafe cast of node to incompatible NodeType done here will
    // result in everything imploding on itself in an unpredictable and confusing fashion.
    template <class Derived, typename NodeType = osg::Node*, typename VisitorType = osg::NodeVisitor*>
    class NodeCallback : public osg::Callback
    {
    public:
        NodeCallback() {}
        NodeCallback(const NodeCallback& nc, const osg::CopyOp& copyop)
            : osg::Callback(nc, copyop)
        {
        }

        bool run(osg::Object* object, osg::Object* data) override
        {
            static_cast<Derived*>(this)->operator()((NodeType)object, (VisitorType)data->asNodeVisitor());
            return true;
        }

        template <typename VT>
        void traverse(NodeType object, VT data)
        {
            if (_nestedCallback.valid())
                _nestedCallback->run(object, data);
            else
                data->traverse(*object);
        }
    };

}
#endif
