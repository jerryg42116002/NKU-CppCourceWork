#pragma once

#include <atomic>
#include <memory>

#include "core/HitRecord.h"
#include "core/Ray.h"

namespace tinyray {

class Object
{
public:
    Object() = default;
    Object(const Object& other)
        : id_(other.id_)
    {
    }

    Object& operator=(const Object& other)
    {
        if (this != &other) {
            id_ = other.id_;
        }
        return *this;
    }

    virtual ~Object() = default;

    int id() const
    {
        return id_;
    }

    void setId(int id)
    {
        id_ = id > 0 ? id : createId();
        int expected = nextId_.load();
        while (expected <= id_ && !nextId_.compare_exchange_weak(expected, id_ + 1)) {
        }
    }

    static int createId()
    {
        return nextId_.fetch_add(1);
    }

    virtual bool intersect(const Ray& ray, double tMin, double tMax, HitRecord& record) const = 0;
    virtual std::shared_ptr<Object> clone() const = 0;

private:
    inline static std::atomic_int nextId_ = 1;
    int id_ = createId();
};

} // namespace tinyray
