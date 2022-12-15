#include "Knob.h"

namespace gui
{
    // KNOB

    Knob::Knob(Utils& u, const String& _name, const String& _tooltip, CursorType _cursorType) :
        Comp(u, _tooltip, _cursorType),
        onEnter([](Knob&) {}),
        onExit([](Knob&) {}),
        onDown([](Knob&) {}),
        onWheel([](Knob&) {}),
        onResize([](Knob&) {}),
        onDoubleClick([](Knob&) {}),
        onDrag([](Knob&, PointF&, bool) {}),
        onUp([](Knob&, const Mouse&) {}),
        onTimer([](Knob&) { return false; }),
        onPaint([](Knob&, Graphics&) {}),
        getInfo([](int) { return ""; }),
        label(u, _name),
        dragXY(), lastPos(),
        knobBounds(),
        values(),
        comps(),
        states(),
        hidesCursor(true),
        locked(false),
		dragMode(DragMode::Vertical),
        activeCursor(_cursorType)
    {
        setInterceptsMouseClicks(true, true);

        setName(_name);

        label.textCID = ColourID::Txt;
        label.just = Just::centred;
        label.mode = Label::Mode::TextToLabelBounds;

        addAndMakeVisible(label);
    }

    Knob::~Knob()
    {
		
    }

    void Knob::init(std::vector<int>&& distX, std::vector<int>&& distY)
    {
        layout.init
        (
            distX,
            distY
        );
    }

    void Knob::timerCallback()
    {
        if (onTimer(*this))
            repaint();
    }

    void Knob::paint(juce::Graphics& g)
    {
        onPaint(*this, g);
    }

    void Knob::resized()
    {
        layout.resized();
        onResize(*this);
    }

    void Knob::mouseEnter(const Mouse& mouse)
    {
        Comp::mouseEnter(mouse);
        onEnter(*this);
    }

    void Knob::mouseExit(const Mouse&)
    {
        onExit(*this);
    }

    void Knob::mouseDown(const Mouse& mouse)
    {
        if (mouse.mods.isLeftButtonDown())
        {
            dragXY.setXY
            (
                mouse.position.x,
                mouse.position.y
            );
            lastPos = getPosition().toFloat();

            onDown(*this);
        }

        utils.giveDAWKeyboardFocus();
    }

    void Knob::mouseDrag(const Mouse& mouse)
    {
        if (mouse.mods.isLeftButtonDown())
        {
            if (hidesCursor)
                hideCursor();
			
            const auto nPosition = getPosition().toFloat();
            const auto posShift = nPosition - lastPos;
            lastPos = nPosition;

			dragXY -= posShift;
            auto dragOffset = mouse.position - dragXY;
            auto shiftDown = mouse.mods.isShiftDown();
            onDrag(*this, dragOffset, shiftDown);
            dragXY = mouse.position;
        }
    }

    void Knob::mouseUp(const Mouse& mouse)
    {
        onUp(*this, mouse);

        if(hidesCursor)
			if(mouse.mouseWasDraggedSinceMouseDown())
                showCursor(*this);
    }

    void Knob::mouseWheelMove(const Mouse& mouse, const MouseWheel& wheel)
    {
        if (mouse.mods.isAnyMouseButtonDown())
            return;

        const bool reversed = wheel.isReversed ? -1.f : 1.f;
        const bool isTrackPad = wheel.deltaY * wheel.deltaY < .0549316f;
        if (isTrackPad)
            dragXY.setXY
            (
                reversed * wheel.deltaX,
                reversed * wheel.deltaY
            );
        else
        {
            const auto deltaXPos = wheel.deltaX > 0.f ? 1.f : -1.f;
            const auto deltaYPos = wheel.deltaY > 0.f ? 1.f : -1.f;
            dragXY.setXY
            (
                reversed * WheelDefaultSpeed * deltaXPos,
                reversed * WheelDefaultSpeed * deltaYPos
            );
        }

        if (mouse.mods.isShiftDown())
            dragXY *= SensitiveDrag;

        onWheel(*this);
    }

    void Knob::mouseDoubleClick(const Mouse&)
    {
        onDoubleClick(*this);
    }

    void Knob::setLocked(bool lckd)
    {
        locked = lckd;
        if (locked)
        {
            setCursorType(CursorType::Inactive);
            setAlpha(.2f);
        }
        else
        {
            setCursorType(activeCursor);
            setAlpha(1.f);
        }
    }

	// KNOB LOOK AND FEEL FREE FUNCS

    namespace looks
    {
        enum { Value, MaxModDepth, ValMod, ModBias, Meter, NumValues };
        enum { ModDial, LockButton, NumComps };
		
        namespace def
        {
            static constexpr float AngleWidth = PiQuart * 3.f;
            static constexpr float AngleRange = AngleWidth * 2.f;

            static std::function<void(Knob&, Graphics&)> paint(bool modulatable, bool hasMeter)
            {
                return [modulatable, hasMeter](Knob& k, Graphics& g)
                {
                    const auto& vals = k.values;
                    const auto thicc = k.getUtils().thicc;
                    const auto thicc2 = thicc * 2.f;
                    const auto thicc3 = thicc * 3.f;
                    const auto thicc5 = thicc * 5.f;
                    Stroke strokeType(thicc, Stroke::JointStyle::curved, Stroke::EndCapStyle::butt);
                    const auto radius = k.knobBounds.getWidth() * .5f;
                    const auto radiusInner = radius * .8f;
                    const auto radDif = (radius - radiusInner) * .8f;

                    PointF centre
                    (
                        radius + k.knobBounds.getX(),
                        radius + k.knobBounds.getY()
                    );

                    auto col = Colours::c(ColourID::Txt);

                    if (hasMeter)
                    {
                        // METER
						auto valMeter = vals[Meter];
#if !JUCE_DEBUG
						if (std::isnan(valMeter))
#endif
                        if (valMeter != 0.f)
                        {
                            const auto radiusBetween = radiusInner + radDif;
                            const auto metr = vals[Meter] > 1.f ? 1.f : vals[Meter];
                            const auto meterAngle = AngleRange * metr;
                            
                            Path meterArc;
                            meterArc.addCentredArc
                            (
                                centre.x, centre.y,
                                radiusBetween, radiusBetween,
                                -AngleWidth,
                                0.f, meterAngle,
                                true
                            );

                            strokeType.setStrokeThickness(radDif);
                            g.setColour(Colours::c(ColourID::Mod));
                            g.strokePath(meterArc, strokeType);
                            strokeType.setStrokeThickness(thicc);
                        }
                    }

                    { // paint lines
                        
                        Path arcOutline;
                        arcOutline.addCentredArc
                        (
                            centre.x, centre.y,
                            radius, radius,
                            0.f,
                            -AngleWidth, AngleWidth,
                            true
                        );
                        g.setColour(col);
                        g.strokePath(arcOutline, strokeType);

                        Path arcInline;
                        arcInline.addCentredArc
                        (
                            centre.x, centre.y,
                            radiusInner, radiusInner,
                            0.f,
                            -AngleWidth, AngleWidth,
                            true
                        );
                        Stroke stroke2 = strokeType;
                        stroke2.setStrokeThickness(radDif);
                        g.strokePath(arcInline, stroke2);
                    };

                    const auto valNormAngle = vals[Value] * AngleRange;
                    const auto valAngle = -AngleWidth + valNormAngle;
                    const auto radiusExt = radius + thicc;

                    // paint modulation
                    if (modulatable)
                    {
                        const auto valModAngle = vals[ValMod] * AngleRange;
                        const auto modAngle = -AngleWidth + valModAngle;
                        const auto modTick = LineF::fromStartAndAngle(centre, radiusExt, modAngle);
                        const auto shortenedModTick = modTick.withShortenedStart(radiusInner - thicc);

                        g.setColour(Colours::c(ColourID::Bg));
                        g.drawLine(shortenedModTick, thicc * 4.f);

                        const auto maxModDepthAngle = juce::jlimit(-AngleWidth, AngleWidth, valNormAngle + vals[MaxModDepth] * AngleRange - AngleWidth);
                        const auto biasAngle = AngleRange * vals[ModBias] - AngleWidth;

                        g.setColour(Colours::c(ColourID::Bias));
                        {
                            Path biasPath;
                            biasPath.addCentredArc
                            (
                                centre.x, centre.y,
                                radiusInner, radiusInner,
                                0.f,
                                0.f, biasAngle,
                                true
                            );
                            Stroke bStroke = strokeType;
                            bStroke.setStrokeThickness(radDif);

                            g.strokePath(biasPath, bStroke);
                        }

                        g.setColour(Colours::c(ColourID::Mod));
                        g.drawLine(modTick.withShortenedStart(radiusInner), thicc2);
                        {
                            Path modPath;
                            modPath.addCentredArc
                            (
                                centre.x, centre.y,
                                radius, radius,
                                0.f,
                                maxModDepthAngle, valAngle,
                                true
                            );
                            g.strokePath(modPath, strokeType);
                        }
                    };

                    col = Colours::c(ColourID::Interact);
					
                    { // paint tick
                        const auto tickLine = LineF::fromStartAndAngle(centre, radius, valAngle);
                        const auto shortened = tickLine.withShortenedStart(radiusInner - thicc);
                        g.setColour(Colours::c(ColourID::Bg));
                        g.drawLine(shortened, thicc5);
                        g.setColour(col);
                        g.drawLine(shortened, thicc3);
                    }
                };
            }

            static void create(Knob& k, bool modulatable, bool hasMeter)
            {
                k.onPaint = paint(modulatable, hasMeter);

                k.onResize = [modulatable](Knob& k)
                {
                    const auto thicc = k.getUtils().thicc;
                    auto& layout = k.getLayout();

                    k.knobBounds = layout(0, 0, 3, 2, true).reduced(thicc);
                    layout.place(k.label, 0, 2, 3, 1, false);
                    if (modulatable)
                    {
                        layout.place(*k.comps[ModDial], 1, 1, 1, 1, true);
                        layout.place(*k.comps[LockButton], 1.5f, 1.5f, 1.5f, 1.5f, true);
                    }
                };

                k.init
                (
                    { 1, 1, 1 },
                    { 13, 5, 5 }
                );
            }
        }

        namespace slidr
        {
            static void create(Knob& k, bool modulatable, bool hasMeter, bool vertical)
            {
                k.onPaint = [modulatable, hasMeter, vertical](Knob& k, Graphics& g)
                {
                    enum { X, Y };
					// remember to implement hasMeter
                    const auto& bounds = k.knobBounds;
					const auto thicc = k.utils.thicc;
					const auto thicc2 = thicc * 2.f;
					const auto thicc4 = thicc * 4.f;
                    const float xy[2] = { bounds.getX(), bounds.getY() };
					const float wh[2] = { bounds.getWidth(), bounds.getHeight() };
					
					const float btm[2] = { xy[X] + wh[X], xy[Y] + wh[Y]};
                    const float centre[2] =
                    {
                        xy[X] + wh[X] * .5f,
                        xy[Y] + wh[Y] * .5f
                    };

                    const auto values = k.values.data();
                    const auto val = values[Value];
                    auto col = Colours::c(ColourID::Interact);
                    g.setColour(col);
                    
                    if (vertical)
                    {
                        const auto valLength = val * wh[Y];
                        const auto valXY = btm[Y] - valLength;

                        const auto line0X = centre[X] - thicc;
                        const auto line1X = centre[X] + thicc2;
                        const auto line0Y = xy[Y];
                        const auto line1Y = btm[Y];
                        { // paint lines

                            g.drawLine({ line0X, line0Y, line0X, line1Y }, thicc);
                            g.drawLine({ line1X, line0Y,  line1X, line1Y }, thicc2);
                        }

                        if (modulatable)
                        { // paint modulation
                            const auto maxModDepth = values[MaxModDepth];
                            const auto mmdLength = maxModDepth * wh[Y];
                            const auto mmdY = juce::jlimit(xy[Y], btm[Y], valXY - mmdLength);

                            col = Colours::c(ColourID::Mod);
                            g.setColour(col);
                            g.drawLine({ line0X, valXY, line0X, mmdY }, thicc);

                            const auto valMod = values[ValMod];
                            const auto valModHeight = valMod * wh[Y];
                            const auto valModY = juce::jlimit(xy[Y], btm[Y], btm[Y] - valModHeight);
                            g.drawLine({ line0X, valModY, line1X, valModY }, thicc2);

                            const auto bias = values[ModBias];
                            const auto biasHeight = bias * wh[Y];
                            const auto biasX = line1X;
                            const auto biasY0 = centre[Y];
                            const auto biasY1 = btm[Y] - biasHeight;
                            col = Colours::c(ColourID::Bias);
                            g.setColour(col);
                            g.drawLine({ biasX, biasY0, biasX, biasY1 }, thicc2);
                        }

                        { // paint tick
                            col = Colours::c(ColourID::Bg);
                            g.setColour(col);
                            g.drawLine({ line0X, valXY, line1X, valXY }, thicc4);
                            col = Colours::c(ColourID::Interact);
                            g.setColour(col);
                            g.drawLine({ line0X, valXY, line1X, valXY }, thicc2);
                        }
                    }
                    else
                    {
						const auto valLength = val * wh[X];
						const auto valXY = xy[X] + valLength;

						const auto line0X = xy[X];
						const auto line1X = btm[X];
						const auto line0Y = centre[Y] - thicc;
						const auto line1Y = centre[Y] + thicc2;
						{ // paint lines

							g.drawLine({ line0X, line0Y, line1X, line0Y }, thicc);
							g.drawLine({ line0X, line1Y, line1X, line1Y }, thicc2);
						}

						if (modulatable)
						{ // paint modulation
							const auto maxModDepth = values[MaxModDepth];
							const auto mmdLength = maxModDepth * wh[X];
							const auto mmdX = juce::jlimit(xy[X], btm[X], valXY + mmdLength);

							col = Colours::c(ColourID::Mod);
							g.setColour(col);
							g.drawLine({ valXY, line0Y, mmdX, line0Y }, thicc);

							const auto valMod = values[ValMod];
                            const auto valModLength = valMod * wh[X];
							const auto valModX = juce::jlimit(xy[X], btm[X], xy[X] + valModLength);
							g.drawLine({ valModX, line0Y, valModX, line1Y }, thicc2);

							const auto bias = values[ModBias];
							const auto biasLength = bias * wh[X];
							const auto biasX0 = centre[X];
							const auto biasX1 = xy[X] + biasLength;
							const auto biasY = line1Y;
							col = Colours::c(ColourID::Bias);
							g.setColour(col);
							g.drawLine({ biasX0, biasY, biasX1, biasY }, thicc2);
						}

                        { // paint tick
                            col = Colours::c(ColourID::Bg);
                            g.setColour(col);
                            g.drawLine({ valXY, line0Y, valXY, line1Y }, thicc4);
                            col = Colours::c(ColourID::Interact);
                            g.setColour(col);
                            g.drawLine({ valXY, line0Y, valXY, line1Y }, thicc);
                        }
                    }
                };

                k.onResize = [modulatable, vertical](Knob& k)
                {
                    const auto thicc = k.utils.thicc;
                    auto& layout = k.layout;

                    if (vertical)
                    {
                        k.knobBounds = layout(0, 1, 1, 1, false).reduced(thicc);
                        layout.place(k.label, 0, 3, 1, 1, false);
                        if (modulatable)
                        {
                            layout.place(*k.comps[LockButton], 0, 0, 1, 1, true);
                            layout.place(*k.comps[ModDial], 0, 2, 1, 1, true);
                        }
                    }
                    else
                    {
						k.knobBounds = layout(2, 0, 1, 1, false).reduced(thicc);
						layout.place(k.label, 0, 0, 1, 1, false);
						if (modulatable)
						{
							layout.place(*k.comps[LockButton], 3, 0, 1, 1, true);
							layout.place(*k.comps[ModDial], 1, 0, 1, 1, true);
						}
                    }  
                };

                if(vertical)
                    k.init
                    (
                        { 1 },
                        { 3, 13, 3, 3 }
                    );
                else
                    k.init
                    (
                        { 3, 3, 13, 3 },
                        { 1 }
                    );
            }
        }

        namespace knot
        {
            static std::function<void(Knob&, Graphics&)> paint()
            {
                return [](Knob& k, Graphics& g)
                {
                    const auto thicc = k.getUtils().thicc;
                    const Stroke strokeType(thicc, Stroke::JointStyle::curved, Stroke::EndCapStyle::butt);
                    const auto radius = k.knobBounds.getWidth() * .5f;
                    
                    const PointF centre
                    (
                        radius + k.knobBounds.getX(),
                        radius + k.knobBounds.getY()
                    );

                    auto col = Colours::c(ColourID::Bg);

                    g.fillEllipse(k.knobBounds);
                    
                    
                    col = Colours::c(ColourID::Interact);
                    g.setColour(col);

                    const auto startAngle = PiHalf;

                    Path arcs;
                    for (auto i = 0.f; i < 3.f; ++i)
                    {
                        const auto rad = radius - thicc * i;
                        const auto angle = startAngle + i * Pi;

                        arcs.addCentredArc
                        (
							centre.x,
                            centre.y,
							rad,
                            rad,
                            angle,
							0.f,
							Pi,
							true
                        );
                    }
					
                    g.strokePath(arcs, strokeType);
                };
            }

            static void create(Knob& k)
            {
                k.onPaint = paint();

                k.onResize = [](Knob& k)
                {
                    k.knobBounds = k.getLocalBounds().toFloat().reduced(k.utils.thicc);
                };
            }
        }
    }

    bool isKnobLooksTypeModulatable(Knob::LooksType lt) noexcept
    {
		return lt == Knob::LooksType::Default
			|| lt == Knob::LooksType::VerticalSlider
			|| lt == Knob::LooksType::HorizontalSlider;
    }

    // PARAMETER CREATION FREE FUNCS

    void makePseudoParameter(Knob& knob, const String& name, String&& tooltip, std::atomic<float>* param, Knob::LooksType looksType)
    {
        knob.setName(name);
        knob.setTooltip(std::move(tooltip));
        knob.label.setText(name);
		
        switch (looksType)
        {
        case Knob::LooksType::VerticalSlider:
            looks::slidr::create(knob, false, false, true);
            break;
		case Knob::LooksType::HorizontalSlider:
			looks::slidr::create(knob, false, false, false);
			knob.dragMode = Knob::DragMode::Horizontal;
            break;
        default:
            looks::def::create(knob, false, false);
            break;
        }

        knob.onEnter = [param](Knob& k)
        {
            k.label.setText(String(param->load(), 2));
            k.label.repaint();
        };

        knob.onExit = [](Knob& k)
        {
            k.label.setText(k.getName());
            k.label.repaint();
        };

        knob.onDown = [param](Knob& k)
        {
            k.label.setText(String(param->load(), 2));
            k.label.repaint();
        };

        knob.onDrag = [param](Knob& k, PointF& dragOffset, bool shiftDown)
        {
            if (shiftDown)
                dragOffset *= SensitiveDrag;

            const auto speed = 1.f / k.getUtils().getDragSpeed();

            const auto dragVal = k.dragMode == Knob::DragMode::Vertical ? -dragOffset.y : dragOffset.x;
            const auto newValue = juce::jlimit(0.f, 1.f, param->load() + dragVal * speed);
            param->store(newValue);
            k.values[0] = newValue;

            k.label.setText(String(newValue, 2));
            repaintWithChildren(&k);
        };

        knob.onUp = [param](Knob& k, const Mouse& mouse)
        {
            if (mouse.mods.isLeftButtonDown())
            {
                if (!mouse.mouseWasDraggedSinceMouseDown())
                {
                    if (mouse.mods.isAltDown())
                        param->store(.25f);
                }
            }
            else if (mouse.mods.isRightButtonDown())
                if (!mouse.mouseWasDraggedSinceMouseDown())
                    if (mouse.mods.isAltDown())
                        param->store(.25f);

            const auto newValue = param->load();
            k.values[0] = newValue;
            k.label.setText(String(newValue, 2));
            repaintWithChildren(&k);
        };

        knob.onWheel = [param](Knob& k)
        {
            const auto newValue = juce::jlimit(0.f, 1.f, param->load() + k.dragXY.y);
            param->store(newValue);
            k.values[0] = newValue;

            k.label.setText(String(newValue, 2));
            repaintWithChildren(&k);
        };

        knob.onDoubleClick = [param](Knob& k)
        {
            const auto dVal = .25f;
            param->store(dVal);
            k.values[0] = dVal;

            k.label.setText(String(dVal, 2));
            repaintWithChildren(&k);
        };

        knob.values.reserve(1);
        auto& values = knob.values;

        values.emplace_back(param->load());
    }

    void makeParameter(Knob& knob, PID pID, const String& name, bool modulatable, const std::atomic<float>* meter, Knob::LooksType looksType)
    {
        std::vector<PID> pIDs;
        pIDs.push_back(pID);

		makeParameter(knob, pIDs, name, modulatable, meter, looksType);
    }

    void makeParameter(Knob& knob, const std::vector<PID>& pIDs, const String& name, bool modulatable, const std::atomic<float>* meter, Knob::LooksType looksType)
    {
		bool hasMeter = meter != nullptr;

        switch (looksType)
        {
        case Knob::LooksType::VerticalSlider:
            looks::slidr::create(knob, modulatable, hasMeter, true);
            break;
        case Knob::LooksType::HorizontalSlider:
            looks::slidr::create(knob, modulatable, hasMeter, false);
            knob.dragMode = Knob::DragMode::Horizontal;
            break;
        case Knob::LooksType::Knot:
            looks::knot::create(knob);
            break;
        default:
            looks::def::create(knob, modulatable, hasMeter);
            break;
        }

        knob.getInfo = [pIDs](int i)
        {
            if (i == 0)
            {
                String idsStr;
                for (auto pid : pIDs)
                    idsStr += toString(pid) + "\n";
                return idsStr;
            }

            return String();
        };

        auto mainPID = pIDs[0];

        knob.setName(name);
        knob.label.setText(name);
        knob.setTooltip(param::toTooltip(mainPID));

        auto& utils = knob.getUtils();
        auto mainParam = utils.getParam(mainPID);

        knob.setLocked(mainParam->isLocked());

        knob.onEnter = [mainParam](Knob& k)
        {
            k.label.setText(mainParam->getCurrentValueAsText());
            k.label.repaint();
        };

        knob.onExit = [](Knob& k)
        {
            k.label.setText(k.getName());
            k.label.repaint();
        };

        knob.onDown = [pIDs, mainParam](Knob& k)
        {
            for (auto& pID : pIDs)
            {
                auto param = k.getUtils().getParam(pID);
                if (!param->isInGesture())
                    param->beginGesture();
            }

            k.label.setText(mainParam->getCurrentValueAsText());
            k.label.repaint();
        };

        knob.onDrag = [pIDs, mainParam](Knob& k, PointF& dragOffset, bool shiftDown)
        {
            if (shiftDown)
                dragOffset *= SensitiveDrag;

            const auto speed = 1.f / k.getUtils().getDragSpeed();

            for (const auto pID : pIDs)
            {
                auto param = k.getUtils().getParam(pID);
				const auto dragVal = k.dragMode == Knob::DragMode::Vertical ? -dragOffset.y : dragOffset.x;
                const auto newValue = juce::jlimit(0.f, 1.f, param->getValue() + dragVal * speed);
                param->setValueNotifyingHost(newValue);
            }

            k.label.setText(mainParam->getCurrentValueAsText());
            k.label.repaint();

            k.notify(EvtType::ParametrDragged, &k);
        };

        knob.onUp = [pIDs, mainParam](Knob& k, const Mouse& mouse)
        {
            if (mouse.mods.isLeftButtonDown())
            {
                if (!mouse.mouseWasDraggedSinceMouseDown())
                {
                    if (mouse.mods.isAltDown())
                        for (auto& pID : pIDs)
                        {
                            auto param = k.getUtils().getParam(pID);
                            param->setValueNotifyingHost(param->getDefaultValue());
                        }
                    else if (mouse.mods.isCtrlDown())
                    {
                        k.utils.getEventSystem().notify(EvtType::EnterParametrValue, &k);
                    }

                }
                for (auto& pID : pIDs)
                {
                    auto param = k.getUtils().getParam(pID);
                    param->endGesture();
                }
            }
            else if (mouse.mods.isRightButtonDown())
                if (!mouse.mouseWasDraggedSinceMouseDown())
                    if (mouse.mods.isAltDown())
                        for (auto& pID : pIDs)
                        {
                            auto param = k.getUtils().getParam(pID);
                            param->setValueWithGesture(param->getDefaultValue());
                        }
                    else
                        k.notify(EvtType::ParametrRightClicked, &k);

            k.label.setText(mainParam->getCurrentValueAsText());
            k.label.repaint();
        };

        knob.onWheel = [pIDs, mainParam](Knob& k)
        {
            for (auto& pID : pIDs)
            {
                auto param = k.getUtils().getParam(pID);
                const auto& range = param->range;
                const auto interval = range.interval;
                if (interval > 0.f)
                {
                    const auto nStep = interval / range.getRange().getLength();
                    k.dragXY.setY(k.dragXY.y > 0.f ? nStep : -nStep);
                    auto newValue = juce::jlimit(0.f, 1.f, param->getValue() + k.dragXY.y);
                    newValue = range.convertTo0to1(range.snapToLegalValue(range.convertFrom0to1(newValue)));
                    param->setValueWithGesture(newValue);
                }
                else
                {
                    const auto newValue = juce::jlimit(0.f, 1.f, param->getValue() + k.dragXY.y);
                    param->setValueWithGesture(newValue);
                }
            }

            k.label.setText(mainParam->getCurrentValueAsText());
            k.label.repaint();
        };

        knob.onDoubleClick = [pIDs, mainParam](Knob& k)
        {
            for (auto& pID : pIDs)
            {
                auto param = k.getUtils().getParam(pID);
                if (param->isInGesture())
                    return;

                const auto dVal = param->getDefaultValue();
                param->setValueWithGesture(dVal);
            }

            k.label.setText(mainParam->getCurrentValueAsText());
            k.label.repaint();

            k.notify(EvtType::ParametrDragged, &k);
        };

        knob.values.reserve(looks::NumValues);
        auto& values = knob.values;

        values.emplace_back(mainParam->getValue());
        values.emplace_back(mainParam->getMaxModDepth());
        values.emplace_back(mainParam->getValMod());
        values.emplace_back(mainParam->getModBias());
        values.emplace_back(0.f);

        knob.onTimer = [mainParam, meter](Knob& k)
        {
			bool hasMeter = meter != nullptr;
            bool shallRepaint = false;

            const auto lckd = mainParam->isLocked();
            if (k.locked != lckd)
                k.setLocked(lckd);

            const auto vn = mainParam->getValue();
            const auto mmd = mainParam->getMaxModDepth();
            const auto vm = mainParam->getValMod();
            const auto mb = mainParam->getModBias();

            auto& vals = k.values;

            if (vals[looks::Value] != vn || vals[looks::MaxModDepth] != mmd || vals[looks::ValMod] != vm || vals[looks::ModBias] != mb)
            {
                vals[looks::Value] = vn;
                vals[looks::MaxModDepth] = mmd;
                vals[looks::ValMod] = vm;
                vals[looks::ModBias] = mb;
                shallRepaint = true;
            }

            if (hasMeter)
            {
                const auto metr = std::floor(meter->load() * 128.f) * .0078125f;
                if (vals[looks::Meter] != metr)
                {
                    vals[looks::Meter] = metr;
                    shallRepaint = true;
                }
            }

            return shallRepaint;
        };

        knob.comps.reserve(looks::NumComps);

        modulatable = modulatable && isKnobLooksTypeModulatable(looksType);
		
        if (modulatable)
        {
            auto modDial = std::make_unique<Knob>(knob.getUtils(), "M", "Drag this to modulate the macro's modulation depth.", CursorType::Mod);
            {
                auto& dial = *modDial;

                auto& label = dial.label;
                label.mode = Label::Mode::TextToLabelBounds;
                label.textCID = ColourID::Bg;

                enum { StateMaxModDepth, StateModBias, NumStates };
                dial.states.push_back(StateMaxModDepth);

                dial.onResize = [](Knob& k)
                {
                    k.knobBounds = k.getLocalBounds().toFloat();
                    const auto thicc = k.utils.thicc * .5f;
                    k.label.setBounds(k.knobBounds.reduced(thicc).toNearestInt());
                };

                dial.onPaint = [](Knob& k, Graphics& g)
                {
                    auto state = k.states[0];

                    Colour col;
                    switch (state)
                    {
                    case StateMaxModDepth:
                        col = Colours::c(ColourID::Mod);
                        break;
                    case StateModBias:
                        col = Colours::c(ColourID::Bias);
                        break;
                    }
                    g.setColour(col);
                    g.fillEllipse(k.knobBounds);
                };

                dial.onDrag = [pIDs](Knob& k, PointF& dragOffset, bool shiftDown)
                {
                    if (shiftDown)
                        dragOffset *= SensitiveDrag;

                    auto state = k.states[0];
                    auto& utils = k.getUtils();
                    if (state == StateModBias)
                        dragOffset *= .5f;
                    const auto speed = 1.f / utils.getDragSpeed();
                    dragOffset *= speed;

                    for (auto pID : pIDs)
                    {
                        auto param = utils.getParam(pID);
                        float newValue;
                        switch (state)
                        {
                        case StateMaxModDepth:
                            newValue = param->getMaxModDepth() - dragOffset.y;
                            param->setMaxModDepth(newValue);
                            break;
                        case StateModBias:
                            newValue = param->getModBias() - dragOffset.y;
                            param->setModBias(newValue);
                            break;
                        }
                    }

                };

                dial.onUp = [pIDs](Knob& k, const Mouse& mouse)
                {
                    if (!mouse.mouseWasDraggedSinceMouseDown())
                    {
                        if (mouse.mods.isCtrlDown())
                            for (auto pID : pIDs)
                            {
                                auto param = k.getUtils().getParam(pID);
                                param->setMaxModDepth(0.f);
                                param->setModBias(.5f);
                            }
                        else if (mouse.mods.isRightButtonDown())
                        {
                            auto& state = k.states[0];
                            state = (state + 1) % NumStates;

                            switch (state)
                            {
                            case StateMaxModDepth:
                                k.label.setText("M");
                                k.setCursorType(CursorType::Mod);
                                k.activeCursor = CursorType::Mod;
                                break;
                            case StateModBias:
                                k.label.setText("B");
                                k.setCursorType(CursorType::Bias);
                                k.activeCursor = CursorType::Bias;
                                break;
                            }

                            repaintWithChildren(&k);
                        }
                    }
                };

                dial.onDoubleClick = [pIDs](Knob& k)
                {
                    for (auto pID : pIDs)
                    {
                        auto param = k.getUtils().getParam(pID);
                        param->setMaxModDepth(0.f);
                        param->setModBias(.5f);
                    }
                };

                knob.comps.emplace_back(std::move(modDial));
                knob.addAndMakeVisible(*knob.comps.back());
            }

            { // lockable
                auto lockButton = std::make_unique<Button>(knob.getUtils(), "Un-/Lock this parameter.");
                {
                    auto& lck = *lockButton;

                    makeSymbolButton(lck, ButtonSymbol::Lock, 1);

                    lck.onClick.push_back([pIDs](Button& btn, const Mouse&)
                    {
                        for (auto pID : pIDs)
                        {
                            auto param = btn.utils.getParam(pID);
							param->setLocked(!param->isLocked());
                        }

                        btn.toggleState = btn.utils.getParam(pIDs[0])->isLocked() ? 1 : 0;
                    });

                    knob.comps.emplace_back(std::move(lockButton));
                    knob.addAndMakeVisible(*knob.comps.back());
                }
            }
        }

        knob.startTimerHz(PPDFPSKnobs);
    }

    void makeParameter(Knob& knob, PID pIDHorizontal, PID pIDVertical)
    {
		makeParameter(knob, std::vector<PID>{ pIDHorizontal }, std::vector<PID>{ pIDVertical });
    }

    void makeParameter(Knob& knob, const std::vector<PID>& pIDHorizontal, PID pIDVertical)
    {
        makeParameter(knob, pIDHorizontal, std::vector<PID>{ pIDVertical });
    }

    void makeParameter(Knob& knob, const std::vector<PID>& pIDsHorizontal, const std::vector<PID>& pIDsVertical)
    {
        looks::knot::create(knob);

        knob.getInfo = [pIDsHorizontal](int i)
        {
            if (i == 0)
            {
                String idsStr;
                for (auto pid : pIDsHorizontal)
                    idsStr += toString(pid) + "\n";
                return idsStr;
            }

            return String();
        };

        auto mainPID = pIDsHorizontal[0];

        knob.setTooltip(param::toTooltip(mainPID));

        auto& utils = knob.getUtils();
        auto mainParam = utils.getParam(mainPID);

        knob.setLocked(mainParam->isLocked());

        knob.onEnter = [mainParam](Knob& k)
        {
            k.label.setText(mainParam->getCurrentValueAsText());
            k.label.repaint();
        };

        knob.onExit = [](Knob& k)
        {
            k.label.setText(k.getName());
            k.label.repaint();
        };

        knob.onDown = [pIDsHorizontal, pIDsVertical, mainParam](Knob& k)
        {
            for (auto pID : pIDsHorizontal)
            {
                auto param = k.getUtils().getParam(pID);
                if (!param->isInGesture())
                    param->beginGesture();
            }
            for (auto pID : pIDsVertical)
            {
				auto param = k.getUtils().getParam(pID);
				if (!param->isInGesture())
					param->beginGesture();
            }

            k.label.setText(mainParam->getCurrentValueAsText());
            k.label.repaint();
        };

        knob.onDrag = [pIDsHorizontal, pIDsVertical, mainParam](Knob& k, PointF& dragOffset, bool shiftDown)
        {
            if (shiftDown)
                dragOffset *= SensitiveDrag;

            const auto speed = 1.f / k.getUtils().getDragSpeed();

            for (const auto pID : pIDsHorizontal)
            {
                auto param = k.getUtils().getParam(pID);
                const auto dragVal = dragOffset.x;
                const auto newValue = juce::jlimit(0.f, 1.f, param->getValue() + dragVal * speed);
                param->setValueNotifyingHost(newValue);
            }
            for (const auto pID : pIDsVertical)
            {
                auto param = k.getUtils().getParam(pID);
                const auto dragVal = -dragOffset.y;
                const auto newValue = juce::jlimit(0.f, 1.f, param->getValue() + dragVal * speed);
                param->setValueNotifyingHost(newValue);
            }

            k.label.setText(mainParam->getCurrentValueAsText());
            k.label.repaint();

            k.notify(EvtType::ParametrDragged, &k);
        };

        knob.onUp = [pIDsHorizontal, pIDsVertical, mainParam](Knob& k, const Mouse& mouse)
        {
            if (mouse.mods.isLeftButtonDown())
            {
                if (!mouse.mouseWasDraggedSinceMouseDown())
                {
                    if (mouse.mods.isAltDown())
                    {
                        for (auto& pID : pIDsHorizontal)
                        {
                            auto param = k.getUtils().getParam(pID);
                            param->setValueNotifyingHost(param->getDefaultValue());
                        }
						for (auto& pID : pIDsVertical)
						{
							auto param = k.getUtils().getParam(pID);
							param->setValueNotifyingHost(param->getDefaultValue());
						}
                    }
                    else if (mouse.mods.isCtrlDown())
                    {
                        k.utils.getEventSystem().notify(EvtType::EnterParametrValue, &k);
                    }
                }
                
                for (auto pID : pIDsHorizontal)
                {
                    auto param = k.getUtils().getParam(pID);
                    param->endGesture();
                }
				for (auto pID : pIDsVertical)
				{
					auto param = k.getUtils().getParam(pID);
					param->endGesture();
				}
            }
            else if (mouse.mods.isRightButtonDown())
                if (!mouse.mouseWasDraggedSinceMouseDown())
                    if (mouse.mods.isAltDown())
                    {
                        for (auto pID : pIDsHorizontal)
                        {
                            auto param = k.getUtils().getParam(pID);
                            param->setValueWithGesture(param->getDefaultValue());
                        }
                        for (auto pID : pIDsVertical)
                        {
							auto param = k.getUtils().getParam(pID);
							param->setValueWithGesture(param->getDefaultValue());
						}
                    }
                        
                    else
                        k.notify(EvtType::ParametrRightClicked, &k);

            k.label.setText(mainParam->getCurrentValueAsText());
            k.label.repaint();
        };

        knob.onWheel = [pIDsVertical, mainParam](Knob& k)
        {
            for (auto pID : pIDsVertical)
            {
                auto param = k.getUtils().getParam(pID);
                const auto& range = param->range;
                const auto interval = range.interval;
                if (interval > 0.f)
                {
                    const auto nStep = interval / range.getRange().getLength();
                    k.dragXY.setY(k.dragXY.y > 0.f ? nStep : -nStep);
                    auto newValue = juce::jlimit(0.f, 1.f, param->getValue() + k.dragXY.y);
                    newValue = range.convertTo0to1(range.snapToLegalValue(range.convertFrom0to1(newValue)));
                    param->setValueWithGesture(newValue);
                }
                else
                {
                    const auto newValue = juce::jlimit(0.f, 1.f, param->getValue() + k.dragXY.y);
                    param->setValueWithGesture(newValue);
                }
            }

            k.label.setText(mainParam->getCurrentValueAsText());
            k.label.repaint();
        };

        knob.onDoubleClick = [pIDsHorizontal, pIDsVertical, mainParam](Knob& k)
        {
            for (auto pID : pIDsHorizontal)
            {
                auto param = k.getUtils().getParam(pID);
                if (param->isInGesture())
                    return;

                const auto dVal = param->getDefaultValue();
                param->setValueWithGesture(dVal);
            }
			for (auto pID : pIDsVertical)
			{
				auto param = k.getUtils().getParam(pID);
                if (param->isInGesture())
                    return;

                const auto dVal = param->getDefaultValue();
                param->setValueWithGesture(dVal);
            }

            k.label.setText(mainParam->getCurrentValueAsText());
            k.label.repaint();

            k.notify(EvtType::ParametrDragged, &k);
        };

        knob.values.reserve(looks::NumValues);
        auto& values = knob.values;

        values.emplace_back(mainParam->getValue());

        knob.onTimer = [mainParam](Knob& k)
        {
            bool shallRepaint = false;

            const auto lckd = mainParam->isLocked();
            if (k.locked != lckd)
                k.setLocked(lckd);

            const auto vn = mainParam->getValue();
            auto& vals = k.values;

            if (vals[looks::Value] != vn)
            {
                vals[looks::Value] = vn;
                shallRepaint = true;
            }

            return shallRepaint;
        };

        knob.comps.reserve(looks::NumComps);

        knob.startTimerHz(PPDFPSKnobs);
    }
	
    // CONTEXT MENU

    Notify ContextMenuKnobs::makeNotify2(ContextMenuKnobs& popUp)
    {
        return [&pop = popUp](EvtType type, const void* stuff)
        {
            if (type == EvtType::ParametrRightClicked)
            {
                const auto& knob = *static_cast<const Knob*>(stuff);
                
                const auto pIDsStr = knob.getInfo(0);
                std::vector<PID> pIDs;
                param::toPIDs(pIDs, pIDsStr, "\n");

                pop.setButton([pIDs](Button& btn, const Mouse&)
                {
                    Random rand;
                    for(auto pID: pIDs)
                        btn.getUtils().getParam(pID)->setValueWithGesture(rand.nextFloat());
                }, 0);
                pop.setButton([pIDs](Button& btn, const Mouse&)
                {
                    juce::Random rand;
                    for (auto pID : pIDs)
                    {
						auto param = btn.getUtils().getParam(pID);
                        auto val = param->getValue();
                        val += .05f * (rand.nextFloat() - .5f);
                        param->setValueWithGesture(juce::jlimit(0.f, 1.f, val));
                    }
                }, 1);
                pop.setButton([pIDs](Button& btn, const Mouse&)
                {
                    for (auto pID : pIDs)
                    {
						auto param = btn.getUtils().getParam(pID);
                        const auto val = param->getDefaultValue();
                        param->setValueWithGesture(val);
                    }
                }, 2);
                pop.setButton([pIDs](Button& btn, const Mouse&)
                {
                    for (auto pID : pIDs)
                    {
                        auto param = btn.getUtils().getParam(pID);
                        param->setDefaultValue(param->getValue());
                    }
                }, 3);
                pop.setButton([pIDs](Button& btn, const Mouse&)
                {
                    for (auto pID : pIDs)
                        btn.getUtils().getParam(pID)->switchLock();
                }, 4);
                pop.setButton([pIDs](Button& btn, const Mouse&)
                {
                    for (auto pID : pIDs)
                        btn.getUtils().assignMIDILearn(pID);
                }, 5);
                pop.setButton([pIDs](Button& btn, const Mouse&)
                {
                    for (auto pID : pIDs)
                        btn.getUtils().removeMIDILearn(pID);
                }, 6);
                pop.setButton([&k = knob](Button& btn, const Mouse&)
                {
                    btn.getUtils().getEventSystem().notify(EvtType::EnterParametrValue, &k);
                }, 7);

                pop.place(&knob);
            }
        };
    }

    ContextMenuKnobs::ContextMenuKnobs(Utils& u) :
        ContextMenu(u)
    {
        evts.push_back({ utils.getEventSystem(), makeNotify2(*this) });

        buttons.reserve(7);
        addButton("Randomize", "Randomize this parameter value.");
        addButton("Rand Relative", "Randomize this parameter value relative to its current value.");
        addButton("Load Default", "Resets this parameter value to its default value.");
        addButton("Save Default", "Saves this parameter value as its default one.");
        addButton("Lock / Unlock", "Parameter values are locked into place, even when changing presets.");
        addButton("MIDI Learn", "Click here to assign this parameter to a hardware control.");
        addButton("MIDI Unlearn", "Click here to remove this parameter from its hardware control(s).");
        addButton("Enter Value", "Click here to enter a parameter value with your keyboard");

        init();
    }

    // TEXT EDITOR KNOB

    Notify TextEditorKnobs::makeNotify(TextEditorKnobs& tek)
    {
        return [&editor = tek](EvtType type, const void* stuff)
        {
            if (type == EvtType::ClickedEmpty)
            {
                editor.disable();
                editor.setVisible(false);
            }
            if (type == EvtType::EnterParametrValue)
            {
                editor.disable();
                editor.txt.clear();

                const auto& knob = *static_cast<const Knob*>(stuff);
                const auto pIDsStr = knob.getInfo(0);
                std::vector<PID> pIDs;
                param::toPIDs(pIDs, pIDsStr, "\n");
                auto& utils = editor.getUtils();

                editor.onEscape = [&tek = editor]()
                {
                    tek.disable();
                    return true;
                };

                editor.onReturn = [&tek = editor, pIDs]()
                {
                    if (tek.txt.isNotEmpty())
                    {
                        for (auto pID : pIDs)
                        {
							auto param = tek.getUtils().getParam(pID);
                            auto& range = param->range;

                            const auto valDenorm = param->getValForTextDenorm(tek.txt);
							const auto val = range.convertTo0to1(range.snapToLegalValue(juce::jlimit(range.start, range.end, valDenorm)));
                            if (valDenorm >= range.start && valDenorm <= range.end)
                            {
								param->setValueWithGesture(val);
                            }
                            else
                            {
                                tek.setText(param->getText(val, 0));
                                tek.enable();
                                tek.repaint();
                            }
                        }
                    }
                    tek.disable();
                    return true;
                };

                const auto mouse = juce::Desktop::getInstance().getMainMouseSource();
                const auto screenPos = utils.getScreenPosition();
                const auto parametrScreenPos = knob.getScreenPosition();
                const auto parametrPos = parametrScreenPos - screenPos;
                const Point parametrCentre
                (
                    knob.getWidth() / 2,
                    knob.getHeight() / 2
                );
                editor.setCentrePosition(parametrPos + parametrCentre);
                editor.enable();
            }
        };
    }

    TextEditorKnobs::TextEditorKnobs(Utils& u) :
        TextEditor(u, "Enter a value for this parameter.", makeNotify(*this))
    {
        multiLine = false;
    }

    void TextEditorKnobs::paint(Graphics& g)
    {
        const auto thicc = utils.thicc;
        g.setColour(Colours::c(ColourID::Darken));
        g.fillRoundedRectangle(getLocalBounds().toFloat(), thicc);

        TextEditor::paint(g);
    }
	
}

