
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

#ifndef PBRT_CORE_REFLECTION_H
#define PBRT_CORE_REFLECTION_H

// core/reflection.h*
#include "pbrt.h"
#include "geometry.h"
#include "shape.h"
#include "rng.h"
#include "spectrum.h"
#include "kdtree.h"

// Reflection Declarations
Spectrum FrDiel(float cosi, float cost, const Spectrum &etai,
                const Spectrum &etat);
Spectrum FrCond(float cosi, const Spectrum &n, const Spectrum &k);

/*
P464
We’d therefore like to use a mapping function for isotropic BRDFs that takes a 4D pair of
directions ωi and ωo and converts them into a point in a three-dimensional space such
that the distance between two points is meaningful, the isotropy of the BRDF is reflected,
and reciprocity is represented.
*/
Point BRDFRemap(const Vector &wo, const Vector &wi);
struct IrregIsotropicBRDFSample {
    IrregIsotropicBRDFSample(const Point &pp, const Spectrum &vv)
        : p(pp), v(vv) { }
    IrregIsotropicBRDFSample() { }
    Point p;
    Spectrum v;
};


inline float Fdr(float eta) {
    if (eta >= 1)
        return -1.4399f / (eta*eta) + 0.7099f / eta + 0.6681f +
            0.0636f * eta;
    else
        return -0.4399f + .7099f / eta - .3319f / (eta * eta) +
            .0636f / (eta*eta*eta);
}



// BSDF Inline Functions
inline float CosTheta(const Vector &w) { return w.z; }
inline float AbsCosTheta(const Vector &w) { return fabsf(w.z); }
inline float SinTheta2(const Vector &w) {
    return max(0.f, 1.f - CosTheta(w)*CosTheta(w));
}


inline float SinTheta(const Vector &w) {
    return sqrtf(SinTheta2(w));
}


inline float CosPhi(const Vector &w) {
    float sintheta = SinTheta(w);
    if (sintheta == 0.f) return 1.f;
    return Clamp(w.x / sintheta, -1.f, 1.f);
}


inline float SinPhi(const Vector &w) {
    float sintheta = SinTheta(w);
    if (sintheta == 0.f) return 0.f;
    return Clamp(w.y / sintheta, -1.f, 1.f);
}


inline bool SameHemisphere(const Vector &w, const Vector &wp) {
    return w.z * wp.z > 0.f;
}



// BSDF Declarations
enum BxDFType {
    BSDF_REFLECTION   = 1<<0,
    BSDF_TRANSMISSION = 1<<1,
    BSDF_DIFFUSE      = 1<<2,
    BSDF_GLOSSY       = 1<<3,
    BSDF_SPECULAR     = 1<<4,
    BSDF_ALL_TYPES        = BSDF_DIFFUSE |
                            BSDF_GLOSSY |
                            BSDF_SPECULAR,
    BSDF_ALL_REFLECTION   = BSDF_REFLECTION |
                            BSDF_ALL_TYPES,
    BSDF_ALL_TRANSMISSION = BSDF_TRANSMISSION |
                            BSDF_ALL_TYPES,
    BSDF_ALL              = BSDF_ALL_REFLECTION |
                            BSDF_ALL_TRANSMISSION
};


struct BSDFSample {
   // BSDFSample Public Methods
   BSDFSample(float up0, float up1, float ucomp) {
       Assert(up0 >= 0.f && up0 < 1.f);
       Assert(up1 >= 0.f && up1 < 1.f);
       Assert(ucomp >= 0.f && ucomp < 1.f);
       uDir[0] = up0;
       uDir[1] = up1;
       uComponent = ucomp;
   }
   BSDFSample(RNG &rng) {
      uDir[0] = rng.RandomFloat();
      uDir[1] = rng.RandomFloat();
      uComponent = rng.RandomFloat();
   }
   BSDFSample(const Sample *sample, const BSDFSampleOffsets &offsets, uint32_t num);
   BSDFSample() { }
   float uDir[2], uComponent;
};


struct BSDFSampleOffsets {
    BSDFSampleOffsets() { }
    BSDFSampleOffsets(int count, Sample *sample);
    int nSamples, componentOffset, dirOffset;
};

/*
Recall(回想) that bidirectional scattering distribution functions(双向散射分布函数) 
describe how a surface reflects light arriving at its boundary;
they essentially(本质上) encapsulate(封装了) the difference between the
appearance of a mirror versus(与) colored paint, and so forth.
They are represented in pbrt by the BSDF class. pbrt provides BSDF implementations for several standard scattering
functions used in computer graphics, such as Lambertian reflection and the Torrance–
Sparrow microfacet model.

The BSDF interface makes it possible to compute reflected light at a single surface point,
but BSDFs may vary across(有所不同) a surface. Surfaces with complex material properties, such
as wood or marble, have a different BSDF at each point. Even if wood is modeled as
being perfectly diffuse, the color at each point will depend on the wood’s grain(纹理). These
spatial variations(空间分布) of shading parameters are described with Textures, which in turn may
be described procedurally or stored in image maps
*/
class BSDF {
public:
    // BSDF Public Methods
    Spectrum Sample_f(const Vector &wo, Vector *wi, const BSDFSample &bsdfSample,
                      float *pdf, BxDFType flags = BSDF_ALL,
                      BxDFType *sampledType = NULL) const;
    float Pdf(const Vector &wo, const Vector &wi,
              BxDFType flags = BSDF_ALL) const;
    BSDF(const DifferentialGeometry &dgs, const Normal &ngeom,
         float eta = 1.f);
    inline void Add(BxDF *bxdf);
    int NumComponents() const { return nBxDFs; }
    int NumComponents(BxDFType flags) const;
    Vector WorldToLocal(const Vector &v) const {
        return Vector(Dot(v, sn), Dot(v, tn), Dot(v, nn));
    }
    Vector LocalToWorld(const Vector &v) const {
        return Vector(sn.x * v.x + tn.x * v.y + nn.x * v.z,
                      sn.y * v.x + tn.y * v.y + nn.y * v.z,
                      sn.z * v.x + tn.z * v.y + nn.z * v.z);
    }
    Spectrum f(const Vector &woW, const Vector &wiW, BxDFType flags = BSDF_ALL) const;
    Spectrum rho(RNG &rng, BxDFType flags = BSDF_ALL,
                 int sqrtSamples = 6) const;
    Spectrum rho(const Vector &wo, RNG &rng, BxDFType flags = BSDF_ALL,
                 int sqrtSamples = 6) const;

    // BSDF Public Data
    const DifferentialGeometry dgShading;
    const float eta;
private:
    // BSDF Private Methods
    ~BSDF() { }

    // BSDF Private Data
    Normal nn, ng;
    Vector sn, tn;
    int nBxDFs;
#define MAX_BxDFS 8
    BxDF *bxdfs[MAX_BxDFS];
    friend class MixMaterial;
};


#define BSDF_ALLOC(arena, Type) new (arena.Alloc(sizeof(Type))) Type

/*

a set of classes for describing the way that light scatters at surfaces.
Recall that in Section 5.6.1 we introduced the bidirectional reflectance distribution
function (BRDF) abstraction to describe light reflection at a surface, the BTDF to describe
transmission at a surface, and the BSDF to encompass both of these effects. In this
chapter, we will start by defining a generic interface to these surface reflection and transmission
functions. Scattering from realistic surfaces is often best described as a mixture
of multiple BRDFs and BTDFs; in Chapter 9, we will introduce a BSDF object that combines
multiple BRDFs and BTDFs to represent overall scattering from the surface.


We will first define the interface for the individual BRDF and BTDF functions. BRDFs
and BTDFs share a common base class, BxDF. Because both have the exact same interface,
sharing the same base class reduces repeated code and allows some parts of the system to
work with BxDFs generically without distinguishing between BRDFs and BTDFs
*/
// BxDF Declarations
class BxDF {
public:
    // BxDF Interface
    virtual ~BxDF() { }
    BxDF(BxDFType t) : type(t) { }

	/*
	The MatchesFlags() utility method determines if the BxDF matches the user-supplied
	flags:
	*/
    bool MatchesFlags(BxDFType flags) const {
        return (type & flags) == type;
    }

	/*
	The key method that BxDFs provide is the BxDF::f() method. It returns the value of the
	distribution function for the given pair of directions. This interface implicitly assumes
	that light in different wavelengths is decoupled(解耦)—energy at one wavelength will not be
	reflected at a different wavelength. By making this assumption, the effect of the reflection
	function can be represented directly with a Spectrum. To support fluorescent materials
	where this assumption is not true would require that this method return an n × n matrix
	that encoded(编码) the transfer(转移) of energy between spectral samples (where n is the number of
	samples in the Spectrum representation).
	*/
    virtual Spectrum f(const Vector &wo, const Vector &wi) const = 0;

	/*
	Not all BxDFs can be evaluated(评估) with the f() method. For example, perfectly specular
	objects like a mirror, glass, or water only scatter light from a single incident direction
	into a single outgoing direction. Such BxDFs are best described with delta distributions
	that are zero except for the single direction where light is scattered.

	These BxDFs need special handling in pbrt, so we will also provide the method BxDF::
	Sample_f(). This method is used both for handling scattering that is described by delta
	distributions as well as for randomly sampling directions from BxDFs that scatter light
	along multiple directions; this second application will be explained in the discussion
	of Monte Carlo sampling in Chapter 14. BxDF::Sample_f() computes the direction of
	incident light ωi given an outgoing direction ωo and returns the value of the BxDF for the
	given pair of directions. For delta distributions, it is necessary for the BxDF to choose
	the incident light direction in this way, since the caller has no chance of generating
	the appropriate ωi direction.1 The u1, u2, and pdf parameters aren’t needed for delta
	distribution BxDFs, so they will be explained later, in Section 14.5, when we provide
	implementations of this method for nonspecular reflection functions.


	*/
    virtual Spectrum Sample_f(const Vector &wo, Vector *wi,
                              float u1, float u2, float *pdf) const;

	/*
	P 430

	It can be useful to take the aggregate(集合) behavior of the 4D BRDF or BTDF, defined as a
	function over pairs of directions, and reduce(归纳) it to a 2D function over a single direction,
	or even to a constant value that describes its overall scattering behavior.

	The hemispherical-directional reflectance is a 2D function that gives the total reflection in
	a given direction due to constant illumination over the hemisphere, or, equivalently, total
	reflection over the hemisphere due to light from a given direction

	The BxDF::rho() method computes the reflectance function ρhd，
	Some BxDFs can compute
	this value in closed form, although most use Monte Carlo integration to compute
	an approximation to it. For those BxDFs, the nSamples and samples parameters are used by
	the implementation of the Monte Carlo algorithm; they are explained in Section 14.5.5

	*/
    virtual Spectrum rho(const Vector &wo, int nSamples,
                         const float *samples) const;

	/*
	P 430

	The hemispherical-hemispherical reflectance of a surface, denoted by ρhh, is a constant
	spectral value that gives the fraction of incident light reflected by a surface when the
	incident light is the same from all directions.

	The BxDF::rho() method computes ρhh if no direction ωo is provided. The remaining
	parameters are again used when computing a Monte Carlo estimate, if needed
	*/
    virtual Spectrum rho(int nSamples, const float *samples1,
                         const float *samples2) const;


    virtual float Pdf(const Vector &wi, const Vector &wo) const;

	/*
	Although we are hiding
	the implementation details of the BxDF behind a common interface for reflective and
	transmissive materials, some of the light transport algorithms in Chapter 15 will need
	to distinguish between these two types. Therefore, all BxDFs have a BxDF::type member
	that holds flags from BxDFType. For each BxDF, the flags should have exactly one of BSDF_
	REFLECTION or BSDF_TRANSMISSION set, and exactly one of the diffuse, glossy, and specular
	flags. Note that there is no retro-reflective flag; retro-reflection is treated as glossy reflection
	in this categorization.
	*/
    // BxDF Public Data
    const BxDFType type;
};

/*
It’s handy to define an adapter class that makes it easy to reuse an already-defined BRDF
class as a BTDF, especially for phenomenological models that may be equally plausible
models of transmission.

The BRDFToBTDF class takes a BRDF’s pointer in the constructor
and uses it to implement a BTDF

*/
class BRDFToBTDF : public BxDF {
public:

	/*
	The constructor for the adapter class is simple. It switches the reflection and transmission
	flags of the BxDF::type member.
	*/
    // BRDFToBTDF Public Methods
    BRDFToBTDF(BxDF *b)
        : BxDF(BxDFType(b->type ^ (BSDF_REFLECTION | BSDF_TRANSMISSION))) {
        brdf = b;
    }

	/*
	The adapter needs to convert an incoming vector to the corresponding vector in the
	opposite hemisphere. Fortunately, this is a simple calculation in the shading coordinate
	system, just requiring negation of the vector’s z coordinate
	*/
    static Vector otherHemisphere(const Vector &w) {
        return Vector(w.x, w.y, -w.z);
    }

	/*
	The BRDFToBTDF::otherHemisphere() method is used to reflect a ray into the other hemisphere
	before calling the BRDF’s BxDF::f(), BxDF::rho(), and BxDF::Sample_f() methods.
	We’ll only include f() here since the others are analogous(类似).
	*/
    Spectrum f(const Vector &wo, const Vector &wi) const;
    Spectrum Sample_f(const Vector &wo, Vector *wi, float u1, float u2,
                      float *pdf) const;
    Spectrum rho(const Vector &w, int nSamples, const float *samples) const {
        return brdf->rho(otherHemisphere(w), nSamples, samples);
    }
    Spectrum rho(int nSamples, const float *samples1, const float *samples2) const {
        return brdf->rho(nSamples, samples1, samples2);
    }
    float Pdf(const Vector &wo, const Vector &wi) const;
private:
    BxDF *brdf;
};

/*
It is also useful to take a given BxDF and scale its contribution with a Spectrum value. The
ScaledBxDF wrapper holds a BxDF * and a Spectrum and implements this functionality.
This class is used by the MixMaterial (defined in Section 9.2.3), which creates BSDFs based
on a weighted combination of two other materials.
*/
class ScaledBxDF : public BxDF {
public:
    // ScaledBxDF Public Methods
    ScaledBxDF(BxDF *b, const Spectrum &sc)
        : BxDF(BxDFType(b->type)), bxdf(b), s(sc) {
    }
    Spectrum rho(const Vector &w, int nSamples, const float *samples) const {
        return s * bxdf->rho(w, nSamples, samples);
    }
    Spectrum rho(int nSamples, const float *samples1,
        const float *samples2) const {
        return s * bxdf->rho(nSamples, samples1, samples2);
    }
    Spectrum f(const Vector &wo, const Vector &wi) const;
    Spectrum Sample_f(const Vector &wo, Vector *wi,
        float u1, float u2, float *pdf) const;
private:
    BxDF *bxdf;
    Spectrum s;
};

/*

P 434
The Fresnel equations describe the amount of light reflected from a surface;

compute the fraction of incoming light that is reflected or transmitted.

Due to conservation of energy, the energy transmitted by a dielectric is 1 - Fr

For convenience, we will define an abstract Fresnel class that provides an interface for
computing Fresnel reflection coefficients.
*/
class Fresnel {
public:
    // Fresnel Interface
    virtual ~Fresnel();

	/*
	The only method provided by the Fresnel interface is Fresnel::Evaluate(). Given the
	cosine of the angle made by the incoming direction and the surface normal, it returns
	the amount of light reflected by the surface.
	*/
    virtual Spectrum Evaluate(float cosi) const = 0;
};


/*
FresnelConductor implements this interface for conductors(导电体).
*/
class FresnelConductor : public Fresnel {
public:
    // FresnelConductor Public Methods
    Spectrum Evaluate(float cosi) const;
    FresnelConductor(const Spectrum &e, const Spectrum &kk)
        : eta(e), k(kk) {
    }

private:
    Spectrum eta, k;
};


/*
FresnelDielectric similarly implements the Fresnel interface for dielectric(绝缘体) materials
*/
class FresnelDielectric : public Fresnel {
public:
    // FresnelDielectric Public Methods
    Spectrum Evaluate(float cosi) const;
    FresnelDielectric(float ei, float et) : eta_i(ei), eta_t(et) { }
private:
    float eta_i, eta_t;
};

/*
The FresnelNoOp implementation of the Fresnel interface returns 100% reflection for all
incoming directions. Although this is physically implausible, it is a convenient capability
to have available.
*/
class FresnelNoOp : public Fresnel {
public:
    Spectrum Evaluate(float) const { return Spectrum(1.); }
};

/*

P440

Intuitively, we want the BRDF to be zero everywhere except at the perfect reflection
direction, which suggests the use of the delta distribution.

the delta distribution:
(狄拉克δ函数)在概念上，它是这么一个“函数”：在除了零以外的点函数值都等于零，而其在整个定义域上的积分等于1。
*/
class SpecularReflection : public BxDF {
public:
	/*
	The SpecularReflection class takes a Fresnel object to describe dielectric or conductor
	Fresnel properties and an additional Spectrum object, which is used to scale the reflected
	color.
	*/
    // SpecularReflection Public Methods
    SpecularReflection(const Spectrum &r, Fresnel *f)
        : BxDF(BxDFType(BSDF_REFLECTION | BSDF_SPECULAR)),
          R(r), fresnel(f) {
    }

	/*
	No scattering is returned from
	SpecularReflection::f(), since for an arbitrary pair of directions the delta function
	returns no scattering
	*/
    Spectrum f(const Vector &, const Vector &) const {
        return Spectrum(0.);
    }
    Spectrum Sample_f(const Vector &wo, Vector *wi,
                      float u1, float u2, float *pdf) const;
    float Pdf(const Vector &wo, const Vector &wi) const {
        return 0.;
    }
private:
    // SpecularReflection Private Data
    Spectrum R;
    Fresnel *fresnel;
};

/*

P442

The BTDF for specular transmission. Snell’s law is the basis of the
derivation.

*/
class SpecularTransmission : public BxDF {
public:
    // SpecularTransmission Public Methods
    SpecularTransmission(const Spectrum &t, float ei, float et)
        : BxDF(BxDFType(BSDF_TRANSMISSION | BSDF_SPECULAR)),
          fresnel(ei, et) {
        T = t;
        etai = ei;
        etat = et;
    }
    Spectrum f(const Vector &, const Vector &) const {
        return Spectrum(0.);
    }
    Spectrum Sample_f(const Vector &wo, Vector *wi, float u1, float u2, float *pdf) const;
    float Pdf(const Vector &wo, const Vector &wi) const {
        return 0.;
    }
private:
    // SpecularTransmission Private Data
    Spectrum T;
    float etai, etat;
    FresnelDielectric fresnel;
};

/*
One of the simplest BRDFs is the Lambertian model. It models a perfect diffuse surface
that scatters incident illumination equally in all directions. Although this reflection
model is not physically plausible, it is a good approximation to many real-world surfaces
such as matte paint.
*/
class Lambertian : public BxDF {
public:
    // Lambertian Public Methods
    Lambertian(const Spectrum &reflectance)
        : BxDF(BxDFType(BSDF_REFLECTION | BSDF_DIFFUSE)), R(reflectance) { }
    Spectrum f(const Vector &wo, const Vector &wi) const;
    Spectrum rho(const Vector &, int, const float *) const { return R; }
    Spectrum rho(int, const float *, const float *) const { return R; }
private:
    // Lambertian Private Data
    Spectrum R;
};

/*
P449

Oren and Nayar (1994) observed that real-world objects tend not to exhibit perfect Lambertian
reflection. Specifically, rough surfaces generally appear brighter as the illumination
direction approaches the viewing direction. They developed a reflection model that
describes rough surfaces as a collection of symmetric V-shaped grooves in an effort to
better model effects like these. They further assumed that each individual microfacet
(groove face) exhibited perfect Lambertian reflection and derived a BRDF that models
the aggregate reflection of the collection of grooves. The distribution of microfacets was
modeled with a Gaussian distribution with a single parameter σ, the standard deviation
of the orientation angle.


*/

class OrenNayar : public BxDF {
public:
    // OrenNayar Public Methods
    Spectrum f(const Vector &wo, const Vector &wi) const;
    OrenNayar(const Spectrum &reflectance, float sig)
        : BxDF(BxDFType(BSDF_REFLECTION | BSDF_DIFFUSE)),
          R(reflectance) {
        float sigma = Radians(sig);
        float sigma2 = sigma*sigma;
        A = 1.f - (sigma2 / (2.f * (sigma2 + 0.33f)));
        B = 0.45f * sigma2 / (sigma2 + 0.09f);
    }
private:
    // OrenNayar Private Data
    Spectrum R;
    float A, B;
};

/*
P454
We now use the Torrance–Sparrow model to implement a general microfacet-based
BRDF. It takes a pointer to an abstract MicrofacetDistribution class, which provides
a single method to compute the D term of the Torrance–Sparrow model. This function,
MicrofacetDistribution::D(), gives the probability density for microfacets to be
oriented with normal ωh.
*/
class MicrofacetDistribution {
public:
    // MicrofacetDistribution Interface
    virtual ~MicrofacetDistribution() { }
    virtual float D(const Vector &wh) const = 0;
    virtual void Sample_f(const Vector &wo, Vector *wi,
                          float u1, float u2, float *pdf) const = 0;
    virtual float Pdf(const Vector &wo, const Vector &wi) const = 0;
};

/*
P452

One of the first microfacet models for computer graphics was developed by Torrance
and Sparrow (1967) to model metallic surfaces. They modeled surfaces as collections of
perfectly smooth mirrored microfacets. The surface is statistically described by a distribution
function D(ωh) that gives the probability that a microfacet has orientation ωh
*/
class Microfacet : public BxDF {
public:
    // Microfacet Public Methods
    Microfacet(const Spectrum &reflectance, Fresnel *f,
        MicrofacetDistribution *d);
    Spectrum f(const Vector &wo, const Vector &wi) const;
    float G(const Vector &wo, const Vector &wi, const Vector &wh) const {
        float NdotWh = AbsCosTheta(wh);
        float NdotWo = AbsCosTheta(wo);
        float NdotWi = AbsCosTheta(wi);
        float WOdotWh = AbsDot(wo, wh);
        return min(1.f, min((2.f * NdotWh * NdotWo / WOdotWh),
                            (2.f * NdotWh * NdotWi / WOdotWh)));
    }
    Spectrum Sample_f(const Vector &wo, Vector *wi,
                              float u1, float u2, float *pdf) const;
    float Pdf(const Vector &wo, const Vector &wi) const;
private:
    // Microfacet Private Data
    Spectrum R;
    MicrofacetDistribution *distribution;
    Fresnel *fresnel;
};

/*
P455
Blinn (1977) proposed a model where the distribution of microfacet normals is approximated
by an exponential falloff. The most likely microfacet orientation in this model is
in the surface normal direction, falling off to no microfacets oriented perpendicular to
the normal. For smooth surfaces, this falloff happens very quickly; for rough surfaces, it
is more gradual.
*/
class Blinn : public MicrofacetDistribution {
public:
    Blinn(float e) { if (e > 10000.f || isnan(e)) e = 10000.f;
                     exponent = e; }
    // Blinn Public Methods
    float D(const Vector &wh) const {
        float costhetah = AbsCosTheta(wh);
        return (exponent+2) * INV_TWOPI * powf(costhetah, exponent);
    }
    virtual void Sample_f(const Vector &wi, Vector *sampled_f, float u1, float u2, float *pdf) const;
    virtual float Pdf(const Vector &wi, const Vector &wo) const;
private:
    float exponent;
};

/*
P457
Ashikhmin and Shirley (2000, 2002) developed a microfacet distribution
function for modeling the appearance of anisotropic surfaces. Recall that an anisotropic
BRDF is one where the reflection characteristics at a point vary as the surface is rotated
about that point in the plane perpendicular to the surface normal. Brushed metals(磨砂金属) and
some types of fabric(织物) exhibit(显示) anisotropy.
*/
class Anisotropic : public MicrofacetDistribution {
public:
    // Anisotropic Public Methods
    Anisotropic(float x, float y) {
        ex = x; ey = y;
        if (ex > 10000.f || isnan(ex)) ex = 10000.f;
        if (ey > 10000.f || isnan(ey)) ey = 10000.f;
    }
    float D(const Vector &wh) const {
        float costhetah = AbsCosTheta(wh);
        float d = 1.f - costhetah * costhetah;
        if (d == 0.f) return 0.f;
        float e = (ex * wh.x * wh.x + ey * wh.y * wh.y) / d;
        return sqrtf((ex+2.f) * (ey+2.f)) * INV_TWOPI * powf(costhetah, e);
    }
    void Sample_f(const Vector &wo, Vector *wi, float u1, float u2, float *pdf) const;
    float Pdf(const Vector &wo, const Vector &wi) const;
    void sampleFirstQuadrant(float u1, float u2, float *phi, float *costheta) const;
private:
    float ex, ey;
};

/*
P460
Most BRDF models in graphics do not account for the fact that Fresnel reflection reduces
the amount of light that reaches the bottom level of layered objects. Consider a polished
wood table or a wall with glossy paint: if you look at their surfaces head-on, you primarily
see the wood or the paint pigment color. As you move your viewpoint toward a glancing
angle, you see less of the underlying color as it is overwhelmed by increasing glossy
reflection due to Fresnel effects.

In this section, we will implement a BRDF model developed by Ashikhmin and Shirley
(2000, 2002) that models a diffuse underlying surface with a glossy specular surface above
it. The effect of reflection from the diffuse surface is modulated by the amount of energy
left after Fresnel effects have been considered. Figure 8.21 shows this idea: When the
incident direction is close to the normal, most light is transmitted to the diffuse layer
and the diffuse term dominates. When the incident direction is close to glancing, glossy
reflection is the primary mode of reflection. The car model in Figures 12.14 and 12.15
uses this BRDF for its paint.

*/
class FresnelBlend : public BxDF {
public:
    // FresnelBlend Public Methods
    FresnelBlend(const Spectrum &Rd,
                 const Spectrum &Rs,
                 MicrofacetDistribution *dist);
    Spectrum f(const Vector &wo, const Vector &wi) const;
    Spectrum SchlickFresnel(float costheta) const {
        return Rs + powf(1 - costheta, 5.f) * (Spectrum(1.) - Rs);
    }
    Spectrum Sample_f(const Vector &wi, Vector *sampled_f, float u1, float u2, float *pdf) const;
    float Pdf(const Vector &wi, const Vector &wo) const;
private:
    // FresnelBlend Private Data
    Spectrum Rd, Rs;
    MicrofacetDistribution *distribution;
};

/*
P462
Using measured data about the reflection properties of real surfaces is one of the most
effective approaches for rendering realistic materials

However, parameterized BRDF models are often not flexible enough to model the full
complexity of scattering characteristics of many interesting surfaces. If accurate measured
reflection data is available for a surface of interest, then directly using it for rendering
can faithfully re-create the surface’s appearance. In this section, we’ll implement
BxDFs that interpolate sampled BRDF values directly. While this is a memory-intensive
approach for scenes that use many different measured BRDFs, it works well for reflection
distributions that don’t fit analytic models well and can provide a reference for comparison
to other approaches.

Rendering with measured BRDF data does have challenges. It can be difficult to make
adjustments for artistic purposes (“like that, just a little more shiny”).5 If there is excessive
error in the data, results may be poor as well: especially for highly specular and
anisotropic surfaces, the 4D BRDF must be densely sampled, leading to a large amount
of data to store and many measurements to be taken. Finally, BRDFs may have considerable
variation in their values, which adds to the challenges of acquiring accurate BRDF
data sets.

Therefore, pbrt has implementations of two BxDFs for using measured reflection data for
rendering. The first, IrregIsotropicBRDF interpolates from irregularly spaced samples of
isotropic BRDFs. 

IrregIsotropicBRDF supports irregularly sampled isotropic BRDF data. Given a pair of
directions, it finds a few BRDF samples that are nearby in the space of incident and
outgoing directions and interpolates between their values, based on how close they are
to the actual directions.

We’d therefore like to use a mapping function for isotropic BRDFs that takes a 4D pair of
directions ωi and ωo and converts them into a point in a three-dimensional space such
that the distance between two points is meaningful, the isotropy of the BRDF is reflected,
and reciprocity is represented.

(简单地理解就是查表)

*/
class IrregIsotropicBRDF : public BxDF {
public:
    // IrregIsotropicBRDF Public Methods
    IrregIsotropicBRDF(const KdTree<IrregIsotropicBRDFSample> *d)
        : BxDF(BxDFType(BSDF_REFLECTION | BSDF_GLOSSY)), isoBRDFData(d) { }
    Spectrum f(const Vector &wo, const Vector &wi) const;
private:
    // IrregIsotropicBRDF Private Data
    const KdTree<IrregIsotropicBRDFSample> *isoBRDFData;
};


/*
The second, RegularHalfangleBRDF, supports the regular sampling used
byMatusik et al.’s data set (2003a, 2003b).

(简单地理解就是查表)

*/
class RegularHalfangleBRDF : public BxDF {
public:
    // RegularHalfangleBRDF Public Methods
    RegularHalfangleBRDF(const float *d, uint32_t nth, uint32_t ntd,
                         uint32_t npd)
        : BxDF(BxDFType(BSDF_REFLECTION | BSDF_GLOSSY)), brdf(d),
          nThetaH(nth), nThetaD(ntd), nPhiD(npd) { }
    Spectrum f(const Vector &wo, const Vector &wi) const;
private:
    // RegularHalfangleBRDF Private Data
    const float *brdf;
    const uint32_t nThetaH, nThetaD, nPhiD;
};



// BSSRDF Declarations
class BSSRDF {
public:
    // BSSRDF Public Methods
    BSSRDF(const Spectrum &sa, const Spectrum &sps, float et)
        : e(et), sig_a(sa), sigp_s(sps) { }
    float eta() const { return e; }
    Spectrum sigma_a() const { return sig_a; }
    Spectrum sigma_prime_s() const { return sigp_s; }
private:
    // BSSRDF Private Data
    float e;
    Spectrum sig_a, sigp_s;
};



// BSDF Inline Method Definitions
inline void BSDF::Add(BxDF *b) {
    Assert(nBxDFs < MAX_BxDFS);
    bxdfs[nBxDFs++] = b;
}


inline int BSDF::NumComponents(BxDFType flags) const {
    int num = 0;
    for (int i = 0; i < nBxDFs; ++i)
        if (bxdfs[i]->MatchesFlags(flags)) ++num;
    return num;
}



#endif // PBRT_CORE_REFLECTION_H
