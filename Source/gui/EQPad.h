#pragma once
#include "GUIParams.h"
#include <array>

namespace gui
{
	class EQPad :
		public Comp,
		public Timer
	{
		enum Tool { Select, Move, NumTools };
	public:
		enum Dimension { X, Y, NumDimensions };
		
		struct Node
		{
			/* u, xPID, yPID, scrollPID, rightClickPID, morePIDs */
			Node(Utils&, PID, PID, PID, PID, const std::vector<PID>&);

			void paint(Graphics&) const;

			bool valueChanged() noexcept;

			float getValue(Dimension) const noexcept;

			void beginGesture() const;

			/* dof, speed */
			void onDrag(const float*, float);

			/* wasDragged, altDown */
			void endGesture(bool, bool);

			/* whellDeltaY, isReversed, shiftDown */
			void onScroll(float, bool, bool);

			void onRightClick();

			std::array<Param*, NumDimensions> xyParam;
			Param *scrollParam, *rightClickParam;
			std::vector<PID> morePIDs;
			BoundsF bounds;
			float x, y;
		protected:
			Utils& utils;
		};

		using Nodes = std::vector<Node>;
		using NodePtrs = std::vector<Node*>;
		using OnSelectionChanged = std::function<void(const NodePtrs&)>;
		using OnSelectionChangeds = std::vector<OnSelectionChanged>;
		
		/* utils, tooltip */
		EQPad(Utils&, String&&);

		/* xParam, yParam, scrollParam, rightClickParam, morePIDs */
		void addNode(PID, PID, PID, PID, const std::vector<PID> & = {});

		void selectNode(int);

		void selectAll();

		void paint(Graphics&) override;
		
		void paintHovered(Graphics&);

		/* graphics, thicc */
		void paintSelectionBounds(Graphics&, float);

		/* graphics, thicc */
		void paintHighlightSelected(Graphics&, float);
			
		void resized() override;

		void timerCallback() override;

		size_t numNodes() noexcept;

		float getNodeSize() noexcept;

		void moveNode(int);
		
		Node* getNode(const PointF&) noexcept;

		bool alreadySelected(const Node&) const noexcept;
		
		bool notSelectedYet(const Node&) const noexcept;

		void mouseMove(const Mouse&) override;

		void mouseDown(const Mouse&) override;

		void mouseDrag(const Mouse&) override;

		void mouseUp(const Mouse&) override;
		
		void mouseWheelMove(const Mouse&, const MouseWheel&) override;

		void mouseDoubleClick(const Mouse&) override;

		void updateSelected();

		void removeNode(const Node&);

		BoundsF bounds;
		OnSelectionChangeds onSelectionChanged;
	protected:
		Nodes nodes;
		NodePtrs selected;
		PointF dragXY;
		LineF selectionLine;
		BoundsF selectionBounds;
		Tool tool;
		Node* hovered;
		float hitBoxLength;
		
		void selectionChanged();

		float normalizeX(float) const noexcept;

		float normalizeY(float) const noexcept;

		float denormalizeX(float) const noexcept;

		float denormalizeY(float) const noexcept;

		PointF normalize(PointF) const noexcept;

		PointF denormalize(PointF) const noexcept;

		BoundsF normalize(BoundsF) const;

		BoundsF denormalize(BoundsF) const;
		
		PointF limit(PointF) const noexcept;
	};
}