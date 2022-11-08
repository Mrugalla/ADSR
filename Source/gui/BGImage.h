#pragma once
#include "Shared.h"

namespace gui
{
	inline void makeBGSpace(Image& bgImage, float thicc, int width, int height)
	{
        bgImage = Image(Image::ARGB, width / 4, height / 4, true);
		
		Random rand;
        auto w = static_cast<float>(width);
        auto h = static_cast<float>(height);
        BoundsF bounds(0.f, 0.f, w, h);
        const Colour trans(0x000000);
        
        {
            Graphics g{ bgImage };

            { // draw fog
                Colour col(0x03ffffff);

                const auto numFogs = 2;
                for (auto i = 0; i < numFogs; ++i)
                {
                    const PointF pt0
                    (
                        rand.nextFloat() * w,
                        rand.nextFloat() * h
                    );
                    const PointF pt1
                    (
                        rand.nextFloat() * w,
                        rand.nextFloat() * h
                    );
                    const Gradient gradient(trans, pt0, col, pt1, false);
                    g.setGradientFill(gradient);
                    g.fillRect(bounds);
                }
            }

            { // draw dust
                const auto randWidth = .05f;
                const auto randRange = randWidth * 2.f;
                auto col = Colours::c(ColourID::Mod).withAlpha(.02f);

                const auto numDusts = 13;
                for (auto i = 0; i < numDusts; ++i)
                {
                    col = col.withRotatedHue(rand.nextFloat() * randWidth - randRange);

                    const PointF pt0
                    (
                        rand.nextFloat() * w,
                        rand.nextFloat() * h
                    );
                    const PointF pt1
                    (
                        rand.nextFloat() * w,
                        rand.nextFloat() * h
                    );
                    const Gradient gradient(trans, pt0, col, pt1, true);
                    g.setGradientFill(gradient);
                    g.fillRect(bounds);
                }
            }
        }

        bgImage = bgImage.rescaled(width, height, Graphics::lowResamplingQuality);
		w = static_cast<float>(bgImage.getWidth());
		h = static_cast<float>(bgImage.getHeight());
        bounds = BoundsF(0.f, 0.f, w, h);

        Graphics g{ bgImage };

        { // draw stars
            const auto maxStarSize = thicc * 1.5f;

            const auto numStars = 128;
            for (auto i = 0; i < numStars; ++i)
            {
                const auto x = rand.nextFloat() * w;
                const auto y = rand.nextFloat() * h;
                const auto starSize = 1.f + rand.nextFloat() * maxStarSize;
                auto alpha = rand.nextFloat() * .9f;
                alpha = .05f + alpha * alpha * alpha * alpha * alpha;
                auto habitable = rand.nextFloat();
                habitable = habitable * habitable * habitable * habitable * habitable;
                const auto col = Colour(0xffffffff)
                    .interpolatedWith(Colours::c(ColourID::Mod), habitable)
                    .withAlpha(alpha);
                g.setColour(col);
                g.fillEllipse(x, y, starSize, starSize);
            }
        }

        { // posterize the image

            const float depth = 32.f;
            const auto dInv = 1.f / depth;

            for (auto y0 = 0; y0 < bgImage.getHeight(); ++y0)
            {
                for (auto x0 = 0; x0 < bgImage.getWidth(); ++x0)
                {
                    const auto pxl = bgImage.getPixelAt(x0, y0);
                    juce::uint8 rgb[3];
                    rgb[0] = pxl.getRed();
                    rgb[1] = pxl.getGreen();
                    rgb[2] = pxl.getBlue();
                    for (auto j = 0; j < 3; ++j)
                    {
                        const auto val = static_cast<float>(rgb[j]) * dInv;
                        const auto newVal = static_cast<juce::uint8>(std::round(val) * depth);
                        rgb[j] = newVal;
                    }
                    const auto col = Colour::fromRGBA(rgb[0], rgb[1], rgb[2], rgb[3]);
                    bgImage.setPixelAt(x0, y0, pxl.interpolatedWith(col, .1f));
                }
            }
        }
	}
}