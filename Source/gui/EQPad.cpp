#include "EQPad.h"

namespace gui
{
	// EQPad::Node

	EQPad::Node::Node(Utils& u, PID xPID, PID yPID, PID scrollPID, PID rightClickPID, const std::vector<PID>& _morePIDs) :
		xyParam{ u.getParam(xPID), u.getParam(yPID) },
		scrollParam(u.getParam(scrollPID)),
		rightClickParam(u.getParam(rightClickPID)),
		morePIDs(_morePIDs),
		bounds(0.f, 0.f, 0.f, 0.f),
		x(getValue(X)),
		y(1.f - getValue(Y)),
		utils(u)
	{
	}

	void EQPad::Node::paint(Graphics& g) const
	{
		const auto thicc = utils.thicc;

		g.setColour(Colours::c(ColourID::Hover));
		g.fillEllipse(bounds);

		if (rightClickParam->getValMod() > .5f)
			g.setColour(Colours::c(ColourID::Interact));
		g.drawEllipse(bounds, thicc);
	}

	bool EQPad::Node::valueChanged() noexcept
	{
		const auto _x = getValue(X);
		const auto _y = 1.f - getValue(Y);

		if (x != _x || y != _y)
		{
			x = _x;
			y = _y;
			return true;
		}

		return false;
	}

	float EQPad::Node::getValue(Dimension d) const noexcept
	{
		return xyParam[d]->getValue();
	}

	void EQPad::Node::beginGesture() const
	{
		for (auto xy = 0; xy < NumDimensions; ++xy)
			if (!xyParam[xy]->isInGesture())
				xyParam[xy]->beginGesture();
	}

	void EQPad::Node::onDrag(const float* dof, float speed)
	{
		for (auto xy = 0; xy < NumDimensions; ++xy)
		{
			auto param = xyParam[xy];

			const auto pol = -1.f + static_cast<float>(xy * 2);
			const auto newValue = juce::jlimit(0.f, 1.f, param->getValue() - dof[xy] * speed * pol);
			param->setValueNotifyingHost(newValue);
		}
	}

	void EQPad::Node::endGesture(bool wasDragged, bool altDown)
	{
		if (!wasDragged)
			if (altDown)
				for (auto xy = 0; xy < NumDimensions; ++xy)
				{
					auto& param = *xyParam[xy];
					param.setValueNotifyingHost(param.getDefaultValue());
				}


		for (auto xy = 0; xy < NumDimensions; ++xy)
			xyParam[xy]->endGesture();
	}

	void EQPad::Node::onScroll(float wheelDeltaY, bool isReversed, bool shiftDown)
	{
		const bool reversed = isReversed ? -1.f : 1.f;
		const bool isTrackPad = wheelDeltaY * wheelDeltaY < .0549316f;
		float dragY;
		if (isTrackPad)
			dragY = reversed * wheelDeltaY;
		else
		{
			const auto deltaYPos = wheelDeltaY > 0.f ? 1.f : -1.f;
			dragY = reversed * WheelDefaultSpeed * deltaYPos;
		}

		if (shiftDown)
			dragY *= SensitiveDrag;

		auto param = scrollParam;
		const auto interval = param->range.interval;
		if (interval > 0.f)
		{
			const auto nStep = interval / param->range.getRange().getLength();
			dragY = dragY > 0.f ? nStep : -nStep;
		}

		const auto newValue = juce::jlimit(0.f, 1.f, param->getValue() + dragY);
		param->setValueWithGesture(newValue);
	}

	void EQPad::Node::onRightClick()
	{
		rightClickParam->setValueWithGesture(rightClickParam->getValue() > .5f ? 0.f : 1.f);
	}

	// EQPad

	EQPad::EQPad(Utils& u, String&& _tooltip) :
		Comp(u, _tooltip, CursorType::Interact),
		bounds(),
		onSelectionChanged(),

		nodes(),
		selected(),
		dragXY(),
		selectionLine(),
		selectionBounds(),
		tool(Tool::Select),
		hovered(nullptr),
		hitBoxLength(0.f)
	{
		setInterceptsMouseClicks(true, true);
		startTimerHz(PPDFPSKnobs);
	}

	void EQPad::addNode(PID xParam, PID yParam, PID scrollParam, PID rightClickParam, const std::vector<PID>& morePIDs)
	{
		nodes.push_back
		({
			utils,
			xParam,
			yParam,
			scrollParam,
			rightClickParam,
			morePIDs
			});
	}

	void EQPad::selectNode(int idx)
	{
		if (idx < 0 || idx >= nodes.size())
			return;

		selected.clear();
		selected.push_back(&nodes[idx]);
		selectionChanged();
	}

	void EQPad::selectAll()
	{
		selected.clear();
		for (auto& node : nodes)
			selected.push_back(&node);
		selectionChanged();
	}

	void EQPad::paint(Graphics& g)
	{
		const auto thicc = utils.thicc;

		paintHovered(g);

		paintSelectionBounds(g, thicc);

		g.setColour(Colours::c(ColourID::Hover));
		g.drawRoundedRectangle(bounds, thicc, thicc);

		for (const auto& node : nodes)
			node.paint(g);

		paintHighlightSelected(g, thicc);
	}

	void EQPad::paintHovered(Graphics& g)
	{
		if (hovered != nullptr)
		{
			g.setColour(Colours::c(ColourID::Hover));
			g.fillEllipse(hovered->bounds);
			PointF hCentre
			(
				hovered->bounds.getX() + hovered->bounds.getWidth() * .5f,
				hovered->bounds.getY() + hovered->bounds.getHeight() * .5f
			);
			const auto infoSize = getNodeSize() * 7.f;
			const auto infoSizeHalf = infoSize * .5f;
			auto infoX = hCentre.x - infoSizeHalf;
			if (infoX < 0.f)
				infoX += infoSizeHalf;
			else if (infoX + infoSize > bounds.getWidth())
				infoX -= infoSizeHalf;
			auto infoY = hCentre.y - infoSizeHalf;
			if (infoY < 0.f)
				infoY += infoSizeHalf;
			BoundsF infoBounds
			(
				infoX,
				infoY,
				infoSize,
				infoSizeHalf
			);
			g.setColour(Colours::c(ColourID::Txt));
			g.drawFittedText(hovered->xyParam[X]->getCurrentValueAsText(), infoBounds.toNearestInt(), Just::centred, 1);
		}
	}

	void EQPad::paintSelectionBounds(Graphics& g, float thicc)
	{
		if (!selectionBounds.isEmpty())
		{
			g.setColour(Colours::c(ColourID::Hover));
			auto dBounds = denormalize(selectionBounds);
			g.drawRoundedRectangle(dBounds.getIntersection(bounds), thicc, thicc);
		}
	}

	void EQPad::paintHighlightSelected(Graphics& g, float thicc)
	{
		if (!selected.empty())
		{
			Stroke stroke(thicc, Stroke::PathStrokeType::JointStyle::curved, Stroke::PathStrokeType::EndCapStyle::butt);

			for (const auto& sel : selected)
			{
				const auto& node = *sel;

				g.setColour(Colours::c(ColourID::Interact));
				drawRectEdges(g, node.bounds, thicc, stroke);
			}
		}
	}

	void EQPad::resized()
	{
		auto nodeSize = getNodeSize();

		bounds = getLocalBounds().toFloat().reduced(nodeSize * .5f);

		for (auto i = 0; i < numNodes(); ++i)
			moveNode(i);

		const auto minDimen = std::min(bounds.getWidth(), bounds.getHeight());
		const auto minDimenInv = 1.f / minDimen;

		hitBoxLength = std::sqrt(nodeSize * minDimenInv) / 24.f;
	}

	void EQPad::timerCallback()
	{
		bool needsRepaint = false;

		for (auto i = 0; i < numNodes(); ++i)
		{
			auto& node = nodes[i];
			if (node.valueChanged())
			{
				moveNode(i);
				needsRepaint = true;
			}
		}

		if (needsRepaint)
			repaint();
	}

	size_t EQPad::numNodes() noexcept
	{
		return nodes.size();
	}

	float EQPad::getNodeSize() noexcept
	{
		return utils.thicc * 5.f;
	}

	void EQPad::moveNode(int idx)
	{
		const auto x = bounds.getX();
		const auto y = bounds.getY();
		const auto w = bounds.getWidth();
		const auto h = bounds.getHeight();

		const auto nodeSize = getNodeSize();
		const auto nodeSizeHalf = nodeSize * .5f;

		auto& node = nodes[idx];

		const BoundsF nodeBounds
		(
			x + node.x * w - nodeSizeHalf,
			y + node.y * h - nodeSizeHalf,
			nodeSize,
			nodeSize
		);

		node.bounds = nodeBounds;
	}

	EQPad::Node* EQPad::getNode(const PointF& pos) noexcept
	{
		Node* closest = nullptr;

		float distance = std::numeric_limits<float>::max();
		for (auto& node : nodes)
		{
			PointF nodePos(node.x, node.y);
			auto nodeDistance = pos.getDistanceSquaredFrom(nodePos);
			if (distance > nodeDistance)
			{
				distance = nodeDistance;
				closest = &node;
			}
		}

		if (distance < hitBoxLength)
			return closest;

		return nullptr;
	}

	bool EQPad::alreadySelected(const Node& node) const noexcept
	{
		for (const auto& sel : selected)
			if (sel == &node)
				return true;
		return false;
	}

	bool EQPad::notSelectedYet(const Node& node) const noexcept
	{
		return !alreadySelected(node);
	}

	void EQPad::mouseMove(const Mouse& mouse)
	{
		auto h = getNode(normalize(mouse.position));
		if (hovered != h)
		{
			hovered = h;
			repaint();
		}
	}

	void EQPad::mouseDown(const Mouse& mouse)
	{
		bool clickedOnNode = hovered != nullptr;

		if (clickedOnNode)
		{
			tool = Tool::Move;
			dragXY = mouse.position;

			if (notSelectedYet(*hovered))
			{
				selected.clear();
				selected.push_back(hovered);
				selectionChanged();
			}

			if (!mouse.mods.isRightButtonDown())
				for (auto i = 0; i < selected.size(); ++i)
				{
					const auto& sel = *selected[i];
					sel.beginGesture();
				}
		}
		else
		{
			if (!mouse.mods.isRightButtonDown())
			{
				tool = Tool::Select;
				auto numSelected = selected.size();
				selected.clear();
				selectionLine.setStart(normalize(mouse.position));
				if (numSelected != selected.size())
					selectionChanged();
			}
		}

		repaint();
	}

	void EQPad::mouseDrag(const Mouse& mouse)
	{
		if (mouse.mods.isRightButtonDown())
			return;

		if (tool == Tool::Select)
		{
			selectionLine.setEnd
			(
				normalizeX(mouse.position.x),
				normalizeY(mouse.position.y)
			);

			selectionBounds = smallestBoundsIn(selectionLine);

			updateSelected();
			repaint();
		}
		else if (tool == Tool::Move)
		{
			hideCursor();

			auto shiftDown = mouse.mods.isShiftDown();
			const auto speed = 1.f / utils.getDragSpeed();

			float pos[2] =
			{
				mouse.position.x,
				mouse.position.y
			};

			auto dragOffset = mouse.position - dragXY;
			if (shiftDown)
				dragOffset *= SensitiveDrag;

			const float dof[NumDimensions] = { dragOffset.x, dragOffset.y };

			for (auto s = 0; s < selected.size(); ++s)
			{
				auto& sel = *selected[s];
				sel.onDrag(dof, speed);
			}

			dragXY = mouse.position;
		}
	}

	void EQPad::mouseUp(const Mouse& mouse)
	{
		if (mouse.mods.isRightButtonDown())
		{
			if (!mouse.mouseWasDraggedSinceMouseDown())
				for (auto i = 0; i < selected.size(); ++i)
				{
					auto& sel = *selected[i];
					sel.onRightClick();
				}
		}

		if (tool == Tool::Select)
		{
			selectionLine = { 0.f, 0.f, 0.f, 0.f };
			selectionBounds = { 0.f, 0.f, 0.f, 0.f };

			repaint();
		}
		else if (tool == Tool::Move)
		{
			if (!mouse.mods.isRightButtonDown())
			{
				bool wasDragged = mouse.mouseWasDraggedSinceMouseDown();

				for (auto i = 0; i < selected.size(); ++i)
				{
					auto& sel = *selected[i];
					sel.endGesture(wasDragged, mouse.mods.isAltDown());
				}

				if (wasDragged)
					showCursor(*this);
				auto mms = juce::Desktop::getInstance().getMainMouseSource();
				const auto pos = getScreenPosition().toFloat() + selected[0]->bounds.getCentre();
				mms.setScreenPosition(pos);
			}
		}

		notify(EvtType::ClickedEmpty);
	}

	void EQPad::mouseWheelMove(const Mouse& mouse, const MouseWheel& wheel)
	{
		if (selected.empty())
		{
			if (hovered != nullptr)
			{
				selected.push_back(hovered);
				repaint();
			}
		}
		else if (selected.size() == 1)
		{
			if (hovered != nullptr)
				if (!alreadySelected(*hovered))
				{
					selected[0] = hovered;
					repaint();
				}
		}

		for (auto s = 0; s < selected.size(); ++s)
		{
			auto& sel = *selected[s];
			sel.onScroll(wheel.deltaY, wheel.isReversed, mouse.mods.isShiftDown());
		}
	}

	void EQPad::mouseDoubleClick(const Mouse& mouse)
	{
		if (hovered == nullptr)
		{
			for (auto& node : nodes)
				if (node.rightClickParam->getValue() <= .5f)
				{
					const auto h = static_cast<float>(getHeight());

					node.rightClickParam->setValue(1.f);
					selected.clear();
					selected.push_back(&node);
					node.xyParam[Dimension::X]->setValue(normalizeX(mouse.position.x));
					node.xyParam[Dimension::Y]->setValue(normalizeY(h - mouse.position.y));
					selectionChanged();
					return;
				}
		}
		else
		{
			for (auto sel : selected)
				sel->onRightClick();

			selectionChanged();
		}
	}

	void EQPad::updateSelected()
	{
		bool changed = false;

		for (auto& node : nodes)
		{
			const auto x = node.x;
			const auto y = node.y;

			bool nodeInSelection = selectionBounds.contains(x, y);

			if (nodeInSelection)
			{
				if (notSelectedYet(node))
				{
					selected.push_back(&node);
					changed = true;
				}
			}
			else
			{
				if (alreadySelected(node))
				{
					removeNode(node);
					changed = true;
				}
			}
		}

		if (changed)
			selectionChanged();
	}

	void EQPad::removeNode(const Node& node)
	{
		for (auto i = 0; i < selected.size(); ++i)
			if (selected[i] == &node)
			{
				selected.erase(selected.begin() + i);
				return;
			}
	}

	//

	void EQPad::selectionChanged()
	{
		for (const auto& func : onSelectionChanged)
			func(selected);
	}

	float EQPad::normalizeX(float value) const noexcept
	{
		return (value - bounds.getX()) / bounds.getWidth();
	}

	float EQPad::normalizeY(float value) const noexcept
	{
		return (value - bounds.getY()) / bounds.getHeight();
	}

	float EQPad::denormalizeX(float value) const noexcept
	{
		return bounds.getX() + value * bounds.getWidth();
	}

	float EQPad::denormalizeY(float value) const noexcept
	{
		return bounds.getY() + value * bounds.getHeight();
	}

	PointF EQPad::normalize(PointF pt) const noexcept
	{
		return
		{
			normalizeX(pt.x),
			normalizeY(pt.y)
		};
	}

	PointF EQPad::denormalize(PointF pt) const noexcept
	{
		return
		{
			denormalizeX(pt.x),
			denormalizeY(pt.y)
		};
	}

	BoundsF EQPad::normalize(BoundsF b) const
	{
		return
		{
			normalizeX(b.getX()),
			normalizeY(b.getY()),
			normalizeX(b.getWidth()),
			normalizeY(b.getHeight())
		};
	}

	BoundsF EQPad::denormalize(BoundsF b) const
	{
		return
		{
			denormalizeX(b.getX()),
			denormalizeY(b.getY()),
			denormalizeX(b.getWidth()),
			denormalizeY(b.getHeight())
		};
	}

	PointF EQPad::limit(PointF pt) const noexcept
	{
		return
		{
			juce::jlimit(0.f, 1.f, pt.x),
			juce::jlimit(0.f, 1.f, pt.y)
		};
	}
}