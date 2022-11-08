#pragma once
#include "../arch/FormulaParser2.h"
#include "TextEditor.h"

namespace gui
{
	struct FormulaParser :
		public TextEditor
	{
		using Parser = fx::Parser;

		enum PostFX
		{
			DCOffset,
			Windowing,
			Normalize,
			NumPostFX
		};

		/* utils, tooltip, tables, table size, table overshoot length */
		FormulaParser(Utils&, String&&, std::vector<float*>&, int, int = 0);

		/* samples */
		std::array<bool, NumPostFX> postFX;
		Parser fx;
		std::function<void()> updateFormula;
	};

	struct FormulaParser2 :
		public Comp
	{
		/* utils, tooltip, tables, table size, table overshoot length */
		FormulaParser2(Utils&, String&&, std::vector<float*>&, int, int);

		void paint(Graphics&);

		void resized() override;

		FormulaParser parser;
		Button dc, normalize, windowing, random, create;
	};
}