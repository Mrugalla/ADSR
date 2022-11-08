#pragma once
#include "Button.h"
#include "TextEditor.h"

namespace gui
{
	class ContextMenu :
		public CompWidgetable
	{
		Notify makeNotify(ContextMenu&);

	public:
		ContextMenu(Utils&);

		void init();

		void place(const Comp*);

		void paint(Graphics&) override;

		/* name, tooltip */
		void addButton(String&&, String&&);
		
		/* onClick, idx */
		void setButton(const Button::OnClick&, int);

		std::vector<std::unique_ptr<Button>> buttons;
		std::vector<Label*> labelPtr;
	protected:
		PointF origin;
		BoundsF bounds;

		void resized() override;
	};

	
	class ContextMenuButtons :
		public ContextMenu
	{
		Notify makeNotify2(ContextMenuButtons&);

	public:
		ContextMenuButtons(Utils&);
	};


	class ContextMenuMacro :
		public ContextMenu
	{
		Notify makeNotify2(ContextMenuMacro&);

	public:
		ContextMenuMacro(Utils&);
	};
}