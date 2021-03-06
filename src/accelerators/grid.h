
/*
    pbrt source code Copyright(c) 1998-2012 Matt Pharr and Greg Humphreys.

    This file is part of pbrt.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are
    met:

    - Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.

    - Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
    IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
    TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
    PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
    HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
    SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
    LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
    DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
    THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
    OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 */

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef PBRT_ACCELERATORS_GRID_H
#define PBRT_ACCELERATORS_GRID_H

// accelerators/grid.h*
#include "pbrt.h"
#include "primitive.h"

// GridAccel Forward Declarations
struct Voxel;

// Voxel Declarations
struct Voxel {
    // Voxel Public Methods
    uint32_t size() const { return primitives.size(); }
    Voxel() { }
    Voxel(Reference<Primitive> op) {
        allCanIntersect = false;
        primitives.push_back(op);
    }
    void AddPrimitive(Reference<Primitive> prim) {
        primitives.push_back(prim);
    }
    bool Intersect(const Ray &ray, Intersection *isect, RWMutexLock &lock);
    bool IntersectP(const Ray &ray, RWMutexLock &lock);
private:
    vector<Reference<Primitive> > primitives;
    bool allCanIntersect;
};


/*
(GridAccel) based on overlaying a uniform grid over the scene,

GridAccel is an accelerator that divides an axis-aligned region of space into equal-sized
box-shaped chunks (called voxels). Each voxel stores references to the primitives that
overlap it. Given a ray, the grid steps through each of the voxels that the
ray passes through in order, checking for intersections with only the primitives in each
voxel. Useless ray intersection tests are reduced substantially because primitives far away
from the ray aren��t considered at all. Furthermore, because the voxels are considered
from near to far along the ray, it is possible to stop performing intersection tests once
an intersection has been found and it is certain that it is not possible for there to be any
closer intersections.

// ����: 
GridAccel can suffer from poor performance when the primitives
in the scene aren��t distributed evenly throughout space. If there��s a small region of space
with a lot of geometry in it, all that geometry might fall in a single voxel, and performance
will suffer when a ray passes through that voxel, as many intersection tests will be
performed.

*/
// GridAccel Declarations
class GridAccel : public Aggregate {
public:
    // GridAccel Public Methods
    GridAccel(const vector<Reference<Primitive> > &p, bool refineImmediately);
    BBox WorldBound() const;
    bool CanIntersect() const { return true; }
    ~GridAccel();
    bool Intersect(const Ray &ray, Intersection *isect) const;
    bool IntersectP(const Ray &ray) const;
private:
    // GridAccel Private Methods
    int posToVoxel(const Point &P, int axis) const {
        int v = Float2Int((P[axis] - bounds.pMin[axis]) *
                          invWidth[axis]);
        return Clamp(v, 0, nVoxels[axis]-1);
    }
    float voxelToPos(int p, int axis) const {
        return bounds.pMin[axis] + p * width[axis];
    }
    inline int offset(int x, int y, int z) const {
        return z*nVoxels[0]*nVoxels[1] + y*nVoxels[0] + x;
    }

    // GridAccel Private Data
    vector<Reference<Primitive> > primitives;
    int nVoxels[3];
    BBox bounds;
    Vector width, invWidth;
    Voxel **voxels;
    MemoryArena voxelArena;
    mutable RWMutex *rwMutex;
};


GridAccel *CreateGridAccelerator(const vector<Reference<Primitive> > &prims,
        const ParamSet &ps);

#endif // PBRT_ACCELERATORS_GRID_H
