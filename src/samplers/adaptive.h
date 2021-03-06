
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

#ifndef PBRT_SAMPLERS_ADAPTIVE_H
#define PBRT_SAMPLERS_ADAPTIVE_H

// samplers/adaptive.h*
#include "pbrt.h"
#include "sampler.h"


/*
The AdaptiveSampler implements a simple adaptive sampling algorithm that attempts
to more-efficiently generate high-quality images by adding extra samples in parts of the
image that are more complex than others.

(不合格的话，就重新采样多一次，无论这次合不合格，都会接受这次采样结果)

The AdaptiveSampler
supports two simple refinement criteria(标准). The first is based on checking to see if different
shapes are intersected by different samples, which indicates a likely geometric discontinuity,
and the second checks for excessive(过分) contrast(对比) between the colors of different samples.

*/

// AdaptiveSampler Declarations
class AdaptiveSampler : public Sampler {
public:
    // AdaptiveSampler Public Methods
    AdaptiveSampler(int xstart, int xend, int ystart, int yend,
        int minSamples, int maxSamples, const string &method,
        float sopen, float sclose);
    Sampler *GetSubSampler(int num, int count);
    ~AdaptiveSampler();
    int RoundSize(int size) const {
        return RoundUpPow2(size);
    }
    int MaximumSampleCount() { return maxSamples; }
    int GetMoreSamples(Sample *sample, RNG &rng);

	/*
	If further sampling is indicated,
	ReportResults() sets supersamplePixel to true and leaves (xPos, yPos) unchanged.
	Thus, the next call to GetMoreSamples() will generate a new set of maxSamples
	samples for the pixel. When these samples are provided to ReportResults(), super
	samplePixel is reset to false and the current pixel is advanced.
	P387
	*/
    bool ReportResults(Sample *samples, const RayDifferential *rays,
        const Spectrum *Ls, const Intersection *isects, int count);
private:
    // AdaptiveSampler Private Methods
    bool needsSupersampling(Sample *samples, const RayDifferential *rays,
        const Spectrum *Ls, const Intersection *isects, int count);

    // AdaptiveSampler Private Data
    int xPos, yPos;
    int minSamples, maxSamples;
    float *sampleBuf;
    enum AdaptiveTest { ADAPTIVE_COMPARE_SHAPE_ID,
                        ADAPTIVE_CONTRAST_THRESHOLD };
    AdaptiveTest method;
    bool supersamplePixel;
};


AdaptiveSampler *CreateAdaptiveSampler(const ParamSet &params, const Film *film,
    const Camera *camera);

#endif // PBRT_SAMPLERS_ADAPTIVE_H
