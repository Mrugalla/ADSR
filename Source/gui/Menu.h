#pragma once
#include "TextEditor.h"

namespace gui
{
	Just getJust(const String&);

	/// Sub Menus:

	struct ColourSelector :
		public Comp,
		public Timer
	{
		using CS = juce::ColourSelector;

		ColourSelector(Utils&);

		void paint(Graphics&) override;

		void resized() override;

		void timerCallback() override;

	protected:
		CS selector;
		Button revert, deflt;
		Colour curSheme;
	};

	struct ErkenntnisseComp :
		public Comp,
		public Timer
	{
		ErkenntnisseComp(Utils&);

		void timerCallback() override;

		void resized() override;

		void paint(Graphics&) override;

		TextEditor editor;
		Label date;
		Button manifest, inspire, reveal, clear, paste;

	private:
		String getFolder();

		void saveToDisk();

		void parse(String&&);
	};

	struct JuxtaComp :
		public Comp
	{
		struct Jux :
			public Comp
		{
			/* utils, text, textA, textB */
			Jux(Utils&, String&&, String&&, String&&);

			void paint(Graphics&) override;

			void resized() override;

		protected:
			Label labelA, labelB;
		};

		JuxtaComp(Utils&, const ValueTree&);

		void paint(Graphics&) override;

		void resized() override;

		std::vector<std::unique_ptr<Jux>> juxi;
	};

	/// MENU STUFF IN GENERAL:

	struct ComponentWithBounds
	{
		/* comp, bounds, isQuad */
		template<typename CompType>
		ComponentWithBounds(CompType* = nullptr, BoundsF&& = BoundsF(0.f, 0.f, 1.f, 1.f), bool = false);

		std::unique_ptr<Component> c;
		BoundsF b;
		bool isQuad;
	};

	struct CompModular :
		public Comp
	{
		/* utils, tooltip, cursorType */
		CompModular(Utils&, String&&, CursorType);

		void init();

		std::vector<ComponentWithBounds> comps;
	protected:
		void paint(Graphics&) override;

		void resized() override;
	};

	class NavBar :
		public Comp
	{
		struct Node
		{
			Node(const ValueTree&, int, int);

			const ValueTree vt;
			const int x, y;
		};

		using Nodes = std::vector<Node>;

		Nodes makeNodes(const ValueTree&, int = 0, int = 0);

		int getDeepestNode() const noexcept;

	public:
		NavBar(Utils&, const ValueTree&);

		/* subMenu, parent */
		void init(std::unique_ptr<CompModular>&, Comp&);

	protected:
		Label label;
		Nodes nodes;
		std::vector<std::unique_ptr<Button>> buttons;
		const int numMenus, deepestNode;

		void paint(Graphics&) override;

		void resized() override;
	};

	struct Menu :
		public CompWidgetable
	{
		Menu(Utils&, const ValueTree&);

	protected:
		Label label;
		NavBar navBar;
		std::unique_ptr<CompModular> subMenu;

		void paint(juce::Graphics&) override;

		void resized() override;
	};
}