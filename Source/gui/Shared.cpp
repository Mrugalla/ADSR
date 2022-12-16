#include "Shared.h"

namespace gui
{
    Colours Colours::c{};

    Font getFont(const char* ttf, size_t size)
    {
        auto typeface = juce::Typeface::createSystemTypefaceFor(ttf, size);
        return Font(typeface);
    }

    Font getFontNEL()
    {
        return getFont(BinaryData::nel19_ttf, BinaryData::nel19_ttfSize);
    }
    Font getFontLobster()
    {
        return getFont(BinaryData::LobsterRegular_ttf, BinaryData::LobsterRegular_ttfSize);
    }
    Font getFontMsMadi()
    {
        return getFont(BinaryData::MsMadiRegular_ttf, BinaryData::MsMadiRegular_ttfSize);
    }
    Font getFontDosisSemiBold()
    {
        return getFont(BinaryData::DosisSemiBold_ttf, BinaryData::DosisSemiBold_ttfSize);
    }
    Font getFontDosisBold()
    {
        return getFont(BinaryData::DosisBold_ttf, BinaryData::DosisBold_ttfSize);
    }
    Font getFontDosisExtraBold()
    {
        return getFont(BinaryData::DosisExtraBold_ttf, BinaryData::DosisExtraBold_ttfSize);
    }
    Font getFontDosisLight()
    {
        return getFont(BinaryData::DosisLight_ttf, BinaryData::DosisLight_ttfSize);
    }
    Font getFontDosisExtraLight()
    {
        return getFont(BinaryData::DosisExtraLight_ttf, BinaryData::DosisExtraLight_ttfSize);
    }
    Font getFontDosisMedium()
    {
        return getFont(BinaryData::DosisMedium_ttf, BinaryData::DosisMedium_ttfSize);
    }
    Font getFontDosisRegular()
    {
        return getFont(BinaryData::DosisRegular_ttf, BinaryData::DosisRegular_ttfSize);
    }
    Font getFontDosisVariable()
    {
        return getFont(BinaryData::DosisVariableFont_wght_ttf, BinaryData::DosisVariableFont_wght_ttfSize);
    }

	// Colours

    Colours::Colours() :
        cols(),
        props(nullptr)
    {
        setInternal(ColourID::Transp, Colour(0x00000000));
        setInternal(ColourID::Abort, Colour(0xffff0000));
    }

    Colour Colours::defaultColour() noexcept
    {
        return Colour(0xffa8e753);
    }

    void Colours::init(Props* p)
    {
        props = p;
        if (props->isValidFile())
        {
            const auto colStr = props->getValue(coloursID(), defaultColour().toString());
            set(juce::Colour::fromString(colStr));
        }
    }

    bool Colours::set(Colour col)
    {
        if (props->isValidFile())
        {
            setInternal(ColourID::Interact, col);
            props->setValue(coloursID(), col.toString());

            setInternal(ColourID::Bg, col.darker(8.f).withMultipliedSaturation(.15f));
            setInternal(ColourID::Txt, col.withRotatedHue(-1.f / 9.f).withMultipliedBrightness(2.f));
            setInternal(ColourID::Mod, col.withRotatedHue(1.f / 3.f));
            setInternal(ColourID::Bias, col.withRotatedHue(.5f));
            setInternal(ColourID::Darken, col.darker(3.f).withMultipliedAlpha(.3f));
            setInternal(ColourID::Hover, col.withMultipliedSaturation(2.f).brighter(2.f).withMultipliedAlpha(.3f));
            setInternal(ColourID::Inactive, col.withMultipliedSaturation(.1f));

            if (props->needsToBeSaved())
            {
                props->save();
                props->sendChangeMessage();
                return true;
            }
        }
        return false;
    }

    Colour Colours::operator()(ColourID i) const noexcept
    {
        return get(static_cast<int>(i));
    }

    Colour Colours::operator()(int i) const noexcept
    {
        return get(i);
    }

    Colour Colours::get(int i) const noexcept
    {
        return cols[i];
    }

    void Colours::setInternal(ColourID cID, Colour col) noexcept
    {
        cols[static_cast<int>(cID)] = col;
    }

    String Colours::coloursID()
    {
        return "coloursMain";
    }
}