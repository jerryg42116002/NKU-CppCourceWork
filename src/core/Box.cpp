#include "core/Box.h"

#include <algorithm>
#include <cmath>

namespace tinyray {

namespace {

Vec3 normalForBoxHit(const Vec3& point, const Vec3& minimum, const Vec3& maximum)
{
    const double distances[6] = {
        std::abs(point.x - minimum.x),
        std::abs(point.x - maximum.x),
        std::abs(point.y - minimum.y),
        std::abs(point.y - maximum.y),
        std::abs(point.z - minimum.z),
        std::abs(point.z - maximum.z)
    };

    int axis = 0;
    for (int index = 1; index < 6; ++index) {
        if (distances[index] < distances[axis]) {
            axis = index;
        }
    }

    switch (axis) {
    case 0:
        return Vec3(-1.0, 0.0, 0.0);
    case 1:
        return Vec3(1.0, 0.0, 0.0);
    case 2:
        return Vec3(0.0, -1.0, 0.0);
    case 3:
        return Vec3(0.0, 1.0, 0.0);
    case 4:
        return Vec3(0.0, 0.0, -1.0);
    default:
        return Vec3(0.0, 0.0, 1.0);
    }
}

} // namespace

Vec3 Box::minCorner() const
{
    return center - Vec3(std::abs(size.x), std::abs(size.y), std::abs(size.z)) * 0.5;
}

Vec3 Box::maxCorner() const
{
    return center + Vec3(std::abs(size.x), std::abs(size.y), std::abs(size.z)) * 0.5;
}

bool Box::intersect(const Ray& ray, double tMin, double tMax, HitRecord& record) const
{
    const Vec3 minimum = minCorner();
    const Vec3 maximum = maxCorner();
    double tEnter = tMin;
    double tExit = tMax;

    const double origins[3] = {ray.origin.x, ray.origin.y, ray.origin.z};
    const double directions[3] = {ray.direction.x, ray.direction.y, ray.direction.z};
    const double mins[3] = {minimum.x, minimum.y, minimum.z};
    const double maxs[3] = {maximum.x, maximum.y, maximum.z};

    for (int axis = 0; axis < 3; ++axis) {
        if (std::abs(directions[axis]) < 1.0e-10) {
            if (origins[axis] < mins[axis] || origins[axis] > maxs[axis]) {
                return false;
            }
            continue;
        }

        const double inverseDirection = 1.0 / directions[axis];
        double t0 = (mins[axis] - origins[axis]) * inverseDirection;
        double t1 = (maxs[axis] - origins[axis]) * inverseDirection;
        if (t0 > t1) {
            std::swap(t0, t1);
        }

        tEnter = std::max(tEnter, t0);
        tExit = std::min(tExit, t1);
        if (tEnter > tExit) {
            return false;
        }
    }

    const double t = tEnter;
    if (t < tMin || t > tMax) {
        return false;
    }

    record.t = t;
    record.point = ray.at(t);
    record.material = &material;
    record.hitObjectId = id();
    record.setFaceNormal(ray, normalForBoxHit(record.point, minimum, maximum));
    return true;
}

std::shared_ptr<Object> Box::clone() const
{
    return std::make_shared<Box>(*this);
}

} // namespace tinyray
