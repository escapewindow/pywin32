# Run this as a python script, to gray "close" off the edit window system menu.
from pywin.framework import interact
import win32con

if __name__=='__main__':
	from . import demoutils
	if demoutils.NeedGoodGUI():
		win=interact.edit.currentView.GetParent()
		menu=win.GetSystemMenu()
		id=menu.GetMenuItemID(6)
		menu.EnableMenuItem(id,win32con.MF_BYCOMMAND|win32con.MF_GRAYED)
		print("The interactive window's 'Close' menu item is now disabled.")