#pragma once
#include "Utils.h"
#include "Layout.h"
#include <juce_gui_basics/juce_gui_basics.h>

namespace gui
{
	struct Comp :
		public Component
	{
		/* utils, tooltip, cursorType */
		Comp(Utils&, const String& = "", CursorType = CursorType::Interact);

		/* utils, tooltip, notify, cursorType */
		Comp(Utils&, const String&, Notify&&, CursorType = CursorType::Interact);

		const Utils& getUtils() const noexcept;
		Utils& getUtils() noexcept;

		const String* getTooltip() const noexcept;
		String* getTooltip() noexcept;

		void setTooltip(String&&);
	
		void setCursorType(CursorType);

		void updateCursor();

		const Layout& getLayout() const noexcept;
		
		/* xL, yL */
		void initLayout(const std::vector<int>&, const std::vector<int>&);
		
		/* xL, yL */
		void initLayout(const String&, const String&);

		void notify(EvtType, const void* = nullptr);

		Utils& utils;
		Layout layout;
	protected:
		std::vector<Evt> evts;
		String tooltip;
		CursorType cursorType;

		void paint(Graphics&) override;

		void mouseEnter(const Mouse&) override;

		void mouseUp(const Mouse&) override;

	private:
		Notify makeNotifyBasic(Comp*);

	};

	struct CompWidgetable :
		public Comp,
		public Timer
	{
		/* utils, tooltip, cursorType */
		CompWidgetable(Utils&, String&& /*_tooltip*/, CursorType = CursorType::Interact);
		
		/* utils, tooltip, notify, cursorType */
		CompWidgetable(Utils&, String&&,
			Notify&& = [](EvtType, const void*) {}, CursorType = CursorType::Interact);

		void defineBounds(const BoundsF&, const BoundsF&);

		/* lengthInSecs, widgetEnvelope */
		void initWidget(float , bool = false);

		void updateBounds();

		void timerCallback() override;
		
		BoundsF bounds0, bounds1;
		float widgetEnvelope;
	private:
		float widgetInc;
	};

	struct CompScrollable :
		public Comp
	{
		struct ScrollBar :
			public Comp
		{
			static constexpr float SensitiveDrag = .2f;
			static constexpr float WheelDefaultSpeed = 12.f;

			/* utils, compScrollable, isVertical */
			ScrollBar(Utils&, CompScrollable&, bool = true);

			bool needed() const noexcept;

			void mouseEnter(const Mouse&) override;

			void mouseDown(const Mouse&) override;

			void mouseDrag(const Mouse&) override;

			void mouseUp(const Mouse&) override;

			void mouseExit(const Mouse&) override;

			void mouseWheelMove(const Mouse&, const MouseWheel&) override;

		protected:
			CompScrollable& scrollable;
			float dragXY;
			bool vertical;

			void paint(Graphics&) override;

			void updateHandlePosY(float);

			void updateHandlePosX(float);
		};

		/* utils, isVertical */
		CompScrollable(Utils&, bool = true);

		void mouseWheelMove(const Mouse&, const MouseWheel&) override;

	protected:
		ScrollBar scrollBar;
		float xScrollOffset, yScrollOffset, actualHeight;
	};

	struct CompScreenshotable :
		public Comp
	{
		using PPFunc = std::function<void(Graphics&, Image&)>;

		CompScreenshotable(Utils&);

		void resized() override;

		void paint(Graphics&) override;
		
		void takeScreenshot();

	protected:
		Image screenshotImage;
		std::vector<PPFunc> onScreenshotFX;
	};
}