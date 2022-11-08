#include "SpectroBeamComp.h"

namespace gui
{
	template<size_t Order>
	SpectroBeamComp<Order>::SpectroBeamComp(Utils& u, SpecBeam& _beam) :
		Comp(u, "Spectro Beam", CursorType::Default),
		mainColCID(ColourID::Hover),
		xen(u.audioProcessor.xenManager),
		beam(_beam),
		img(Image::RGB, Size, 1, true)
	{
		setInterceptsMouseClicks(false, false);
		startTimerHz(60);
		setOpaque(true);
	}

	template<size_t Order>
	void SpectroBeamComp<Order>::paint(Graphics& g)
	{
		g.setImageResamplingQuality(Graphics::lowResamplingQuality);
		g.drawImage(img, getLocalBounds().toFloat());
	}

	template<size_t Order>
	void SpectroBeamComp<Order>::timerCallback()
	{
		auto ready = beam.ready.load();
		if (!ready)
			return;

		const auto Fs = static_cast<float>(utils.audioProcessor.getSampleRate());
		const auto fsInv = 1.f / Fs;
		const auto colBase = Colours::c(ColourID::Bg);
		const auto col = Colours::c(mainColCID);
		const auto buf = beam.buffer.data();

		const auto lowestDb = -12.f;
		const auto highestDb = 6.f;
		const auto rangeDb = highestDb - lowestDb;
		const auto rangeDbInv = 1.f / rangeDb;

		for (auto x = 0; x < Size; ++x)
		{
			const auto norm = static_cast<float>(x) * SizeInv;
			const auto pitch = norm * 128.f;
			const auto freqHz = xen.noteToFreqHzWithWrap(pitch + xen.getXen());
			const auto binIdx = freqHz * fsInv * SizeF;

			const auto bin = interpolate::lerp(buf, binIdx);
			const auto magDb = audio::gainToDecibel(bin);
			const auto magMapped = juce::jlimit(0.f, 1.f, (magDb - lowestDb) * rangeDbInv);
			const auto nCol = colBase.interpolatedWith(col, magMapped);
			img.setPixelAt(x, 0, nCol);
		}

		beam.ready.store(false);
		repaint();
	}

	template struct SpectroBeamComp<8>;
	template struct SpectroBeamComp<9>;
	template struct SpectroBeamComp<10>;
	template struct SpectroBeamComp<11>;
	template struct SpectroBeamComp<12>;
	template struct SpectroBeamComp<13>;
	template struct SpectroBeamComp<14>;
	template struct SpectroBeamComp<15>;
}