#pragma once
#include "Button.h"
#include "../arch/Conversion.h"

namespace gui
{
	struct KeyboardComp :
		public Comp
	{
		/* noteVal */
		using PitchCallback = std::function<void(int)>;

	protected:
		struct Key :
			public Comp
		{
			/* utils, noteVal */
			Key(Utils&, int);
			
			void paint(Graphics&) override;

			void resized() override;
			
			int noteVal;
		protected:
			Colour bgCol;
			Label label;
		};

	public:
		/* utils, tooltip */
		KeyboardComp(Utils&, String&&);

		void resized() override;

		void paint(Graphics&) override;

		void mouseMove(const Mouse&) override;

		void mouseDown(const Mouse&) override;

		void mouseDrag(const Mouse&) override;

		void mouseUp(const Mouse&) override;

		void mouseExit(const Mouse&) override;

		PitchCallback onDown, onDrag, onUp;
	protected:
		std::array<std::unique_ptr<Key>, 24> keys;
		Button octDown, octUp;
		int hoverIdx, octIdx;
		
		int getHoverIdx(Point) noexcept;

		void keyRangeChanged();
		
		int getPitch() noexcept;
	};
	
}