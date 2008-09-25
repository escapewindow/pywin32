# This is a "demo" of win32file - it used to be more a test case than a 
# demo, so has been moved to the test directory.

# Please contribute your favourite simple little demo.
import pywintypes ## This can go away when bug fixed
import win32file, win32api, win32con
import os

# A very simple demo - note that this does no more than you can do with 
# builtin Python file objects, so for something as simple as this, you
# generally *should* use builtin Python objects.  Only use win32file etc
# when you need win32 specific features not available in Python.
def SimpleFileDemo():
    testName = os.path.join( win32api.GetTempPath(), "win32file_demo_test_file")
    if os.path.exists(testName): os.unlink(testName)
    # Open the file for writing.
    handle = win32file.CreateFile(testName, 
                                  win32file.GENERIC_WRITE, 
                                  0, 
                                  None, 
                                  win32con.CREATE_NEW, 
                                  0, 
                                  None)
    test_data = "Hello\0there"
    win32file.WriteFile(handle, test_data.encode('mbcs'))
    handle.Close()
    # Open it for reading.
    handle = win32file.CreateFile(testName, win32file.GENERIC_READ, 0, None, win32con.OPEN_EXISTING, 0, None)
    rc, data = win32file.ReadFile(handle, 1024)
    handle.Close()
    if data.decode('mbcs') == test_data:
        print("Successfully wrote and read a file")
    os.unlink(testName)

if __name__=='__main__':
    SimpleFileDemo()
