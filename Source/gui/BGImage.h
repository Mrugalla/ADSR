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
                    juce::uint8 rgb[3] =
                    {
                        pxl.getRed(),
                        pxl.getGreen(),
                        pxl.getBlue()
                    };
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

	inline void makeBGBlurredCurves(Image& bgImage, float thicc, int width, int height)
	{
        bgImage = Image(Image::ARGB, width / 4, height / 4, false);

        Random rand;
		
        auto bgCol = Colours::c(ColourID::Bg);

        Graphics g{ bgImage };
        g.fillAll(bgCol);
		
        const auto srgbToLinear = [](float srgb)
        {
            if (srgb <= 0.0404482362771082f)
                return srgb / 12.92f;
             return std::pow((srgb + 0.055f) / 1.055f, 2.4f);
        };

        const auto paintedPercentage = [srgbToLinear, bgCol](const Image& img)
        {
            const auto numPixels = static_cast<float>(img.getWidth() * img.getHeight());
			
            const auto bgRedLinear = srgbToLinear(bgCol.getFloatRed());
            const auto bgGreenLinear = srgbToLinear(bgCol.getFloatGreen());
            const auto bgBlueLinear = srgbToLinear(bgCol.getFloatBlue());

            auto z = 0.f;
            for (auto y = 0; y < img.getHeight(); ++y)
            {
                for (auto x = 0; x < img.getWidth(); ++x)
                {
                    const auto pxl = img.getPixelAt(x, y);
                    const auto redLinear = srgbToLinear(pxl.getFloatRed());
                    const auto greenLinear = srgbToLinear(pxl.getFloatGreen());
                    const auto blueLinear = srgbToLinear(pxl.getFloatBlue());

					auto diff = std::abs(redLinear - bgRedLinear);
					diff += std::abs(greenLinear - bgGreenLinear);
					diff += std::abs(blueLinear - bgBlueLinear);
					diff /= 3.f;
					z += diff;
                }
            }
            return z / numPixels;
        };

        const auto randPointAtEdge = [&r = rand](const Image& img)
        {
            const auto w = static_cast<float>(img.getWidth() - 1);
            const auto h = static_cast<float>(img.getHeight() - 1);

            PointF pt;
            const auto edge = r.nextFloat();
            if (edge < .25f)
            {
                pt.x = r.nextFloat() * w;
                pt.y = 0.f;
            }
            else if (edge < .5f)
            {
                pt.x = r.nextFloat() * w;
                pt.y = h;
            }
            else if (edge < .75f)
            {
                pt.x = 0.f;
                pt.y = r.nextFloat() * h;
            }
            else
            {
                pt.x = w;
                pt.y = r.nextFloat() * h;
            }

            return pt;
        };

        const auto w = static_cast<float>(bgImage.getWidth());
        const auto h = static_cast<float>(bgImage.getHeight());
        const auto maxDimen = std::max(w, h);
		const auto maxDimenInv = 1.f / maxDimen;
		
        PointF centre
        {
            w * .5f,
            h * .5f
        };

        auto timeOut = 1000;
        while (paintedPercentage(bgImage) < .0008f && timeOut != 0)
        {
            auto ptStart = randPointAtEdge(bgImage);
            auto dir = rand.nextFloat() * Tau;
            auto curviness = .001f;

			const auto mainCol = rand.nextBool() ? Colours::c(ColourID::Mod) : Colours::c(ColourID::Interact);
			
            while (ptStart.x >= 0.f && ptStart.x < w && ptStart.y >= 0.f && ptStart.y < h)
            {
				curviness += (rand.nextFloat() - .5f) * .1f;
                dir += (rand.nextFloat() * PiHalf - Pi) * curviness;
                auto dist = thicc;
                PointF ptEnd
                (
                    ptStart.x + std::cos(dir) * dist,
                    ptStart.y + std::sin(dir) * dist
                );
				const auto centreDist = centre.getDistanceFrom(ptEnd) * maxDimenInv;
				const auto colZ = centreDist * centreDist * centreDist * centreDist;
                const auto col = bgCol.interpolatedWith(mainCol, colZ);
                g.setColour(col);
                
                g.drawLine({ ptStart, ptEnd }, thicc * rand.nextFloat());
                
                ptStart = ptEnd;
            }

            --timeOut;
        }
        
        bgImage = bgImage.rescaled(width, height, Graphics::mediumResamplingQuality);
	}
}