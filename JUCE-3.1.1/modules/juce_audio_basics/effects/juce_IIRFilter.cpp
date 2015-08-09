/*
  ==============================================================================

   This file is part of the JUCE library.
   Copyright (c) 2013 - Raw Material Software Ltd.

   Permission is granted to use this software under the terms of either:
   a) the GPL v2 (or any later version)
   b) the Affero GPL v3

   Details of these licenses can be found at: www.gnu.org/licenses

   JUCE is distributed in the hope that it will be useful, but WITHOUT ANY
   WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
   A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

   ------------------------------------------------------------------------------

   To release a closed-source product which uses JUCE, commercial licenses are
   available: visit www.juce.com for more information.

  ==============================================================================
*/

//==============================================================================


//==============================================================================
IIRFilter::IIRFilter() noexcept
    : v1 (0), v2 (0), active (false)
{
}

IIRFilter::IIRFilter (const IIRFilter& other) noexcept
    : v1 (0), v2 (0), active (other.active)
{
    //const SpinLock::ScopedLockType sl (other.processLock);
    coefficients = other.coefficients;
}

IIRFilter::~IIRFilter() noexcept
{
}

//==============================================================================
void IIRFilter::makeInactive() noexcept
{
    //const SpinLock::ScopedLockType sl (processLock);
    active = false;
}

/*
void IIRFilter::setCoefficients (const IIRCoefficients& newCoefficients) noexcept
{
    //const SpinLock::ScopedLockType sl (processLock);

    coefficients = newCoefficients;
    active = true;
}
*/

//==============================================================================
void IIRFilter::reset() noexcept
{
    //const SpinLock::ScopedLockType sl (processLock);
    v1 = v2 = 0;
}
/*
float IIRFilter::processSingleSampleRaw (const float in) noexcept
{
    if(!active)
      return in;
  
    float out = coefficients.coefficients[0] * in + v1;

    JUCE_SNAP_TO_ZERO (out);

    v1 = coefficients.coefficients[1] * in - coefficients.coefficients[3] * out + v2;
    v2 = coefficients.coefficients[2] * in - coefficients.coefficients[4] * out;

    return out;
}
*/

void IIRFilter::processSamples (float* const samples, const int numSamples) noexcept
{
    //const SpinLock::ScopedLockType sl (processLock);

    if (active)
    {
        const float c0 = coefficients.coefficients[0];
        const float c1 = coefficients.coefficients[1];
        const float c2 = coefficients.coefficients[2];
        const float c3 = coefficients.coefficients[3];
        const float c4 = coefficients.coefficients[4];
        float lv1 = v1, lv2 = v2;

        for (int i = 0; i < numSamples; ++i)
        {
            const float in = samples[i];
            const float out = c0 * in + lv1;
            samples[i] = out;

            lv1 = c1 * in - c3 * out + lv2;
            lv2 = c2 * in - c4 * out;
        }

        JUCE_SNAP_TO_ZERO (lv1);  v1 = lv1;
        JUCE_SNAP_TO_ZERO (lv2);  v2 = lv2;
    }
}

#undef JUCE_SNAP_TO_ZERO
