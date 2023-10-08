#include "matrixtransform.hpp"

namespace NifOsg
{
    MatrixTransform::MatrixTransform(const Nif::NiTransform& transform)
        : osg::MatrixTransform(transform.toMatrix())
        , mScale(transform.mScale)
        , mRotationScale(transform.mRotation)
    {
    }

    MatrixTransform::MatrixTransform(const MatrixTransform& copy, const osg::CopyOp& copyop)
        : osg::MatrixTransform(copy, copyop)
        , mScale(copy.mScale)
        , mRotationScale(copy.mRotationScale)
    {
    }

    osg::Matrix MatrixTransform::NifToOsgMtx(Nif::Matrix3 nifMtx)
    {
        // Identity matrix
        osg::Matrix osgMtx(
            1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);

        for (int i = 0; i < 3; ++i)
            for (int j = 0; j < 3; ++j)
                osgMtx(i, j) = nifMtx.mValues[j][i]; // NB: column/row major difference

        return osgMtx;
    }

    void MatrixTransform::setScale(float scale)
    {
        // Update the decomposed scale.
        mScale = scale;

        // Rescale the node using the known components.
        for (int i = 0; i < 3; ++i)
            for (int j = 0; j < 3; ++j)
                _matrix(i, j) = mRotationScale.mValues[j][i] * mScale; // NB: column/row major difference

        _inverseDirty = true;
        dirtyBound();
    }

    void MatrixTransform::setRotation(const osg::Quat& rotation)
    {
        // First override the rotation ignoring the scale.
        _matrix.setRotate(rotation);

        for (int i = 0; i < 3; ++i)
        {
            for (int j = 0; j < 3; ++j)
            {
                // Update the current decomposed rotation and restore the known scale.
                mRotationScale.mValues[j][i] = _matrix(i, j); // NB: column/row major difference
                _matrix(i, j) *= mScale;
            }
        }

        _inverseDirty = true;
        dirtyBound();
    }

    void MatrixTransform::setRotation(const Nif::Matrix3& rotation)
    {
        // Update the decomposed rotation.
        mRotationScale = rotation;

        // Reorient the node using the known components.
        for (int i = 0; i < 3; ++i)
            for (int j = 0; j < 3; ++j)
                _matrix(i, j) = mRotationScale.mValues[j][i] * mScale; // NB: column/row major difference

        _inverseDirty = true;
        dirtyBound();
    }

    osg::Quat MatrixTransform::getmRotation()
    {

        osg::Matrix osgRotMtx = NifToOsgMtx(mRotationScale);
        return osgRotMtx.getRotate();
    }

    void MatrixTransform::setTranslation(const osg::Vec3f& translation)
    {
        // The translation is independent from the rotation and scale so we can apply it directly.
        _matrix.setTrans(translation);

        _inverseDirty = true;
        dirtyBound();
    }

    osg::Vec3f MatrixTransform::getTranslation()
    {
        return _matrix.getTrans();
    }
}
