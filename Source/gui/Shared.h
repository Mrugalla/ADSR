#pragma once
#include "Using.h"

namespace gui
{
    enum class ColourID
    {
        Bg,
        Txt,
        Abort,
        Interact,
        Inactive,
        Darken,
        Hover,
        Transp,
        Mod,
        Bias,
        NumCols
    };

    static constexpr int NumColours = static_cast<int>(ColourID::NumCols);

    class Colours
    {
        using Array = std::array<Colour, static_cast<int>(ColourID::NumCols)>;
    public:
        Colours();

        Colour defaultColour() noexcept;

        void init(Props*);

        bool set(Colour);

        Colour operator()(ColourID) const noexcept;

        Colour operator()(int) const noexcept;

        Colour get(int) const noexcept;

        static Colours c;
    private:
        Array cols;
        Props* props;

        void setInternal(ColourID, Colour) noexcept;

        String coloursID();
    };

    // GET FONT
	
    Font getFontNEL();
    Font getFontLobster();
    Font getFontMsMadi();
    Font getFontDosisSemiBold();
    Font getFontDosisBold();
    Font getFontDosisExtraBold();
    Font getFontDosisLight();
    Font getFontDosisExtraLight();
    Font getFontDosisMedium();
    Font getFontDosisRegular();
    Font getFontDosisVariable();
}