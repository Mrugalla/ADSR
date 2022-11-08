/*
  ==============================================================================

    OverdriveReNEO.cpp
    Created: 2 Nov 2022 11:26:06pm
    Author:  Hans

  ==============================================================================
*/

#include "OverdriveReNEO.h"

namespace audio
{
	// OverdriveReNeo::Filter

	OverdriveReNeo::Filter::Filter() :
		filtr{ 0.f, 0.f }
	{}

	void OverdriveReNeo::Filter::reset() noexcept
	{
		for (auto& f : filtr)
			f.reset();
	}

	void OverdriveReNeo::Filter::setFreq(float hz, float Fs) noexcept
	{
		filtr[0].makeFromDecayInHz(hz, Fs);
		filtr[1].copyCutoffFrom(filtr[0]);
	}

	float OverdriveReNeo::Filter::operator()(float x) noexcept
	{
		for (auto& f : filtr)
			x = f(x);
		return x;
	}

	// OverdriveReNeo

	OverdriveReNeo::OverdriveReNeo(PinkNoise& pinkNoise, const Range& _muffleRange) :
		filtr(),
		muffled(0.f),
		pan(0.f),
		drive(2.f),
		scrap(0.f),

		muffleGain(pinkNoise, _muffleRange, 7),
		driveGain(pinkNoise, { 0.f, 1.f }, 7),
		scrapGain(pinkNoise, { 0.f, 1.f }, 7),

		panVecs(),
		Fs(1.f)
	{
		initAutoGain();
	}

	void OverdriveReNeo::prepare(float sampleRate, int blockSize)
	{
		resetFilter();

		muffled.prepare(sampleRate, blockSize, 20.f);
		pan.prepare(sampleRate, blockSize, 20.f);
		drive.prepare(sampleRate, blockSize, 20.f);
		scrap.prepare(sampleRate, blockSize, 20.f);

		for (auto& panVec : panVecs)
			panVec.resize(blockSize);

		Fs = sampleRate;
	}

	void OverdriveReNeo::operator()(float** samples, int numChannels, int numSamples,
		float driveP, float muffleP, float panP, float scrapP) noexcept
	{
		const auto muffleBuf = muffled(muffleP, numSamples);
		const auto driveBuf = drive(2.f + GlueBoost * driveP, numSamples);
		const auto scrapBuf = scrap(scrapP, numSamples);

		if (numChannels == 1)
			processBlockMono(samples[0], numSamples, muffleBuf, driveBuf, filtr[0], scrapBuf);
		else
			processBlockStereo(samples, numChannels, numSamples, muffleBuf, driveBuf, panP, scrapBuf);
	}

	void OverdriveReNeo::resetFilter() noexcept
	{
		for (auto& f : filtr)
			f.reset();
	}

	void OverdriveReNeo::initAutoGain()
	{
		muffleGain.evaluate
		(
			[&](float sampleRate, int blockSize)
			{
				prepare(sampleRate, blockSize);
			},
			[&](float** samples, int numChannels, int numSamples, float valPDenorm)
			{
				operator()(samples, numChannels, numSamples, 0.f, valPDenorm, 0.f, 0.f);
			},
				[&]()
			{
				resetFilter();
			}
			);

		driveGain.evaluate
		(
			[&](float sampleRate, int blockSize)
			{
				prepare(sampleRate, blockSize);
			},
			[&](float** samples, int numChannels, int numSamples, float valPDenorm)
			{
				operator()(samples, numChannels, numSamples, valPDenorm, 20000.f, 0.f, 0.f);
			},
				[&]()
			{
				resetFilter();
			}
			);

		scrapGain.evaluate
		(
			[&](float sampleRate, int blockSize)
			{
				prepare(sampleRate, blockSize);
			},
			[&](float** samples, int numChannels, int numSamples, float valPDenorm)
			{
				operator()(samples, numChannels, numSamples, .5f, 20000.f, 0.f, valPDenorm);
			},
				[&]()
			{
				resetFilter();
			}
			);
	}

	void OverdriveReNeo::processBlockMono(float* smpls, int numSamples,
		const float* muffleBuf, const float* driveBuf, Filter& fltr, const float* scrapBuf) noexcept
	{
		for (auto s = 0; s < numSamples; ++s)
		{
			fltr.setFreq(muffleBuf[s], Fs);

			auto smpl = smpls[s];

			smpl = waveshape(smpl, driveBuf[s], scrapBuf[s]);
			smpl = fltr(smpl);

			smpls[s] = applyAutoGain(smpl, muffleBuf[s], driveBuf[s], scrapBuf[s]);
		}
	}

	void OverdriveReNeo::processBlockStereo(float** samples, int numChannels, int numSamples,
		const float* muffleBuf, const float* driveBuf, float panP, const float* scrapBuf) noexcept
	{
		updatePanVecs(panP, numSamples);

		for (auto ch = 0; ch < numChannels; ++ch)
		{
			auto smpls = samples[ch];
			auto& fltr = filtr[ch];
			auto panVec = panVecs[PanVec::DriveC].data();

			for (auto s = 0; s < numSamples; ++s)
			{
				const auto panGain = panVecs[PanVec::Gain][s];
				const auto panDrive = panVecs[PanVec::DriveL + ch][s];

				panVec[s] = 2.f + panDrive * (driveBuf[s] - 2.f) + (1.f - panGain) * (driveBuf[s] - 2.f);
			}

			processBlockMono(smpls, numSamples, muffleBuf, panVec, fltr, scrapBuf);
		}
	}

	float OverdriveReNeo::waveshape(float x, float p, float xy) const noexcept
	{
		x *= DriveBoost;

		const auto xx = x * x * x * x;
		const auto d = 1.f / (p + xy * (xx * p - p));

		if (x > 0.f)
			return std::pow(x, d);
		return -std::pow(-x, d);
	}

	float OverdriveReNeo::applyAutoGain(float x, float ms, float ds, float scp) noexcept
	{
		x = muffleGain.fromDenorm(x, ms);
		x = driveGain(x, (ds - 2.f) * GlueBoostInv);
		x = scrapGain(x, scp);

		return x;
	}

	void OverdriveReNeo::updatePanVecs(float panP, int numSamples) noexcept
	{
		const auto panBuf = pan(panP, numSamples);
		for (auto s = 0; s < numSamples; ++s)
		{
			const auto panS = panBuf[s];
			const auto panGain = panS * panS;
			panVecs[PanVec::Gain][s] = panGain;
			if (panS < 0.f)
			{
				panVecs[PanVec::DriveL][s] = panGain;
				panVecs[PanVec::DriveR][s] = 0.f;
			}
			else
			{
				panVecs[PanVec::DriveL][s] = 0.f;
				panVecs[PanVec::DriveR][s] = panGain;
			}
		}
	}
}